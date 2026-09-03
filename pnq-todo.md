# pnq — defects found by a downstream consumer

## Why this list exists

An external correctness review of **insti** (`github.com/gersonkurz/insti`, Aug–Sep 2026) ran
to 14 rounds against one defect class: *a failure that is indistinguishable from legitimate
absence*. insti captures and restores machine state — files, registry keys, services — so a
"the resource was not there" answer that actually means "I could not look" causes it to record
absence, and restore then removes the live resource to match.

Several of those findings bottomed out in **pnq**, not in insti. insti has worked around each
one locally; the workarounds are named below because they are evidence of what a fixed API has
to provide, not a substitute for fixing it.

Everything here was verified against `main` at `1d16f05`, not against the older commit insti
currently pins.

**IDs are stable.** insti's source comments and `insti-todo.md` will cite them.

---

## The root cause, stated once

Most of this list was the same mistake at a different layer: **a boolean where a tri-state is
needed.**

`RegOpenKeyEx` and `RegEnumKeyEx` each distinguish "not there" (`ERROR_FILE_NOT_FOUND`,
`ERROR_NO_MORE_ITEMS`) from "could not look" (`ERROR_ACCESS_DENIED`, and everything else).
regis3 now carries that distinction through `key::last_status()`, the enumerators'
`last_status()`, and `registry_importer::was_complete()`; `file::exists()` and
`directory::exists()` carry it through `GetLastError()`. Item 7 below is a different mistake
with the same consequence: a value silently replaced by another.

---

## 7. `create_service()` silently downgrades a boot-start driver — P2

`include/pnq/win32/service.h:511`

```cpp
config.start_type ? config.start_type : SERVICE_DEMAND_START,
```

`SERVICE_BOOT_START` is **0**, so a caller asking for a boot-start driver gets
`SERVICE_DEMAND_START` — a service that ran at boot now starts on demand, at a completely
different point in startup, with no error. The same line does the same thing to
`service_type`, where `0` is not a valid type, so that half is defensible; the `start_type`
half is not.

A `DWORD` whose valid range includes zero cannot use zero as "unset". Either take
`std::optional<DWORD>`, or default the parameter and drop the ternary.

**How insti works around it:** it refuses to capture a boot-start service rather than
recording one it cannot restore faithfully (`shared/src/actions/service_action.cpp:109-117`).
