#pragma once

#include <pnq/targetver.h>

#include <Windows.h>

#include <combaseapi.h>
#include <shellapi.h>
#include <shlobj_core.h>
#include <psapi.h>
#include <strsafe.h>

// standard C++ headers
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <ios>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <string>
#include <source_location>
#include <tchar.h>
#include <time.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef PNQ_USE_MEMORY_DEBUGGING
#define PNQ_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ ) 
#else
#define PNQ_NEW new
#endif

// see http://stackoverflow.com/questions/5641427/how-to-make-preprocessor-generate-a-string-for-line-keyword
#define S(x) #x
#define S_(x) S(x)
#define S__LINE__ S_(__LINE__)

// see http://msdn.microsoft.com/de-de/library/b0084kay.aspx
#define PNQ_FUNCTION_CONTEXT __FUNCTION__ "[" S__LINE__ "]"

/// This macro can be used on classes that should not enable a copy / move constructor / assignment operator
#define PNQ_DECLARE_NON_COPYABLE(__CLASSNAME__) \
    __CLASSNAME__(const __CLASSNAME__&) = delete; \
    __CLASSNAME__& operator=(const __CLASSNAME__&) = delete; \
    __CLASSNAME__(__CLASSNAME__&&) = delete; \
    __CLASSNAME__& operator=(__CLASSNAME__&&) = delete;

namespace pnq
{
    template <typename X, typename Y> X truncate_cast(Y y)
    {
        // if this assertion fails, it means the truncate-cast is loosing information, which it shouldn't.
        assert(y == (Y)((X)y));
        return (X)y;
    }
}


#include <pnq/app_init.h>
#include <pnq/binary_file.h>
#include <pnq/console.h>
#include <pnq/directory.h>
#include <pnq/ref_counted.h>
#include <pnq/environment_variables.h>
#include <pnq/file.h>
#include <pnq/memory_view.h>
#include <pnq/path.h>
#include <pnq/string.h>
#include <pnq/string_expander.h>
#include <pnq/string_writer.h>
#include <pnq/text_file.h>
#include <pnq/version.h>
#include <pnq/windows_errors.h>
#include <pnq/wstring.h>

#include <pnq/win32/critical_section.h>
#include <pnq/win32/event_semaphore.h>
#include <pnq/win32/handle.h>
#include <pnq/win32/security_attributes.h>
#include <pnq/win32/wstr_param.h>

#include <pnq/config/config_backend.h>
#include <pnq/config/section.h>
#include <pnq/config/toml_backend.h>
#include <pnq/config/typed_value.h>
#include <pnq/config/typed_vector_value.h>
#include <pnq/config/value_interface.h>

#include <pnq/sqlite/sqlite.h>

namespace fs = std::filesystem;
