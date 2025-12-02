#include <catch2/catch_test_macros.hpp>
#include <pnq/pnq.h>
#include <pnq/regis3.h>

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

TEST_CASE("string::strip", "[string]") {
    using pnq::string::strip;
    using pnq::string::lstrip;
    using pnq::string::rstrip;

    SECTION("strip both sides") {
        REQUIRE(std::string(strip("  hello  ")) == "hello");
        REQUIRE(std::string(strip("\t\nhello\r\n")) == "hello");
        REQUIRE(std::string(strip("   ")) == "");
        REQUIRE(std::string(strip("")) == "");
        REQUIRE(std::string(strip("hello")) == "hello");
    }

    SECTION("lstrip") {
        REQUIRE(std::string(lstrip("  hello  ")) == "hello  ");
        REQUIRE(std::string(lstrip("\t\nhello")) == "hello");
        REQUIRE(std::string(lstrip("   ")) == "");
        REQUIRE(std::string(lstrip("hello")) == "hello");
    }

    SECTION("rstrip") {
        REQUIRE(std::string(rstrip("  hello  ")) == "  hello");
        REQUIRE(std::string(rstrip("hello\r\n")) == "hello");
        REQUIRE(std::string(rstrip("   ")) == "");
        REQUIRE(std::string(rstrip("hello")) == "hello");
    }

    SECTION("custom chars") {
        REQUIRE(std::string(strip("xxhelloxx", "x")) == "hello");
        REQUIRE(std::string(lstrip("##value", "#")) == "value");
        REQUIRE(std::string(rstrip("value##", "#")) == "value");
    }
}

TEST_CASE("string::split_stripped", "[string]") {
    using pnq::string::split_stripped;

    SECTION("basic split with stripping") {
        auto result = split_stripped("a , b , c", ",");
        REQUIRE(result.size() == 3);
        REQUIRE(result[0] == "a");
        REQUIRE(result[1] == "b");
        REQUIRE(result[2] == "c");
    }

    SECTION("mixed whitespace") {
        auto result = split_stripped("  foo  ;\t bar \t; baz  ", ";");
        REQUIRE(result.size() == 3);
        REQUIRE(result[0] == "foo");
        REQUIRE(result[1] == "bar");
        REQUIRE(result[2] == "baz");
    }

    SECTION("empty elements become empty strings") {
        auto result = split_stripped("a ,  , c", ",");
        REQUIRE(result.size() == 3);
        REQUIRE(result[0] == "a");
        REQUIRE(result[1] == "");
        REQUIRE(result[2] == "c");
    }

    SECTION("custom strip chars") {
        auto result = split_stripped("xax,xbx,xcx", ",", false, "x");
        REQUIRE(result.size() == 3);
        REQUIRE(result[0] == "a");
        REQUIRE(result[1] == "b");
        REQUIRE(result[2] == "c");
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

TEST_CASE("file::match", "[file]") {
    namespace f = pnq::file;

    SECTION("exact match") {
        REQUIRE(f::match("hello", "hello"));
        REQUIRE_FALSE(f::match("hello", "world"));
    }

    SECTION("case insensitive") {
        REQUIRE(f::match("Hello", "HELLO"));
        REQUIRE(f::match("WORLD", "world"));
        REQUIRE(f::match("Test.TXT", "test.txt"));
    }

    SECTION("? matches single character") {
        REQUIRE(f::match("h?llo", "hello"));
        REQUIRE(f::match("h?llo", "hallo"));
        REQUIRE_FALSE(f::match("h?llo", "hllo"));
        REQUIRE_FALSE(f::match("h?llo", "heello"));
    }

    SECTION("* matches any sequence") {
        REQUIRE(f::match("*.txt", "file.txt"));
        REQUIRE(f::match("*.txt", "document.txt"));
        REQUIRE(f::match("file.*", "file.exe"));
        REQUIRE(f::match("*", "anything"));
        REQUIRE(f::match("*", ""));
    }

    SECTION("* matches zero characters") {
        REQUIRE(f::match("a*b", "ab"));
        REQUIRE(f::match("*test", "test"));
        REQUIRE(f::match("test*", "test"));
    }

    SECTION("multiple wildcards") {
        REQUIRE(f::match("*.tar.gz", "archive.tar.gz"));
        REQUIRE(f::match("*.*.*", "a.b.c"));
        REQUIRE(f::match("a*b*c", "aXXbYYc"));
        REQUIRE(f::match("a*b*c", "abc"));
    }

    SECTION("combined ? and *") {
        REQUIRE(f::match("?est*", "test.txt"));
        REQUIRE(f::match("*.??", "file.cc"));
        REQUIRE_FALSE(f::match("*.??", "file.cpp"));
    }

    SECTION("empty patterns and text") {
        REQUIRE(f::match("", ""));
        REQUIRE_FALSE(f::match("", "text"));
        REQUIRE_FALSE(f::match("pattern", ""));
    }

    SECTION("trailing stars consumed") {
        REQUIRE(f::match("test***", "test"));
        REQUIRE(f::match("a*****", "abc"));
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

TEST_CASE("RefCountedVector", "[ref_counted]") {
    using Vec = pnq::RefCountedVector<TestRefCounted*>;

    SECTION("empty vector") {
        TestRefCounted::instance_count = 0;
        {
            Vec v;
            REQUIRE(v.empty());
            REQUIRE(v.size() == 0);
        }
        REQUIRE(TestRefCounted::instance_count == 0);
    }

    SECTION("push_back retains, destructor releases") {
        TestRefCounted::instance_count = 0;
        auto* obj = new TestRefCounted();
        REQUIRE(TestRefCounted::instance_count == 1);

        {
            Vec v;
            v.push_back(obj);
            REQUIRE(v.size() == 1);
            REQUIRE(v[0] == obj);

            obj->release();  // release our reference
            REQUIRE(TestRefCounted::instance_count == 1);  // vector still holds it
        }
        REQUIRE(TestRefCounted::instance_count == 0);  // vector released it
    }

    SECTION("pop_back releases") {
        TestRefCounted::instance_count = 0;
        auto* obj = new TestRefCounted();

        Vec v;
        v.push_back(obj);
        obj->release();
        REQUIRE(TestRefCounted::instance_count == 1);

        v.pop_back();
        REQUIRE(v.empty());
        REQUIRE(TestRefCounted::instance_count == 0);
    }

    SECTION("clear releases all") {
        TestRefCounted::instance_count = 0;
        auto* a = new TestRefCounted();
        auto* b = new TestRefCounted();

        Vec v;
        v.push_back(a);
        v.push_back(b);
        a->release();
        b->release();
        REQUIRE(TestRefCounted::instance_count == 2);

        v.clear();
        REQUIRE(v.empty());
        REQUIRE(TestRefCounted::instance_count == 0);
    }

    SECTION("copy constructor retains") {
        TestRefCounted::instance_count = 0;
        auto* obj = new TestRefCounted();

        Vec v1;
        v1.push_back(obj);
        obj->release();
        REQUIRE(TestRefCounted::instance_count == 1);

        {
            Vec v2(v1);
            REQUIRE(v2.size() == 1);
            REQUIRE(v2[0] == obj);
            REQUIRE(TestRefCounted::instance_count == 1);  // same object, refcount=2
        }
        REQUIRE(TestRefCounted::instance_count == 1);  // v1 still holds it
    }

    SECTION("copy assignment retains and releases") {
        TestRefCounted::instance_count = 0;
        auto* a = new TestRefCounted();
        auto* b = new TestRefCounted();

        Vec v1, v2;
        v1.push_back(a);
        v2.push_back(b);
        a->release();
        b->release();
        REQUIRE(TestRefCounted::instance_count == 2);

        v2 = v1;
        REQUIRE(TestRefCounted::instance_count == 1);  // b was released
        REQUIRE(v2[0] == a);
    }

    SECTION("move constructor transfers ownership") {
        TestRefCounted::instance_count = 0;
        auto* obj = new TestRefCounted();

        Vec v1;
        v1.push_back(obj);
        obj->release();

        Vec v2(std::move(v1));
        REQUIRE(v2.size() == 1);
        REQUIRE(v1.empty());
        REQUIRE(TestRefCounted::instance_count == 1);
    }

    SECTION("move assignment transfers ownership") {
        TestRefCounted::instance_count = 0;
        auto* a = new TestRefCounted();
        auto* b = new TestRefCounted();

        Vec v1, v2;
        v1.push_back(a);
        v2.push_back(b);
        a->release();
        b->release();

        v2 = std::move(v1);
        REQUIRE(v1.empty());
        REQUIRE(v2[0] == a);
        REQUIRE(TestRefCounted::instance_count == 1);  // b was released
    }

    SECTION("iteration") {
        TestRefCounted::instance_count = 0;
        auto* a = new TestRefCounted();
        auto* b = new TestRefCounted();

        Vec v;
        v.push_back(a);
        v.push_back(b);
        a->release();
        b->release();

        int count = 0;
        for (auto* p : v) {
            REQUIRE(p != nullptr);
            ++count;
        }
        REQUIRE(count == 2);
    }

    SECTION("at with bounds check") {
        TestRefCounted::instance_count = 0;
        auto* obj = new TestRefCounted();

        Vec v;
        v.push_back(obj);
        obj->release();

        REQUIRE(v.at(0) == obj);
        REQUIRE_THROWS_AS(v.at(1), std::out_of_range);
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

// =============================================================================
// registry::value tests
// =============================================================================

TEST_CASE("registry::value basics", "[registry]") {
    using pnq::regis3::value;

    SECTION("default constructor") {
        value v;
        REQUIRE(v.is_default_value());
        REQUIRE(v.type() == pnq::regis3::REG_TYPE_UNKNOWN);
        REQUIRE_FALSE(v.remove_flag());
    }

    SECTION("named value") {
        value v("TestValue");
        REQUIRE_FALSE(v.is_default_value());
        REQUIRE(v.name() == "TestValue");
    }

    SECTION("set/get DWORD") {
        value v("DwordVal");
        v.set_dword(0x12345678);
        REQUIRE(v.type() == REG_DWORD);
        REQUIRE(v.get_dword() == 0x12345678);
        REQUIRE(v.get_dword(99) == 0x12345678);
    }

    SECTION("get DWORD with wrong type returns default") {
        value v("StringVal");
        v.set_string("hello");
        REQUIRE(v.get_dword(42) == 42);
    }

    SECTION("set/get QWORD") {
        value v("QwordVal");
        v.set_qword(0x123456789ABCDEF0ULL);
        REQUIRE(v.type() == REG_QWORD);
        REQUIRE(v.get_qword() == 0x123456789ABCDEF0ULL);
    }

    SECTION("set/get string") {
        value v("StringVal");
        v.set_string("Hello World");
        REQUIRE(v.type() == REG_SZ);
        REQUIRE(v.get_string() == "Hello World");
    }

    SECTION("set/get expanded string") {
        value v("ExpandVal");
        v.set_expanded_string("%WINDIR%\\system32");
        REQUIRE(v.type() == REG_EXPAND_SZ);
        REQUIRE(v.get_string() == "%WINDIR%\\system32");
    }

    SECTION("set/get multi-string") {
        value v("MultiVal");
        std::vector<std::string> strings{"one", "two", "three"};
        v.set_multi_string(strings);
        REQUIRE(v.type() == REG_MULTI_SZ);

        auto result = v.get_multi_string();
        REQUIRE(result.size() == 3);
        REQUIRE(result[0] == "one");
        REQUIRE(result[1] == "two");
        REQUIRE(result[2] == "three");
    }

    SECTION("set none") {
        value v("NoneVal");
        v.set_none();
        REQUIRE(v.type() == REG_NONE);
        REQUIRE(v.get_binary().empty());
    }

    SECTION("remove flag") {
        value v("ToRemove");
        REQUIRE_FALSE(v.remove_flag());
        v.set_remove_flag(true);
        REQUIRE(v.remove_flag());
    }

    SECTION("copy constructor") {
        value v1("Original");
        v1.set_dword(100);
        value v2(v1);
        REQUIRE(v2.name() == "Original");
        REQUIRE(v2.get_dword() == 100);
    }

    SECTION("unicode string") {
        value v("UnicodeVal");
        v.set_string("Héllo Wörld 日本語");
        REQUIRE(v.get_string() == "Héllo Wörld 日本語");
    }
}

TEST_CASE("registry::key_entry basics", "[registry]") {
    using pnq::regis3::key_entry;
    using pnq::regis3::value;

    SECTION("default constructor creates root") {
        key_entry* root = PNQ_NEW key_entry();
        REQUIRE(root->name().empty());
        REQUIRE(root->parent() == nullptr);
        REQUIRE(root->get_path().empty());
        root->release();
    }

    SECTION("named key with parent") {
        key_entry* root = PNQ_NEW key_entry();
        key_entry* child = PNQ_NEW key_entry(root, "Software");

        REQUIRE(child->name() == "Software");
        REQUIRE(child->parent() == root);
        REQUIRE(child->get_path() == "Software");

        child->release();
        root->release();
    }

    SECTION("find_or_create_key creates hierarchy") {
        key_entry* root = PNQ_NEW key_entry();

        key_entry* deep = root->find_or_create_key("HKEY_LOCAL_MACHINE\\SOFTWARE\\MyApp\\Settings");

        REQUIRE(deep->name() == "Settings");
        REQUIRE(deep->get_path() == "HKEY_LOCAL_MACHINE\\SOFTWARE\\MyApp\\Settings");

        // Same path returns same key
        key_entry* same = root->find_or_create_key("HKEY_LOCAL_MACHINE\\SOFTWARE\\MyApp\\Settings");
        REQUIRE(same == deep);

        // Case-insensitive lookup
        key_entry* also_same = root->find_or_create_key("hkey_local_machine\\software\\myapp\\settings");
        REQUIRE(also_same == deep);

        root->release();
    }

    SECTION("find_or_create_key with remove flag") {
        key_entry* root = PNQ_NEW key_entry();

        key_entry* removed = root->find_or_create_key("-HKEY_CURRENT_USER\\DeleteMe");

        REQUIRE(removed->remove_flag() == true);
        REQUIRE(removed->name() == "DeleteMe");

        root->release();
    }

    SECTION("find_or_create_value") {
        key_entry* root = PNQ_NEW key_entry();
        key_entry* key = root->find_or_create_key("TestKey");

        // Named value
        value* v1 = key->find_or_create_value("TestValue");
        REQUIRE(v1->name() == "TestValue");
        REQUIRE_FALSE(v1->is_default_value());

        // Same value returned on second call
        value* v1_again = key->find_or_create_value("TestValue");
        REQUIRE(v1 == v1_again);

        // Case-insensitive
        value* v1_case = key->find_or_create_value("testvalue");
        REQUIRE(v1 == v1_case);

        // Default value
        value* def = key->find_or_create_value("");
        REQUIRE(def->is_default_value());
        REQUIRE(key->default_value() == def);

        root->release();
    }

    SECTION("clone creates deep copy") {
        key_entry* root = PNQ_NEW key_entry();
        key_entry* orig = root->find_or_create_key("Original\\Subkey");

        value* v = orig->find_or_create_value("MyValue");
        v->set_dword(42);

        key_entry* cloned = orig->clone(nullptr);

        // Clone is independent
        REQUIRE(cloned->name() == "Subkey");
        REQUIRE(cloned->parent() == nullptr);  // we passed nullptr as parent

        // Values are copied
        REQUIRE(cloned->values().size() == 1);
        auto it = cloned->values().find("myvalue");
        REQUIRE(it != cloned->values().end());
        REQUIRE(it->second->get_dword() == 42);

        // Modifying clone doesn't affect original
        it->second->set_dword(100);
        REQUIRE(v->get_dword() == 42);

        cloned->release();
        root->release();
    }

    SECTION("has_values and has_keys") {
        key_entry* root = PNQ_NEW key_entry();

        REQUIRE_FALSE(root->has_values());
        REQUIRE_FALSE(root->has_keys());

        root->find_or_create_key("Child");
        REQUIRE(root->has_keys());
        REQUIRE_FALSE(root->has_values());

        key_entry* child = root->find_or_create_key("Child");
        child->find_or_create_value("Val");
        REQUIRE(child->has_values());

        root->release();
    }

    SECTION("ask_to_add_value and ask_to_remove_value") {
        key_entry* source = PNQ_NEW key_entry();
        key_entry* src_key = source->find_or_create_key("Source\\Key");
        value* src_val = src_key->find_or_create_value("MyVal");
        src_val->set_string("hello");

        key_entry* diff = PNQ_NEW key_entry();

        // Add value
        diff->ask_to_add_value(src_key, src_val);

        key_entry* diff_key = diff->find_or_create_key("Source\\Key");
        REQUIRE(diff_key->values().size() == 1);
        REQUIRE(diff_key->values().at("myval")->get_string() == "hello");
        REQUIRE_FALSE(diff_key->values().at("myval")->remove_flag());

        // Remove value
        key_entry* diff2 = PNQ_NEW key_entry();
        diff2->ask_to_remove_value(src_key, src_val);

        key_entry* diff2_key = diff2->find_or_create_key("Source\\Key");
        REQUIRE(diff2_key->values().at("myval")->remove_flag());

        diff2->release();
        diff->release();
        source->release();
    }
}

TEST_CASE("registry::key live access", "[registry]") {
    using pnq::regis3::key;
    using pnq::regis3::value;

    // Use HKCU\Software for testing - should be writable without elevation
    const std::string test_path = "HKEY_CURRENT_USER\\Software\\pnq_test_" + std::to_string(GetCurrentProcessId());

    SECTION("parse_hive") {
        std::string relative;

        HKEY hive = pnq::regis3::parse_hive("HKEY_LOCAL_MACHINE\\SOFTWARE\\Test", relative);
        REQUIRE(hive == HKEY_LOCAL_MACHINE);
        REQUIRE(relative == "SOFTWARE\\Test");

        hive = pnq::regis3::parse_hive("HKLM\\Test", relative);
        REQUIRE(hive == HKEY_LOCAL_MACHINE);
        REQUIRE(relative == "Test");

        hive = pnq::regis3::parse_hive("HKCU", relative);
        REQUIRE(hive == HKEY_CURRENT_USER);
        REQUIRE(relative.empty());
    }

    SECTION("open for reading existing key") {
        key k("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion");
        REQUIRE(k.open_for_reading());
        REQUIRE(k.is_open());

        // Read a known value
        std::string prog_files = k.get_string("ProgramFilesDir");
        REQUIRE_FALSE(prog_files.empty());
        REQUIRE(prog_files.find("Program Files") != std::string::npos);
    }

    SECTION("open nonexistent key fails") {
        key k("HKEY_LOCAL_MACHINE\\SOFTWARE\\ThisKeyDoesNotExist_12345");
        REQUIRE_FALSE(k.open_for_reading());
    }

    SECTION("write and read back values") {
        // Create test key
        key k(test_path);
        REQUIRE(k.open_for_writing());

        // Write values
        REQUIRE(k.set_string("TestString", "Hello World"));
        REQUIRE(k.set_dword("TestDword", 0x12345678));

        // Read them back
        REQUIRE(k.get_string("TestString") == "Hello World");
        REQUIRE(k.get_dword("TestDword") == 0x12345678);

        // Clean up
        k.close();
        REQUIRE(key::delete_recursive(test_path));
    }

    SECTION("write and delete value") {
        key k(test_path);
        REQUIRE(k.open_for_writing());

        // Write a value
        REQUIRE(k.set_string("ToDelete", "delete me"));
        REQUIRE(k.get_string("ToDelete") == "delete me");

        // Delete it via remove_flag
        value v("ToDelete");
        v.set_string("ignored");
        v.set_remove_flag(true);
        REQUIRE(k.set("ToDelete", v));

        // Should be gone
        REQUIRE(k.get_string("ToDelete", "default") == "default");

        k.close();
        REQUIRE(key::delete_recursive(test_path));
    }

    SECTION("enumerate values") {
        key k(test_path);
        REQUIRE(k.open_for_writing());

        k.set_string("Val1", "one");
        k.set_string("Val2", "two");
        k.set_dword("Val3", 3);

        int count = 0;
        for (const auto& v : k.enum_values()) {
            ++count;
            // Check that values are readable
            REQUIRE_FALSE(v.name().empty());
        }
        REQUIRE(count == 3);

        k.close();
        REQUIRE(key::delete_recursive(test_path));
    }

    SECTION("enumerate subkeys") {
        key parent(test_path);
        REQUIRE(parent.open_for_writing());

        // Create subkeys
        key sub1(test_path + "\\SubKey1");
        REQUIRE(sub1.open_for_writing());
        sub1.set_string("Val", "test");

        key sub2(test_path + "\\SubKey2");
        REQUIRE(sub2.open_for_writing());
        sub2.set_string("Val", "test");

        int count = 0;
        for (const auto& subkey_path : parent.enum_keys()) {
            ++count;
            REQUIRE(subkey_path.find("SubKey") != std::string::npos);
        }
        REQUIRE(count == 2);

        sub1.close();
        sub2.close();
        parent.close();
        REQUIRE(key::delete_recursive(test_path));
    }
}

TEST_CASE("registry::regfile_parser", "[registry]") {
    using pnq::regis3::regfile_parser;
    using pnq::regis3::key_entry;
    using pnq::regis3::import_options;

    SECTION("parse REGEDIT4 format") {
        const char* content =
            "REGEDIT4\r\n"
            "\r\n"
            "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Test]\r\n"
            "\"StringValue\"=\"Hello World\"\r\n"
            "\"DwordValue\"=dword:00001234\r\n"
            "\r\n";

        regfile_parser parser("REGEDIT4", import_options::none);
        REQUIRE(parser.parse_text(content));

        key_entry* result = parser.get_result();
        REQUIRE(result != nullptr);
        REQUIRE(result->get_path() == "HKEY_LOCAL_MACHINE\\SOFTWARE\\Test");

        // Check values
        auto it = result->values().find("stringvalue");
        REQUIRE(it != result->values().end());
        REQUIRE(it->second->get_string() == "Hello World");

        it = result->values().find("dwordvalue");
        REQUIRE(it != result->values().end());
        REQUIRE(it->second->get_dword() == 0x1234);

        result->release();
    }

    SECTION("parse Windows Registry Editor 5.00 format") {
        const char* content =
            "Windows Registry Editor Version 5.00\r\n"
            "\r\n"
            "[HKEY_CURRENT_USER\\Software\\MyApp]\r\n"
            "@=\"Default Value\"\r\n"
            "\"Name\"=\"Test\"\r\n"
            "\r\n";

        regfile_parser parser("Windows Registry Editor Version 5.00", import_options::none);
        REQUIRE(parser.parse_text(content));

        key_entry* result = parser.get_result();
        REQUIRE(result != nullptr);
        REQUIRE(result->default_value() != nullptr);
        REQUIRE(result->default_value()->get_string() == "Default Value");

        result->release();
    }

    SECTION("parse multi-line hex value") {
        const char* content =
            "REGEDIT4\r\n"
            "\r\n"
            "[HKEY_LOCAL_MACHINE\\Test]\r\n"
            "\"Binary\"=hex:01,02,03,04,\\\r\n"
            "  05,06,07,08\r\n"
            "\r\n";

        regfile_parser parser("REGEDIT4", import_options::none);
        REQUIRE(parser.parse_text(content));

        key_entry* result = parser.get_result();
        REQUIRE(result != nullptr);

        auto it = result->values().find("binary");
        REQUIRE(it != result->values().end());

        auto& data = it->second->get_binary();
        REQUIRE(data.size() == 8);
        REQUIRE(data[0] == 0x01);
        REQUIRE(data[7] == 0x08);

        result->release();
    }

    SECTION("parse hex(7) multi-string") {
        const char* content =
            "REGEDIT4\r\n"
            "\r\n"
            "[HKEY_LOCAL_MACHINE\\Test]\r\n"
            "\"MultiSz\"=hex(7):4f,00,6e,00,65,00,00,00,54,00,77,00,6f,00,00,00,00,00\r\n"
            "\r\n";

        regfile_parser parser("REGEDIT4", import_options::none);
        REQUIRE(parser.parse_text(content));

        key_entry* result = parser.get_result();
        auto it = result->values().find("multisz");
        REQUIRE(it != result->values().end());
        REQUIRE(it->second->type() == REG_MULTI_SZ);

        result->release();
    }

    SECTION("parse with escaped strings") {
        const char* content =
            "REGEDIT4\r\n"
            "\r\n"
            "[HKEY_LOCAL_MACHINE\\Test]\r\n"
            "\"Path\"=\"C:\\\\Windows\\\\System32\"\r\n"
            "\"Quote\"=\"Say \\\"Hello\\\"\"\r\n"
            "\r\n";

        regfile_parser parser("REGEDIT4", import_options::none);
        REQUIRE(parser.parse_text(content));

        key_entry* result = parser.get_result();

        auto it = result->values().find("path");
        REQUIRE(it != result->values().end());
        REQUIRE(it->second->get_string() == "C:\\Windows\\System32");

        it = result->values().find("quote");
        REQUIRE(it != result->values().end());
        REQUIRE(it->second->get_string() == "Say \"Hello\"");

        result->release();
    }

    SECTION("parse remove key syntax") {
        const char* content =
            "REGEDIT4\r\n"
            "\r\n"
            "[-HKEY_LOCAL_MACHINE\\DeleteMe]\r\n"
            "\r\n";

        regfile_parser parser("REGEDIT4", import_options::none);
        REQUIRE(parser.parse_text(content));

        key_entry* result = parser.get_result();
        REQUIRE(result != nullptr);
        REQUIRE(result->remove_flag() == true);

        result->release();
    }

    SECTION("parse with comments") {
        const char* content =
            "REGEDIT4\r\n"
            "; This is a comment\r\n"
            "[HKEY_LOCAL_MACHINE\\Test]\r\n"
            "# Another comment\r\n"
            "\"Value\"=\"Test\"\r\n"
            "\r\n";

        regfile_parser parser("REGEDIT4",
            import_options::allow_semicolon_comments | import_options::allow_hashtag_comments);
        REQUIRE(parser.parse_text(content));

        key_entry* result = parser.get_result();
        REQUIRE(result != nullptr);

        result->release();
    }

    SECTION("invalid header fails") {
        const char* content =
            "INVALID HEADER\r\n"
            "[HKEY_LOCAL_MACHINE\\Test]\r\n";

        regfile_parser parser("REGEDIT4", import_options::none);
        REQUIRE_FALSE(parser.parse_text(content));
    }
}

TEST_CASE("registry::importer", "[registry]") {
    using namespace pnq::regis3;

    SECTION("format4 importer") {
        const char* content =
            "REGEDIT4\r\n"
            "\r\n"
            "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Test]\r\n"
            "\"StringValue\"=\"Hello\"\r\n"
            "\"DwordValue\"=dword:0000002a\r\n"
            "\r\n";

        regfile_format4_importer importer(content);
        key_entry* result = importer.import();
        REQUIRE(result != nullptr);
        REQUIRE(result->get_path() == "HKEY_LOCAL_MACHINE\\SOFTWARE\\Test");

        auto it = result->values().find("stringvalue");
        REQUIRE(it != result->values().end());
        REQUIRE(it->second->get_string() == "Hello");

        it = result->values().find("dwordvalue");
        REQUIRE(it != result->values().end());
        REQUIRE(it->second->get_dword() == 42);

        result->release();
    }

    SECTION("format5 importer") {
        const char* content =
            "Windows Registry Editor Version 5.00\r\n"
            "\r\n"
            "[HKEY_CURRENT_USER\\Software\\Test]\r\n"
            "@=\"Default\"\r\n"
            "\"Name\"=\"Value\"\r\n"
            "\r\n";

        regfile_format5_importer importer(content);
        key_entry* result = importer.import();
        REQUIRE(result != nullptr);
        REQUIRE(result->default_value()->get_string() == "Default");

        result->release();
    }

    SECTION("importer caches result") {
        const char* content =
            "REGEDIT4\r\n"
            "[HKEY_LOCAL_MACHINE\\Test]\r\n"
            "\"Val\"=\"Test\"\r\n";

        regfile_format4_importer importer(content);
        key_entry* result1 = importer.import();
        key_entry* result2 = importer.import();

        // Should be same object, but with refcount incremented
        REQUIRE(result1 == result2);

        result1->release();
        result2->release();
    }

    SECTION("create_importer_from_string auto-detects format4") {
        const char* content =
            "REGEDIT4\r\n"
            "[HKEY_LOCAL_MACHINE\\Test]\r\n";

        auto importer = create_importer_from_string(content);
        REQUIRE(importer != nullptr);

        key_entry* result = importer->import();
        REQUIRE(result != nullptr);
        result->release();
    }

    SECTION("create_importer_from_string auto-detects format5") {
        const char* content =
            "Windows Registry Editor Version 5.00\r\n"
            "[HKEY_LOCAL_MACHINE\\Test]\r\n";

        auto importer = create_importer_from_string(content);
        REQUIRE(importer != nullptr);

        key_entry* result = importer->import();
        REQUIRE(result != nullptr);
        result->release();
    }

    SECTION("create_importer_from_string handles UTF-8 BOM") {
        std::string content = "\xEF\xBB\xBF"  // UTF-8 BOM
            "Windows Registry Editor Version 5.00\r\n"
            "[HKEY_LOCAL_MACHINE\\Test]\r\n";

        auto importer = create_importer_from_string(content);
        REQUIRE(importer != nullptr);

        key_entry* result = importer->import();
        REQUIRE(result != nullptr);
        result->release();
    }

    SECTION("create_importer_from_string returns null for unknown format") {
        auto importer = create_importer_from_string("Not a valid header");
        REQUIRE(importer == nullptr);
    }

    SECTION("registry_importer reads live registry") {
        // Create a simple test key first
        const std::string test_path = "HKEY_CURRENT_USER\\Software\\pnq_reg_importer_test_" + std::to_string(GetCurrentProcessId());

        key test_key(test_path);
        REQUIRE(test_key.open_for_writing());
        test_key.set_string("TestVal", "TestData");
        test_key.close();

        // Now import it
        registry_importer importer(test_path);
        key_entry* result = importer.import();
        REQUIRE(result != nullptr);

        auto it = result->values().find("testval");
        REQUIRE(it != result->values().end());
        REQUIRE(it->second->get_string() == "TestData");

        result->release();
        key::delete_recursive(test_path);
    }

    SECTION("registry_importer handles non-existent key") {
        registry_importer importer("HKEY_LOCAL_MACHINE\\SOFTWARE\\ThisDoesNotExist_12345");
        key_entry* result = importer.import();
        REQUIRE(result != nullptr);  // Returns empty tree, not null

        // Should be empty
        REQUIRE_FALSE(result->has_values());
        REQUIRE_FALSE(result->has_keys());

        result->release();
    }

    SECTION("registry_importer reads subkeys recursively") {
        // Use our test key
        const std::string test_path = "HKEY_CURRENT_USER\\Software\\pnq_importer_test_" + std::to_string(GetCurrentProcessId());

        // Create test structure
        key parent(test_path);
        REQUIRE(parent.open_for_writing());
        parent.set_string("ParentVal", "parent");

        key child(test_path + "\\Child");
        REQUIRE(child.open_for_writing());
        child.set_string("ChildVal", "child");

        child.close();
        parent.close();

        // Now import
        registry_importer importer(test_path);
        key_entry* result = importer.import();
        REQUIRE(result != nullptr);

        // Check parent value
        auto it = result->values().find("parentval");
        REQUIRE(it != result->values().end());
        REQUIRE(it->second->get_string() == "parent");

        // Check child key exists
        REQUIRE(result->has_keys());
        auto kit = result->keys().find("child");
        REQUIRE(kit != result->keys().end());

        // Check child value
        it = kit->second->values().find("childval");
        REQUIRE(it != kit->second->values().end());
        REQUIRE(it->second->get_string() == "child");

        result->release();

        // Clean up
        key::delete_recursive(test_path);
    }
}

TEST_CASE("registry::exporter", "[registry]") {
    using namespace pnq::regis3;

    SECTION("export simple key to string - format4") {
        key_entry* root = PNQ_NEW key_entry();
        key_entry* k = root->find_or_create_key("HKEY_LOCAL_MACHINE\\SOFTWARE\\Test");
        k->find_or_create_value("StringVal")->set_string("Hello World");
        k->find_or_create_value("DwordVal")->set_dword(42);

        regfile_format4_exporter exporter;
        REQUIRE(exporter.perform_export(root));

        const std::string& result = exporter.result();
        REQUIRE(result.starts_with("REGEDIT4\r\n"));
        REQUIRE(result.find("[HKEY_LOCAL_MACHINE\\SOFTWARE\\Test]") != std::string::npos);
        REQUIRE(result.find("\"StringVal\"=\"Hello World\"") != std::string::npos);
        REQUIRE(result.find("\"DwordVal\"=dword:0000002a") != std::string::npos);

        root->release();
    }

    SECTION("export simple key to string - format5") {
        key_entry* root = PNQ_NEW key_entry();
        key_entry* k = root->find_or_create_key("HKEY_CURRENT_USER\\Test");
        k->find_or_create_value("")->set_string("Default Value");  // default value via empty name

        regfile_format5_exporter exporter;
        REQUIRE(exporter.perform_export(root));

        const std::string& result = exporter.result();
        REQUIRE(result.starts_with("Windows Registry Editor Version 5.00\r\n"));
        REQUIRE(result.find("@=\"Default Value\"") != std::string::npos);

        root->release();
    }

    SECTION("export escapes strings properly") {
        key_entry* root = PNQ_NEW key_entry();
        key_entry* k = root->find_or_create_key("HKEY_LOCAL_MACHINE\\Test");
        k->find_or_create_value("Path")->set_string("C:\\Windows\\System32");
        k->find_or_create_value("Quote")->set_string("Say \"Hello\"");

        regfile_format4_exporter exporter;
        REQUIRE(exporter.perform_export(root));

        const std::string& result = exporter.result();
        REQUIRE(result.find("\"Path\"=\"C:\\\\Windows\\\\System32\"") != std::string::npos);
        REQUIRE(result.find("\"Quote\"=\"Say \\\"Hello\\\"\"") != std::string::npos);

        root->release();
    }

    SECTION("export remove key syntax") {
        key_entry* root = PNQ_NEW key_entry();
        (void)root->find_or_create_key("-HKEY_LOCAL_MACHINE\\DeleteMe");

        regfile_format4_exporter exporter;
        REQUIRE(exporter.perform_export(root));

        const std::string& result = exporter.result();
        REQUIRE(result.find("[-HKEY_LOCAL_MACHINE\\DeleteMe]") != std::string::npos);

        root->release();
    }

    SECTION("export remove value syntax") {
        key_entry* root = PNQ_NEW key_entry();
        key_entry* k = root->find_or_create_key("HKEY_LOCAL_MACHINE\\Test");
        value* v = k->find_or_create_value("ToDelete");
        v->set_remove_flag(true);

        regfile_format4_exporter exporter;
        REQUIRE(exporter.perform_export(root));

        const std::string& result = exporter.result();
        REQUIRE(result.find("\"ToDelete\"=-") != std::string::npos);

        root->release();
    }

    SECTION("export binary value with hex") {
        key_entry* root = PNQ_NEW key_entry();
        key_entry* k = root->find_or_create_key("HKEY_LOCAL_MACHINE\\Test");
        value* v = k->find_or_create_value("Binary");
        bytes data = {0x01, 0x02, 0x03, 0x04};
        v->set_binary_type(REG_BINARY, data);

        regfile_format4_exporter exporter;
        REQUIRE(exporter.perform_export(root));

        const std::string& result = exporter.result();
        REQUIRE(result.find("\"Binary\"=hex:01,02,03,04") != std::string::npos);

        root->release();
    }

    SECTION("export sorts keys and values") {
        key_entry* root = PNQ_NEW key_entry();
        key_entry* k = root->find_or_create_key("HKEY_LOCAL_MACHINE\\Test");
        k->find_or_create_value("Zebra")->set_string("last");
        k->find_or_create_value("Alpha")->set_string("first");
        k->find_or_create_value("Middle")->set_string("middle");

        regfile_format4_exporter exporter;
        REQUIRE(exporter.perform_export(root));

        const std::string& result = exporter.result();

        // Find positions - alpha should come before middle, middle before zebra
        size_t pos_alpha = result.find("\"Alpha\"");
        size_t pos_middle = result.find("\"Middle\"");
        size_t pos_zebra = result.find("\"Zebra\"");

        REQUIRE(pos_alpha != std::string::npos);
        REQUIRE(pos_middle != std::string::npos);
        REQUIRE(pos_zebra != std::string::npos);
        REQUIRE(pos_alpha < pos_middle);
        REQUIRE(pos_middle < pos_zebra);

        root->release();
    }

    SECTION("registry_exporter writes to live registry") {
        const std::string test_path = "HKEY_CURRENT_USER\\Software\\pnq_exporter_test_" + std::to_string(GetCurrentProcessId());

        // Create key_entry tree
        key_entry* root = PNQ_NEW key_entry();
        key_entry* k = root->find_or_create_key(test_path);
        k->find_or_create_value("TestVal")->set_string("Exported");
        k->find_or_create_value("TestNum")->set_dword(12345);

        // Export to registry
        registry_exporter exporter;
        REQUIRE(exporter.perform_export(root));

        // Verify by reading back
        key verify_key(test_path);
        REQUIRE(verify_key.open_for_reading());
        REQUIRE(verify_key.get_string("TestVal") == "Exported");
        REQUIRE(verify_key.get_dword("TestNum") == 12345);

        root->release();
        key::delete_recursive(test_path);
    }

    SECTION("round-trip: export then import") {
        key_entry* original = PNQ_NEW key_entry();
        key_entry* k = original->find_or_create_key("HKEY_LOCAL_MACHINE\\SOFTWARE\\Test");
        k->find_or_create_value("String")->set_string("Value");
        k->find_or_create_value("Number")->set_dword(100);
        k->find_or_create_value("")->set_string("DefaultVal");  // default value

        // Export - use format4 for string round-trip (format5 writes UTF-16LE to files)
        regfile_format4_exporter exporter;
        REQUIRE(exporter.perform_export(original));

        // Import
        regfile_format4_importer importer(exporter.result());
        key_entry* imported = importer.import();
        REQUIRE(imported != nullptr);

        // Verify
        REQUIRE(imported->get_path() == "HKEY_LOCAL_MACHINE\\SOFTWARE\\Test");
        REQUIRE(imported->values().at("string")->get_string() == "Value");
        REQUIRE(imported->values().at("number")->get_dword() == 100);
        REQUIRE(imported->default_value() != nullptr);
        REQUIRE(imported->default_value()->get_string() == "DefaultVal");

        original->release();
        imported->release();
    }
}
