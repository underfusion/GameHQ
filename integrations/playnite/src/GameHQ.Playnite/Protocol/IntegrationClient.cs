using System;
using System.Collections.Concurrent;
using System.IO.Pipes;
using System.Security.Principal;
using System.Threading;
using Playnite.SDK;

namespace GameHQ.Playnite.Protocol
{
    internal enum IntegrationConnectionState
    {
        Disconnected,
        Connecting,
        Connected,
        // A maintenance window is active (app.maintenance) — the client
        // deliberately stops connecting and launching until it clears.
        Suspended
    }

    // Long-lived client for the GameHQ.Local.v1 named pipe. Owns a background
    // connect/reconnect loop so callers (Playnite event handlers) never block
    // waiting on GameHQ. See docs/integration-protocol.md for the wire spec
    // this implements, and plan item p5-2 for the locator/launcher it pairs
    // with.
    internal sealed class IntegrationClient : IDisposable
    {
        public const string PipeName = "GameHQ.Local.v1";
        private const string ClientName = "GameHQ.Playnite";
        private const int ProtocolMin = 1;
        private const int ProtocolMax = 1;
        private const int ConnectTimeoutMs = 1500;
        private const int HandshakeTimeoutMs = 5000;
        private const int MinBackoffMs = 500;
        private const int MaxBackoffMs = 30000;
        private const int OutgoingQueueCapacity = 64;

        private static readonly ILogger Logger = LogManager.GetLogger(nameof(IntegrationClient));

        private readonly string _clientVersion;
        private readonly BlockingCollection<IntegrationMessage> _outgoing =
            new BlockingCollection<IntegrationMessage>(new ConcurrentQueue<IntegrationMessage>(), OutgoingQueueCapacity);

        private readonly AutoResetEvent _wake = new AutoResetEvent(false);

        private CancellationTokenSource _cts;
        private Thread _worker;
        private volatile IntegrationConnectionState _state = IntegrationConnectionState.Disconnected;
        private DateTime _suspendedUntilUtc;
        private volatile string _remoteAppVersion;
        private int? _protocolSelected; // read/written from one worker thread at a time; UI reads are advisory
        private volatile string _lastError;

        public event Action<IntegrationConnectionState> StateChanged;
        public event Action<IntegrationMessage> MessageReceived;

        public IntegrationClient(string clientVersion)
        {
            _clientVersion = clientVersion;
        }

        public IntegrationConnectionState State
        {
            get { return _state; }
        }

        // Diagnostics for the settings page (p5-4) — the GameHQ version and
        // protocol reported by the last successful handshake, and the last
        // connection/handshake failure reason (cleared on success).
        public string RemoteAppVersion
        {
            get { return _remoteAppVersion; }
        }

        public int? ProtocolSelected
        {
            get { return _protocolSelected; }
        }

        public string LastError
        {
            get { return _lastError; }
        }

        // Nudges the background loop to retry immediately instead of
        // waiting out its current backoff. Used by the settings page's
        // "Test connection" action; safe to call at any time.
        public void TriggerReconnect()
        {
            _wake.Set();
        }

        public void Start()
        {
            if (_worker != null) return;

            _cts = new CancellationTokenSource();
            var token = _cts.Token;
            _worker = new Thread(() => RunLoop(token)) { IsBackground = true, Name = "GameHQ.Integration" };
            _worker.Start();
        }

        public void Stop()
        {
            var cts = _cts;
            _cts = null;
            _worker = null;
            if (cts != null) cts.Cancel();
            _wake.Set();
        }

        // Never blocks the caller. Drops the message and logs if GameHQ is
        // unreachable or the outgoing queue is saturated, rather than
        // stalling Playnite's UI or a game launch.
        public void Send(IntegrationMessage message)
        {
            if (_state != IntegrationConnectionState.Connected) return;
            if (!_outgoing.TryAdd(message))
                Logger.Warn("GameHQ integration outgoing queue full, dropping " + message.Type);
        }

        // Called when an app.maintenance broadcast is observed; suppresses
        // reconnect/launch attempts until the marker clears or the bounded
        // window passes. An update-related disconnect is expected, not a
        // crash to relaunch from.
        public void EnterMaintenance(int retryAfterSeconds)
        {
            _suspendedUntilUtc = DateTime.UtcNow.AddSeconds(Math.Max(1, retryAfterSeconds));
            SetState(IntegrationConnectionState.Suspended);
        }

        private void RunLoop(CancellationToken token)
        {
            int backoffMs = MinBackoffMs;

            while (!token.IsCancellationRequested)
            {
                if (_state == IntegrationConnectionState.Suspended)
                {
                    if (DateTime.UtcNow < _suspendedUntilUtc)
                    {
                        Thread.Sleep(500);
                        continue;
                    }
                    SetState(IntegrationConnectionState.Disconnected);
                }

                bool handshakeOk = false;
                using (var pipe = new NamedPipeClientStream(".", PipeName, PipeDirection.InOut, PipeOptions.None, TokenImpersonationLevel.None))
                {
                    try
                    {
                        SetState(IntegrationConnectionState.Connecting);
                        pipe.Connect(ConnectTimeoutMs);

                        handshakeOk = Handshake(pipe);
                        if (handshakeOk)
                        {
                            backoffMs = MinBackoffMs;
                            SetState(IntegrationConnectionState.Connected);
                            PumpUntilDisconnected(pipe, token);
                        }
                    }
                    catch (TimeoutException)
                    {
                        // GameHQ not listening yet; back off and retry.
                        _lastError = "GameHQ is not running or not reachable";
                    }
                    catch (Exception ex)
                    {
                        Logger.Warn("GameHQ integration connection error: " + ex.Message);
                        _lastError = ex.Message;
                    }
                }

                if (_state != IntegrationConnectionState.Suspended)
                    SetState(IntegrationConnectionState.Disconnected);

                if (token.IsCancellationRequested) break;

                backoffMs = handshakeOk ? MinBackoffMs : NextBackoff(backoffMs);
                _wake.WaitOne(backoffMs);
            }
        }

        private bool Handshake(NamedPipeClientStream pipe)
        {
            pipe.ReadMode = PipeTransmissionMode.Byte;

            var hello = new IntegrationMessage("hello")
                .Set("client", ClientName)
                .Set("clientVersion", _clientVersion)
                .Set("protocolMin", ProtocolMin)
                .Set("protocolMax", ProtocolMax)
                .Set("requestId", Guid.NewGuid().ToString("N"));

            PipeFraming.WriteFrame(pipe, hello.ToUtf8Json());

            var deadline = DateTime.UtcNow.AddMilliseconds(HandshakeTimeoutMs);
            pipe.ReadTimeout = HandshakeTimeoutMs;

            var payload = PipeFraming.ReadFrame(pipe);
            if (payload == null) return false;

            var reply = IntegrationMessage.FromUtf8Json(payload);
            if (reply.Type == "hello.ack")
            {
                _remoteAppVersion = reply.GetString("appVersion");
                _protocolSelected = reply.GetInt("protocolSelected");
                _lastError = null;
                return true;
            }

            if (reply.Type == "error")
            {
                var message = reply.GetString("code") + ": " + reply.GetString("message");
                Logger.Warn("GameHQ integration handshake rejected: " + message);
                _lastError = message;
            }

            return false;
        }

        private void PumpUntilDisconnected(NamedPipeClientStream pipe, CancellationToken token)
        {
            using (var stopped = new ManualResetEventSlim(false))
            {
                Exception readerError = null;
                var reader = new Thread(() =>
                {
                    try
                    {
                        while (!token.IsCancellationRequested)
                        {
                            var payload = PipeFraming.ReadFrame(pipe);
                            if (payload == null) break; // graceful disconnect
                            HandleIncoming(IntegrationMessage.FromUtf8Json(payload));
                        }
                    }
                    catch (Exception ex)
                    {
                        readerError = ex;
                    }
                    finally
                    {
                        stopped.Set();
                    }
                }) { IsBackground = true, Name = "GameHQ.Integration.Reader" };
                reader.Start();

                while (!stopped.IsSet && !token.IsCancellationRequested)
                {
                    IntegrationMessage outgoing;
                    if (_outgoing.TryTake(out outgoing, 250))
                    {
                        try
                        {
                            PipeFraming.WriteFrame(pipe, outgoing.ToUtf8Json());
                        }
                        catch (Exception ex)
                        {
                            Logger.Warn("GameHQ integration send failed: " + ex.Message);
                            break;
                        }
                    }
                }

                if (readerError != null)
                    Logger.Warn("GameHQ integration read failed: " + readerError.Message);

                stopped.Wait(TimeSpan.FromSeconds(2));
            }
        }

        private void HandleIncoming(IntegrationMessage message)
        {
            if (message.Type == "app.maintenance")
            {
                EnterMaintenance(message.GetInt("retryAfterSeconds") ?? 30);
            }

            var handler = MessageReceived;
            if (handler != null) handler(message);
        }

        private void SetState(IntegrationConnectionState state)
        {
            if (_state == state) return;
            _state = state;
            var handler = StateChanged;
            if (handler != null) handler(state);
        }

        private static int NextBackoff(int currentMs)
        {
            var next = currentMs * 2;
            return next > MaxBackoffMs ? MaxBackoffMs : next;
        }

        public void Dispose()
        {
            Stop();
            _outgoing.Dispose();
            _wake.Dispose();
        }
    }
}
