/// @file sqlite.h
/// @brief SQLite wrapper - include sqlite3.h BEFORE this header to enable.
///
/// Usage:
/// @code
/// #include <sqlite3.h>       // You provide this
/// #include <pnq/sqlite/sqlite.h>  // Or just include pnq/pnq.h
///
/// pnq::sqlite::Database db;
/// db.open("mydb.sqlite");
/// @endcode
///
/// If sqlite3.h is not in the include path, this header is empty (no-op).
#pragma once

#if __has_include(<sqlite3.h>)

#include <pnq/sqlite/database.h>
#include <pnq/sqlite/statement.h>
#include <pnq/sqlite/transaction.h>

#endif // __has_include(<sqlite3.h>)
