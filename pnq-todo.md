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

Every regis3 finding below is the same mistake at a different layer: **a boolean where a
tri-state is needed.**

`RegOpenKeyEx` and `RegEnumKeyEx` each distinguish "not there" (`ERROR_FILE_NOT_FOUND`,
`ERROR_NO_MORE_ITEMS`) from "could not look" (`ERROR_ACCESS_DENIED`, and everything else).
`key::last_status()` now reports it at the primitive, but every layer above still throws it
away, so a key nobody is allowed to read is reported exactly like a key that is not there.

The Windows API already provides the distinction. regis3 discards it.

---

## 3. `import_recursive()` reports a partial traversal as a complete one — P1

`include/pnq/regis3/importer.h:226-241`

```cpp
key_entry* entry = parent->find_or_create_key(subkey_name);

key subkey{subkey_path};
if (subkey.open_for_reading())
{
    import_recursive(entry, subkey);
}
```

Note the order: `find_or_create_key()` runs **before** the open attempt. An unreadable subkey
is therefore not omitted — it is added to the tree *empty*, which is worse, because an empty
key is a legitimate thing for a registry to contain. There is no signal anywhere that the
subtree was skipped.

A key whose children are partly unreadable captures as a complete-looking subset. Nothing
returns an error, nothing sets a flag, and the caller has no way to ask.

**Fixing this needs a contract decision**, which is why insti could not work around it:
`import()` is already fallible for an unreadable *root*, so either that extends to any
unreadable subkey - one denied leaf fails the whole import - or the importer carries a
"traversal was incomplete" flag that a caller can check. The flag changes `import_interface`,
which other consumers may rely on.

## 4. `key_iterator` cannot distinguish enumeration failure from end-of-enumeration — P1

`include/pnq/regis3/iterators.h:174`, advance loop at `:240-274`

```cpp
if (result == ERROR_MORE_DATA) { /* grow buffers, retry */ }
else
{
    return false;   // ERROR_NO_MORE_ITEMS and ERROR_ACCESS_DENIED, identically
}
```

`ERROR_NO_MORE_ITEMS` is the normal end of enumeration. Every other failure returns the same
`false`, so a range-`for` over `enum_keys()` simply stops early and looks finished. Unlike the
comparable loop at `key.h:281`, this one does not even log — `PNQ_LOG_WIN_ERROR` is absent, so
a truncated enumeration leaves no trace at all.

This is what item 3 iterates. Fixing 3 without fixing 4 leaves the same hole one level down.

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

**3 → 4** is one piece of work, not two: 4 is the enumeration half of 3. Both rest on
`key::last_status()`. Fixing one alone leaves the same defect reachable through the other.

**5, 6 and 7 are independent** and can go in any order.

Item 3's contract change is the only one that can break other consumers, so it deserves a
deliberate decision about whether `import()` becomes fallible or gains a companion flag.
