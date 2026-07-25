# Protocol

Client-side implementation of the `GameHQ.Local.v1` named-pipe protocol. The
wire protocol itself is specified in
`../../../../docs/integration-protocol.md` in the main GameHQ app — this
folder implements it, it does not redefine it.

- `PipeFraming.cs` — the 4-byte length-prefix + UTF-8 JSON frame format.
- `IntegrationMessage.cs` — a loosely-typed message wrapper (JSON in/out).
- `IntegrationClient.cs` — owns the `NamedPipeClientStream`, the handshake,
  a background reconnect loop with bounded backoff, a bounded outgoing
  queue, and `app.maintenance` awareness. Never blocks the caller.

Game-lifecycle message sending (`playnite.game.*`, `playnite.state.sync`)
lands in plan item p5-3.
