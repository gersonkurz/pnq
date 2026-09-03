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
`last_status()`, and `registry_importer::was_complete()`. What is left below is the same
mistake outside regis3, plus one API that promises more than it checks.

---

## 5. `create_importer_from_string()` validates only the header — P2

`include/pnq/regis3/importer.h:122-141`

The factory checks for `HEADER_FORMAT5`, `HEADER_FORMAT4` or a UTF-8 BOM and returns an
importer. The body is not parsed until `import()` is called, so a **successfully constructed
importer says almost nothing about whether the content is parsable.**

Not wrong, but the name invites the wrong reading, and a caller validating input will get it
wrong on the first try.

**What it cost downstream:** insti used a successful construction as its "is this .reg valid"
check at preflight. A malformed body with a valid header passed, the live registry key was
cleaned, and the restore then failed — the live key destroyed on the strength of a check that
had not parsed anything. insti now runs the real parse and releases the result
(`shared/src/actions/registry.cpp:147-161`).

Either document that construction is a format sniff, or add a `validate()` that parses.

---

## Outside regis3

Same defect class, different modules. Listed here because they came from the same review and
have the same shape.

## 6. `file::exists()` and `directory::exists()` collapse "absent" and "unreadable" — P2

`include/pnq/file.h:40-53`, `include/pnq/directory.h:15-17`

`file::exists()` inspects `GetLastError()`, logs anything that is not
`ERROR_FILE_NOT_FOUND` / `ERROR_PATH_NOT_FOUND`, and then returns `false` anyway — the same
answer as genuine absence. `directory::exists()` does not check `GetLastError()` at all.

Same shape as the `open_for_reading()` defect, in a different namespace: the caller cannot
tell a missing file from one it lacks permission to stat. `key::last_status()` is the pattern
to copy.

**How insti works around it:** `probe_path()`, a three-valued probe that calls
`GetFileAttributesW` directly and inspects the error. Note the measurement, since the obvious
test does not reproduce it: making an attribute query fail needs a DENY on the parent
directory's `FILE_LIST_DIRECTORY` **and** on the file's `FILE_READ_ATTRIBUTES`. Holding the
file open with `FILE_SHARE_NONE` does not do it — an attribute-only query is answered from the
parent's directory entry when it can be, skipping the sharing check.

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

---

## Fix order

**5, 6 and 7 are independent** and can go in any order.
