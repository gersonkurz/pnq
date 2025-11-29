#include <catch2/catch_test_macros.hpp>
#include <pnq/pnq.h>

TEST_CASE("Version is defined", "[version]") {
    REQUIRE(pnq::version_major == 0);
    REQUIRE(pnq::version_minor == 1);
    REQUIRE(pnq::version_patch == 0);
}

TEST_CASE("windows::hresult_as_string", "[windows]") {
    using pnq::windows::hresult_as_string;

    SECTION("known system error") {
        auto msg = hresult_as_string(E_INVALIDARG);
        REQUIRE_FALSE(msg.empty());
        // Known errors should return descriptive text
        REQUIRE(msg.size() > 10);
    }

    SECTION("S_OK") {
        auto msg = hresult_as_string(S_OK);
        REQUIRE_FALSE(msg.empty());
    }

    SECTION("unknown error returns something") {
        // 0xDEADBEEF might actually be a known NTSTATUS, just verify we get something
        auto msg = hresult_as_string(static_cast<HRESULT>(0x87654321));
        REQUIRE_FALSE(msg.empty());
    }

    SECTION("result has no trailing newline") {
        auto msg = hresult_as_string(E_FAIL);
        REQUIRE_FALSE(msg.empty());
        REQUIRE(msg.back() != '\n');
        REQUIRE(msg.back() != '\r');
    }
}

TEST_CASE("text_file read/write", "[text_file]") {
    namespace tf = pnq::text_file;

    // Get temp directory
    wchar_t temp_path[MAX_PATH];
    GetTempPathW(MAX_PATH, temp_path);
    std::wstring temp_dir(temp_path);

    SECTION("write_utf8 with BOM and read_auto") {
        std::string filename = pnq::string::encode_as_utf8(temp_dir + L"pnq_test_utf8_bom.txt");
        std::string content = "Hello, UTF-8 with BOM!";

        REQUIRE(tf::write_utf8(filename, content, true));
        auto result = tf::read_auto(filename);
        REQUIRE(result == content);

        DeleteFileW((temp_dir + L"pnq_test_utf8_bom.txt").c_str());
    }

    SECTION("write_utf8 without BOM and read_auto") {
        std::string filename = pnq::string::encode_as_utf8(temp_dir + L"pnq_test_utf8_nobom.txt");
        std::string content = "Hello, UTF-8 without BOM!";

        REQUIRE(tf::write_utf8(filename, content, false));
        auto result = tf::read_auto(filename);
        REQUIRE(result == content);

        DeleteFileW((temp_dir + L"pnq_test_utf8_nobom.txt").c_str());
    }

    SECTION("write_utf16 with BOM and read_auto") {
        std::string filename = pnq::string::encode_as_utf8(temp_dir + L"pnq_test_utf16.txt");
        std::wstring content = L"Hello, UTF-16LE!";

        REQUIRE(tf::write_utf16(filename, content, true));
        auto result = tf::read_auto(filename);
        // Result should be UTF-8 conversion of the UTF-16 content
        REQUIRE(result == "Hello, UTF-16LE!");

        DeleteFileW((temp_dir + L"pnq_test_utf16.txt").c_str());
    }

    SECTION("read_auto on non-existent file returns empty") {
        auto result = tf::read_auto("C:\\this_file_does_not_exist_12345.txt");
        REQUIRE(result.empty());
    }
}

TEST_CASE("string::is_empty", "[string]") {
    using pnq::string::is_empty;

    SECTION("nullptr is empty") {
        REQUIRE(is_empty(static_cast<const char *>(nullptr)));
        REQUIRE(is_empty(static_cast<const wchar_t *>(nullptr)));
    }

    SECTION("empty string is empty") {
        REQUIRE(is_empty(""));
        REQUIRE(is_empty(L""));
    }

    SECTION("non-empty string is not empty") {
        REQUIRE_FALSE(is_empty("hello"));
        REQUIRE_FALSE(is_empty(L"hello"));
    }
}

TEST_CASE("string::equals_nocase", "[string]") {
    using pnq::string::equals_nocase;

    SECTION("equal strings, same case") {
        REQUIRE(equals_nocase("hello", "hello"));
    }

    SECTION("equal strings, different case") {
        REQUIRE(equals_nocase("Hello", "hELLO"));
        REQUIRE(equals_nocase("HELLO", "hello"));
    }

    SECTION("different strings") {
        REQUIRE_FALSE(equals_nocase("hello", "world"));
    }

    SECTION("different lengths") {
        REQUIRE_FALSE(equals_nocase("hello", "hello!"));
        REQUIRE_FALSE(equals_nocase("hello!", "hello"));
    }

    SECTION("empty strings") {
        REQUIRE(equals_nocase("", ""));
        REQUIRE_FALSE(equals_nocase("", "a"));
    }
}

TEST_CASE("string::starts_with_nocase", "[string]") {
    using pnq::string::starts_with_nocase;

    SECTION("matching prefix") {
        REQUIRE(starts_with_nocase("Hello World", "hello"));
        REQUIRE(starts_with_nocase("HELLO", "hel"));
    }

    SECTION("non-matching prefix") {
        REQUIRE_FALSE(starts_with_nocase("Hello", "world"));
    }

    SECTION("prefix longer than string") {
        REQUIRE_FALSE(starts_with_nocase("Hi", "Hello"));
    }

    SECTION("empty prefix matches everything") {
        REQUIRE(starts_with_nocase("Hello", ""));
    }
}

TEST_CASE("string::join", "[string]") {
    using pnq::string::join;

    SECTION("join multiple strings") {
        std::vector<std::string> items{"a", "b", "c"};
        REQUIRE(join(items, ", ") == "a, b, c");
    }

    SECTION("join single string") {
        std::vector<std::string> items{"alone"};
        REQUIRE(join(items, ", ") == "alone");
    }

    SECTION("join empty vector") {
        std::vector<std::string> items;
        REQUIRE(join(items, ", ") == "");
    }
}

TEST_CASE("string::uppercase/lowercase", "[string]") {
    SECTION("uppercase") {
        REQUIRE(pnq::string::uppercase("hello") == "HELLO");
        REQUIRE(pnq::string::uppercase("Hello World") == "HELLO WORLD");
    }

    SECTION("lowercase") {
        REQUIRE(pnq::string::lowercase("HELLO") == "hello");
        REQUIRE(pnq::string::lowercase("Hello World") == "hello world");
    }
}

TEST_CASE("string::split", "[string]") {
    using pnq::string::split;

    SECTION("split by single separator") {
        auto result = split("a,b,c", ",");
        REQUIRE(result.size() == 3);
        REQUIRE(result[0] == "a");
        REQUIRE(result[1] == "b");
        REQUIRE(result[2] == "c");
    }

    SECTION("split by multiple separators") {
        auto result = split("a,b;c", ",;");
        REQUIRE(result.size() == 3);
    }

    SECTION("split empty string") {
        auto result = split("", ",");
        REQUIRE(result.empty());
    }

    SECTION("split with quotes") {
        auto result = split("a,\"b,c\",d", ",", true);
        // Note: produces ["a", "b,c", "", "d"] - empty string after closing quote before comma
        REQUIRE(result.size() == 4);
        REQUIRE(result[0] == "a");
        REQUIRE(result[1] == "b,c");
        REQUIRE(result[3] == "d");
    }
}

TEST_CASE("string::encode_as_utf8/utf16", "[string]") {
    SECTION("utf16 to utf8") {
        auto result = pnq::string::encode_as_utf8(L"Hello");
        REQUIRE(result == "Hello");
    }

    SECTION("utf8 to utf16") {
        auto result = pnq::string::encode_as_utf16("Hello");
        REQUIRE(result == L"Hello");
    }

    SECTION("roundtrip with unicode") {
        std::wstring original = L"Héllo Wörld 日本語";
        auto utf8 = pnq::string::encode_as_utf8(original);
        auto back = pnq::string::encode_as_utf16(utf8);
        REQUIRE(back == original);
    }

    SECTION("empty string") {
        REQUIRE(pnq::string::encode_as_utf8(L"").empty());
        REQUIRE(pnq::string::encode_as_utf16("").empty());
    }
}

TEST_CASE("string::escape_json_string", "[string]") {
    using pnq::string::escape_json_string;

    SECTION("simple string") {
        REQUIRE(escape_json_string("hello") == "\"hello\"");
    }

    SECTION("string with quotes") {
        REQUIRE(escape_json_string("say \"hi\"") == "\"say \\\"hi\\\"\"");
    }

    SECTION("string with backslash") {
        REQUIRE(escape_json_string("a\\b") == "\"a\\\\b\"");
    }

    SECTION("string with newlines") {
        REQUIRE(escape_json_string("a\nb\r\n") == "\"a\\nb\\r\\n\"");
    }

    SECTION("string with tab") {
        REQUIRE(escape_json_string("a\tb") == "\"a\\tb\"");
    }
}

TEST_CASE("string::from_hex_string", "[string]") {
    using pnq::string::from_hex_string;
    uint32_t result;

    SECTION("valid hex") {
        REQUIRE(from_hex_string("FF", result));
        REQUIRE(result == 255);

        REQUIRE(from_hex_string("deadbeef", result));
        REQUIRE(result == 0xDEADBEEF);
    }

    SECTION("hex with 0x prefix fails") {
        // from_chars doesn't handle 0x prefix
        REQUIRE_FALSE(from_hex_string("0xFF", result));
    }

    SECTION("invalid hex") {
        REQUIRE_FALSE(from_hex_string("xyz", result));
        REQUIRE_FALSE(from_hex_string("12g", result));
    }
}

TEST_CASE("string::slice", "[string]") {
    using pnq::string::slice;

    SECTION("positive indices") {
        REQUIRE(slice("hello", 0, 2) == "he");
        REQUIRE(slice("hello", 1, 4) == "ell");
    }

    SECTION("negative indices") {
        REQUIRE(slice("hello", -3, -1) == "ll");
        REQUIRE(slice("hello", -2, 5) == "lo");
    }

    SECTION("empty result") {
        REQUIRE(slice("hello", 3, 2) == "");
        REQUIRE(slice("", 0, 1) == "");
    }
}

TEST_CASE("string::split_at_first_occurence", "[string]") {
    using pnq::string::split_at_first_occurence;

    SECTION("split by char") {
        auto [first, second] = split_at_first_occurence("key=value", '=');
        REQUIRE(first == "key");
        REQUIRE(second == "value");
    }

    SECTION("split by char not found") {
        auto [first, second] = split_at_first_occurence("keyvalue", '=');
        REQUIRE(first == "keyvalue");
        REQUIRE(second == "");
    }

    SECTION("split by substring") {
        auto [first, second] = split_at_first_occurence("hello::world", "::");
        REQUIRE(first == "hello");
        REQUIRE(second == "world");
    }
}

TEST_CASE("string::split_at_last_occurence", "[string]") {
    using pnq::string::split_at_last_occurence;

    SECTION("split by char") {
        auto [first, second] = split_at_last_occurence("path/to/file", '/');
        REQUIRE(first == "path/to");
        REQUIRE(second == "file");
    }

    SECTION("split by char not found") {
        auto [first, second] = split_at_last_occurence("filename", '/');
        REQUIRE(first == "");
        REQUIRE(second == "filename");
    }
}

TEST_CASE("string::Writer", "[string_writer]") {
    using pnq::string::Writer;

    SECTION("empty writer") {
        Writer w;
        REQUIRE(w.empty());
        REQUIRE(w.as_string() == "");
    }

    SECTION("append char") {
        Writer w;
        w.append('H');
        w.append('i');
        REQUIRE_FALSE(w.empty());
        REQUIRE(w.as_string() == "Hi");
    }

    SECTION("append c-string") {
        Writer w;
        w.append("Hello");
        w.append(" ");
        w.append("World");
        REQUIRE(w.as_string() == "Hello World");
    }

    SECTION("append string_view") {
        Writer w;
        std::string_view sv = "test";
        w.append(sv);
        REQUIRE(w.as_string() == "test");
    }

    SECTION("append string_view not null-terminated") {
        Writer w;
        std::string base = "hello world";
        std::string_view sv(base.data(), 5);  // "hello" without null terminator
        w.append(sv);
        REQUIRE(w.as_string() == "hello");
    }

    SECTION("append_repeated char") {
        Writer w;
        w.append_repeated('=', 5);
        REQUIRE(w.as_string() == "=====");
    }

    SECTION("append_repeated string") {
        Writer w;
        w.append_repeated("ab", 3);
        REQUIRE(w.as_string() == "ababab");
    }

    SECTION("append_formatted") {
        Writer w;
        w.append_formatted("Value: {}", 42);
        REQUIRE(w.as_string() == "Value: 42");
    }

    SECTION("newline") {
        Writer w;
        w.append("line1");
        w.newline();
        w.append("line2");
        REQUIRE(w.as_string() == "line1\r\nline2");
    }

    SECTION("clear") {
        Writer w;
        w.append("something");
        REQUIRE_FALSE(w.empty());
        w.clear();
        REQUIRE(w.empty());
        REQUIRE(w.as_string() == "");
    }

    SECTION("copy constructor") {
        Writer w1;
        w1.append("original");
        Writer w2(w1);
        REQUIRE(w2.as_string() == "original");
        w1.append(" modified");
        REQUIRE(w2.as_string() == "original");  // w2 should be independent
    }

    SECTION("move constructor") {
        Writer w1;
        w1.append("moveme");
        Writer w2(std::move(w1));
        REQUIRE(w2.as_string() == "moveme");
    }

    SECTION("grows beyond builtin buffer") {
        Writer w;
        // Builtin buffer is 1024, so write more than that
        for (int i = 0; i < 200; ++i) {
            w.append("0123456789");  // 10 chars each, 2000 total
        }
        auto result = w.as_string();
        REQUIRE(result.size() == 2000);
    }
}

TEST_CASE("string::Expander", "[string_expander]") {
    using pnq::string::Expander;

    SECTION("no variables") {
        Expander e;
        REQUIRE(e.expand("hello world") == "hello world");
    }

    SECTION("empty string") {
        Expander e;
        REQUIRE(e.expand("") == "");
    }

    SECTION("environment variable") {
        Expander e;
        auto result = e.expand("%PATH%");
        REQUIRE_FALSE(result.empty());
        REQUIRE(result != "%PATH%");  // Should be expanded
    }

    SECTION("escaped percent") {
        Expander e;
        REQUIRE(e.expand("100%%") == "100%");
    }

    SECTION("custom variables") {
        std::unordered_map<std::string, std::string> vars{
            {"NAME", "World"},
            {"GREETING", "Hello"}
        };
        Expander e(vars);
        REQUIRE(e.expand("%GREETING%, %NAME%!") == "Hello, World!");
    }

    SECTION("unknown variable unchanged") {
        std::unordered_map<std::string, std::string> vars;
        Expander e(vars, false);  // disable env vars
        REQUIRE(e.expand("%UNKNOWN%") == "%UNKNOWN%");
    }

    SECTION("custom variable overrides env") {
        std::unordered_map<std::string, std::string> vars{
            {"PATH", "custom_path"}
        };
        Expander e(vars);
        REQUIRE(e.expand("%PATH%") == "custom_path");
    }

    SECTION("dollar syntax disabled by default") {
        std::unordered_map<std::string, std::string> vars{{"NAME", "World"}};
        Expander e(vars, false);
        REQUIRE(e.expand("${NAME}") == "${NAME}");  // not expanded
    }

    SECTION("dollar syntax enabled") {
        std::unordered_map<std::string, std::string> vars{{"NAME", "World"}};
        Expander e(vars, false);
        e.expand_dollar(true);
        REQUIRE(e.expand("Hello ${NAME}!") == "Hello World!");
    }

    SECTION("escaped dollar") {
        std::unordered_map<std::string, std::string> vars;
        Expander e(vars, false);
        e.expand_dollar(true);
        REQUIRE(e.expand("Cost: $$100") == "Cost: $100");
    }

    SECTION("both percent and dollar") {
        std::unordered_map<std::string, std::string> vars{
            {"A", "alpha"},
            {"B", "beta"}
        };
        Expander e(vars, false);
        e.expand_dollar(true).expand_percent(true);
        REQUIRE(e.expand("%A% and ${B}") == "alpha and beta");
    }

    SECTION("disable percent, enable dollar") {
        std::unordered_map<std::string, std::string> vars{
            {"VAR", "value"}
        };
        Expander e(vars, false);
        e.expand_percent(false).expand_dollar(true);
        REQUIRE(e.expand("%VAR% ${VAR}") == "%VAR% value");
    }

    SECTION("dollar without braces unchanged") {
        std::unordered_map<std::string, std::string> vars{{"NAME", "test"}};
        Expander e(vars, false);
        e.expand_dollar(true);
        REQUIRE(e.expand("$NAME") == "$NAME");  // only ${NAME} works
    }

    SECTION("unclosed dollar brace") {
        std::unordered_map<std::string, std::string> vars;
        Expander e(vars, false);
        e.expand_dollar(true);
        REQUIRE(e.expand("${UNCLOSED") == "${UNCLOSED");
    }

    SECTION("unclosed percent") {
        std::unordered_map<std::string, std::string> vars;
        Expander e(vars, false);
        REQUIRE(e.expand("%UNCLOSED") == "%UNCLOSED");
    }

    SECTION("unknown dollar variable unchanged") {
        std::unordered_map<std::string, std::string> vars;
        Expander e(vars, false);
        e.expand_dollar(true);
        REQUIRE(e.expand("${UNKNOWN}") == "${UNKNOWN}");
    }

    SECTION("fluent interface chaining") {
        std::unordered_map<std::string, std::string> vars{{"X", "y"}};
        auto result = Expander(vars, false)
            .expand_dollar(true)
            .expand_percent(true)
            .expand("${X}%X%");
        REQUIRE(result == "yy");
    }
}

// Test helper class for ref counting
class TestRefCounted : public pnq::RefCountImpl
{
public:
    static int instance_count;
    TestRefCounted() { ++instance_count; }
    ~TestRefCounted() { --instance_count; }
};
int TestRefCounted::instance_count = 0;

TEST_CASE("path::combine", "[path]") {
    namespace p = pnq::path;

    SECTION("combine two components") {
        REQUIRE(p::combine("C:\\Users", "test") == "C:\\Users\\test");
    }

    SECTION("combine multiple components") {
        REQUIRE(p::combine("C:", "Users", "test", "file.txt") == "C:\\Users\\test\\file.txt");
    }

    SECTION("handles .. navigation") {
        REQUIRE(p::combine("C:\\Users\\test", "..") == "C:\\Users");
        REQUIRE(p::combine("C:\\Users\\test", "..", "other") == "C:\\Users\\other");
    }
}

TEST_CASE("path::change_extension", "[path]") {
    namespace p = pnq::path;

    SECTION("change existing extension") {
        REQUIRE(p::change_extension("file.txt", ".doc") == "file.doc");
    }

    SECTION("add extension to file without one") {
        REQUIRE(p::change_extension("file", ".txt") == "file.txt");
    }

    SECTION("full path") {
        REQUIRE(p::change_extension("C:\\dir\\file.txt", ".doc") == "C:\\dir\\file.doc");
    }
}

TEST_CASE("path::find_executable", "[path]") {
    namespace p = pnq::path;

    SECTION("find cmd.exe") {
        std::string result;
        REQUIRE(p::find_executable("cmd", result));
        REQUIRE(result.find("cmd") != std::string::npos);
    }

    SECTION("find notepad") {
        std::string result;
        REQUIRE(p::find_executable("notepad", result));
        REQUIRE(result.find("notepad") != std::string::npos);
    }
}

TEST_CASE("path::normalize", "[path]") {
    namespace p = pnq::path;

    SECTION("expands environment variables") {
        auto result = p::normalize("%WINDIR%\\system32");
        REQUIRE(result.find("Windows") != std::string::npos);
        REQUIRE(result.find("system32") != std::string::npos);
        REQUIRE(result.find('%') == std::string::npos);
    }

    SECTION("expands builtin CD") {
        auto result = p::normalize("%CD%\\subdir");
        auto cd = pnq::directory::current();
        REQUIRE(result.starts_with(cd));
        REQUIRE(result.ends_with("\\subdir"));
    }

    SECTION("expands builtin APPDIR") {
        auto result = p::normalize("%APPDIR%");
        REQUIRE_FALSE(result.empty());
        REQUIRE(result.find('%') == std::string::npos);
    }

    SECTION("expands builtin SYSDIR") {
        auto result = p::normalize("%SYSDIR%");
        auto sysdir = pnq::directory::system();
        REQUIRE(result == sysdir);
    }

    SECTION("custom vars take priority") {
        std::unordered_map<std::string, std::string> vars{{"CUSTOM", "my_value"}};
        auto result = p::normalize("%CUSTOM%\\file.txt", vars);
        REQUIRE(result == "my_value\\file.txt");
    }

    SECTION("custom vars override builtins") {
        std::unordered_map<std::string, std::string> vars{{"CD", "C:\\Override"}};
        auto result = p::normalize("%CD%\\test", vars);
        REQUIRE(result == "C:\\Override\\test");
    }

    SECTION("normalizes forward slashes to backslashes") {
        auto result = p::normalize("C:/Users/test/file.txt");
        REQUIRE(result == "C:\\Users\\test\\file.txt");
    }

    SECTION("unknown vars preserved") {
        auto result = p::normalize("%UNKNOWN_VAR_XYZ%\\file");
        REQUIRE(result == "%UNKNOWN_VAR_XYZ%\\file");
    }

    SECTION("escaped percent") {
        auto result = p::normalize("100%% complete");
        REQUIRE(result == "100% complete");
    }
}

TEST_CASE("memory_view", "[memory_view]") {
    SECTION("construct from bytes vector") {
        pnq::bytes data{0x01, 0x02, 0x03, 0x04};
        pnq::memory_view view(data);
        REQUIRE(view.size() == 4);
        REQUIRE(view.data() == data.data());
        REQUIRE_FALSE(view.empty());
    }

    SECTION("construct from bytes vector with size limit") {
        pnq::bytes data{0x01, 0x02, 0x03, 0x04, 0x05};
        pnq::memory_view view(data, 3);
        REQUIRE(view.size() == 3);
    }

    SECTION("size limit clamped to vector size") {
        pnq::bytes data{0x01, 0x02};
        pnq::memory_view view(data, 100);
        REQUIRE(view.size() == 2);
    }

    SECTION("construct from raw pointer") {
        std::uint8_t raw[] = {0xAA, 0xBB, 0xCC};
        pnq::memory_view view(raw, 3);
        REQUIRE(view.size() == 3);
        REQUIRE(view.data()[0] == 0xAA);
    }

    SECTION("construct from string_view") {
        std::string_view text = "hello";
        pnq::memory_view view(text);
        REQUIRE(view.size() == 5);
        REQUIRE(view.data()[0] == 'h');
    }

    SECTION("empty view") {
        pnq::bytes empty_data;
        pnq::memory_view view(empty_data);
        REQUIRE(view.empty());
        REQUIRE(view.size() == 0);
    }

    SECTION("duplicate creates copy") {
        pnq::bytes original{0x01, 0x02, 0x03};
        pnq::memory_view view(original);
        auto copy = view.duplicate();
        REQUIRE(copy.size() == 3);
        REQUIRE(copy[0] == 0x01);
        REQUIRE(copy.data() != original.data());  // different memory
    }

    SECTION("equality - same content") {
        pnq::bytes a{0x01, 0x02, 0x03};
        pnq::bytes b{0x01, 0x02, 0x03};
        pnq::memory_view view_a(a);
        pnq::memory_view view_b(b);
        REQUIRE(view_a == view_b);
    }

    SECTION("equality - different content") {
        pnq::bytes a{0x01, 0x02, 0x03};
        pnq::bytes b{0x01, 0x02, 0x04};
        pnq::memory_view view_a(a);
        pnq::memory_view view_b(b);
        REQUIRE_FALSE(view_a == view_b);
    }

    SECTION("equality - different sizes") {
        pnq::bytes a{0x01, 0x02, 0x03};
        pnq::bytes b{0x01, 0x02};
        pnq::memory_view view_a(a);
        pnq::memory_view view_b(b);
        REQUIRE_FALSE(view_a == view_b);
    }

    SECTION("equality - same pointer") {
        pnq::bytes data{0x01, 0x02, 0x03};
        pnq::memory_view view_a(data);
        pnq::memory_view view_b(data);
        REQUIRE(view_a == view_b);
    }
}

TEST_CASE("file::get_extension", "[file]") {
    namespace f = pnq::file;

    SECTION("simple extension") {
        REQUIRE(std::string(f::get_extension("file.txt")) == ".txt");
    }

    SECTION("executable") {
        REQUIRE(std::string(f::get_extension("notepad.exe")) == ".exe");
    }

    SECTION("no extension") {
        REQUIRE(f::get_extension("README").empty());
    }

    SECTION("multiple dots") {
        REQUIRE(std::string(f::get_extension("archive.tar.gz")) == ".gz");
    }

    SECTION("path with extension") {
        REQUIRE(std::string(f::get_extension("C:\\Users\\test\\file.doc")) == ".doc");
    }

    SECTION("hidden file unix style") {
        REQUIRE(std::string(f::get_extension(".gitignore")) == ".gitignore");
    }
}

TEST_CASE("file::exists", "[file]") {
    namespace f = pnq::file;

    SECTION("existing file") {
        // cmd.exe should always exist on Windows
        REQUIRE(f::exists("C:\\Windows\\System32\\cmd.exe"));
    }

    SECTION("non-existing file") {
        REQUIRE_FALSE(f::exists("C:\\this_file_does_not_exist_12345.txt"));
    }

    SECTION("non-existing path") {
        REQUIRE_FALSE(f::exists("Z:\\non_existent_drive\\file.txt"));
    }
}

TEST_CASE("file::remove", "[file]") {
    namespace f = pnq::file;

    // Get temp directory
    wchar_t temp_path[MAX_PATH];
    GetTempPathW(MAX_PATH, temp_path);
    std::string temp_file = pnq::string::encode_as_utf8(temp_path) + "pnq_test_remove.txt";

    SECTION("remove existing file") {
        // Create a test file
        HANDLE h = CreateFileA(temp_file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        REQUIRE(h != INVALID_HANDLE_VALUE);
        CloseHandle(h);
        REQUIRE(f::exists(temp_file));

        // Remove it
        REQUIRE(f::remove(temp_file));
        REQUIRE_FALSE(f::exists(temp_file));
    }

    SECTION("remove non-existing file fails") {
        REQUIRE_FALSE(f::remove("C:\\this_file_does_not_exist_12345.txt"));
    }
}

TEST_CASE("directory::exists", "[directory]") {
    namespace d = pnq::directory;

    SECTION("Windows directory exists") {
        REQUIRE(d::exists("C:\\Windows"));
    }

    SECTION("System32 exists") {
        REQUIRE(d::exists("C:\\Windows\\System32"));
    }

    SECTION("non-existing directory") {
        REQUIRE_FALSE(d::exists("C:\\this_directory_does_not_exist_12345"));
    }

    SECTION("file is not a directory") {
        REQUIRE_FALSE(d::exists("C:\\Windows\\System32\\cmd.exe"));
    }
}

TEST_CASE("directory::system", "[directory]") {
    auto result = pnq::directory::system();
    REQUIRE_FALSE(result.empty());
    // Case-insensitive check - could be "System32" or "system32"
    auto lower = pnq::string::lowercase(result);
    REQUIRE(lower.find("system32") != std::string::npos);
}

TEST_CASE("directory::windows", "[directory]") {
    auto result = pnq::directory::windows();
    REQUIRE_FALSE(result.empty());
    REQUIRE(result.find("Windows") != std::string::npos);
}

TEST_CASE("directory::current", "[directory]") {
    auto result = pnq::directory::current();
    REQUIRE_FALSE(result.empty());
    // Should be a valid path with drive letter
    REQUIRE(result.size() >= 3);
    REQUIRE(result[1] == ':');
}

TEST_CASE("directory::application", "[directory]") {
    auto result = pnq::directory::application();
    REQUIRE_FALSE(result.empty());
    // Should be a valid directory
    REQUIRE(pnq::directory::exists(result));
}

TEST_CASE("RefCountImpl", "[ref_counted]") {
    SECTION("basic retain/release") {
        TestRefCounted::instance_count = 0;
        auto *obj = new TestRefCounted();
        REQUIRE(TestRefCounted::instance_count == 1);

        obj->retain();
        obj->retain();
        // ref count is now 3

        obj->release();
        obj->release();
        REQUIRE(TestRefCounted::instance_count == 1);  // still alive

        obj->release();  // should delete
        REQUIRE(TestRefCounted::instance_count == 0);
    }
}

TEST_CASE("environment_variables::get", "[environment_variables]") {
    namespace ev = pnq::environment_variables;

    SECTION("get PATH") {
        std::string value;
        REQUIRE(ev::get("PATH", value));
        REQUIRE_FALSE(value.empty());
    }

    SECTION("get WINDIR") {
        std::string value;
        REQUIRE(ev::get("WINDIR", value));
        REQUIRE(value.find("Windows") != std::string::npos);
    }

    SECTION("get non-existent variable") {
        std::string value = "unchanged";
        REQUIRE_FALSE(ev::get("THIS_VAR_DOES_NOT_EXIST_12345", value));
        REQUIRE(value == "unchanged");  // value should not be modified
    }

    SECTION("get USERNAME") {
        std::string value;
        REQUIRE(ev::get("USERNAME", value));
        REQUIRE_FALSE(value.empty());
    }
}

TEST_CASE("environment_variables::expand", "[string_expander]") {
    SECTION("expand PATH") {
        auto result = pnq::environment_variables::expand("%PATH%");
        REQUIRE_FALSE(result.empty());
    }

    SECTION("expand with custom vars") {
        std::unordered_map<std::string, std::string> vars{{"FOO", "bar"}};
        auto result = pnq::environment_variables::expand("%FOO%", vars);
        REQUIRE(result == "bar");
    }
}

TEST_CASE("string::multiply", "[string_writer]") {
    SECTION("multiply char") {
        REQUIRE(pnq::string::multiply('x', 5) == "xxxxx");
        REQUIRE(pnq::string::multiply('a', 0) == "");
    }

    SECTION("multiply string") {
        REQUIRE(pnq::string::multiply("ab", 3) == "ababab");
        REQUIRE(pnq::string::multiply("test", 0) == "");
    }
}

TEST_CASE("wstring::equals_nocase", "[wstring]") {
    using pnq::wstring::equals_nocase;

    SECTION("equal strings, same case") {
        REQUIRE(equals_nocase(L"hello", L"hello"));
    }

    SECTION("equal strings, different case") {
        REQUIRE(equals_nocase(L"Hello", L"hELLO"));
        REQUIRE(equals_nocase(L"HELLO", L"hello"));
    }

    SECTION("different strings") {
        REQUIRE_FALSE(equals_nocase(L"hello", L"world"));
        REQUIRE_FALSE(equals_nocase(L"hello", L"hell"));
    }

    SECTION("empty strings") {
        REQUIRE(equals_nocase(L"", L""));
        REQUIRE_FALSE(equals_nocase(L"", L"a"));
        REQUIRE_FALSE(equals_nocase(L"a", L""));
    }

    SECTION("different lengths") {
        REQUIRE_FALSE(equals_nocase(L"hello", L"hello!"));
        REQUIRE_FALSE(equals_nocase(L"hello!", L"hello"));
    }
}
