---
name: plan-api-mutation-ops
description: "Cockpit plan API — which PATCH mutation ops exist, and that current_item_id auto-advances"
metadata: 
  node_type: memory
  type: reference
  originSessionId: 3726f5a3-e657-4ad3-bc6c-005cd55d0034
---

The Cockpit plan API (`PATCH /api/plans/documents/<id>`) supports `update_item` and `update_plan`.
There is **no `set_current_item` / `set_current` / `update_document` op** — sending one returns
HTTP 422 `{"code":"plan_document_error","message":"Unsupported mutation operation: ..."}` and the
whole PATCH is rejected, so any valid `update_item` in the same `operations` array is lost too.

`current_item_id` advances **automatically** when an item leaves `todo` — never try to set it.

**Why:** repeatedly hand-writing `set_current_item` made every plan update fail and looked like a
broken plan API; the actual item status change never landed.

**How to apply:** send only `update_item` ops, then re-read the document to see where
`current_item_id` landed. Always pipe the response through a filter (`| python -c "..."`) — the
full document reply is ~44 KB and blows up the tool output. Include `cwd` on every call.

Related: [[gamehq-build-commands]]
