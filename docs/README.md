# GameHQ Documentation

Technical and product documentation for contributors and maintainers.

## Start here

- [Development Setup](dev-setup.md) — toolchain, configure, and build steps.
- [Architecture](architecture.md) — modules, data flow, and repository layout.
- [Product Specification](product-spec.md) — intended behavior and user flows.
- [Packaging & Distribution](packaging.md) — clean portable release layout.

## Subsystems

| Document | Topic |
|---|---|
| [branding.md](branding.md) | Product identity and naming |
| [capture-engine.md](capture-engine.md) | Screenshots and Windows Graphics Capture |
| [controller-input.md](controller-input.md) | Controllers, keyboard, mouse, gestures, and bindings |
| [database.md](database.md) | SQLite schema and migrations |
| [design-system.md](design-system.md) | Visual tokens, components, focus, and motion |
| [notifications.md](notifications.md) | In-app toast notifications |
| [overlay.md](overlay.md) | In-game overlay behavior and constraints |
| [replay-buffer.md](replay-buffer.md) | Rolling capture and MP4 export |
| [sound-system.md](sound-system.md) | Sound events and settings |
| [storage.md](storage.md) | Portable paths, watched folders, and cleanup |

## Project maintenance

| Document | Topic |
|---|---|
| [roadmap.md](roadmap.md) | Product direction |
| [technical-audit.md](technical-audit.md) | Architecture risks and improvement areas |
| [versioning.md](versioning.md) | Version and release rules |

When implementation changes behavior, update the relevant document in the same
pull request.
