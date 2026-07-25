using System;
using System.IO;

namespace GameHQ.Playnite.Protocol
{
    // Wire framing shared with the GameHQ app: a 4-byte little-endian payload
    // length prefix followed by that many bytes of UTF-8 JSON. See
    // docs/integration-protocol.md in the main repo for the full spec.
    internal static class PipeFraming
    {
        public const int MaxFrameBytes = 64 * 1024;

        public static void WriteFrame(Stream stream, byte[] payload)
        {
            if (payload == null || payload.Length == 0 || payload.Length > MaxFrameBytes)
                throw new InvalidOperationException("Frame payload out of bounds: " + (payload?.Length ?? -1));

            var header = BitConverter.GetBytes(payload.Length);
            if (!BitConverter.IsLittleEndian)
                Array.Reverse(header);

            stream.Write(header, 0, header.Length);
            stream.Write(payload, 0, payload.Length);
            stream.Flush();
        }

        // Returns null if the stream ended cleanly before a new frame started
        // (a graceful disconnect between messages, not mid-frame).
        public static byte[] ReadFrame(Stream stream)
        {
            var header = ReadExact(stream, 4);
            if (header == null)
                return null;

            if (!BitConverter.IsLittleEndian)
                Array.Reverse(header);

            int length = BitConverter.ToInt32(header, 0);
            if (length <= 0 || length > MaxFrameBytes)
                throw new IOException("Invalid frame length: " + length);

            var payload = ReadExact(stream, length);
            if (payload == null)
                throw new IOException("Connection closed mid-frame");

            return payload;
        }

        private static byte[] ReadExact(Stream stream, int count)
        {
            var buffer = new byte[count];
            int offset = 0;
            while (offset < count)
            {
                int read = stream.Read(buffer, offset, count - offset);
                if (read == 0)
                {
                    if (offset == 0) return null;
                    throw new IOException("Connection closed mid-frame");
                }
                offset += read;
            }
            return buffer;
        }
    }
}
