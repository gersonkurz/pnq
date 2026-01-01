#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <pnq/log.h>
#include <pnq/platform.h>
#include <pnq/string.h>
#include <pnq/wstring.h>
#include <pnq/environment_variables.h>
#include <pnq/string_expander.h>
#include <pnq/directory.h>
#include <pnq/console.h>
#include <pnq/file.h>

namespace pnq
{
    namespace path
    {
        /// Get the platform path separator character.
        constexpr char separator()
        {
#ifdef PNQ_PLATFORM_WINDOWS
            return '\\';
#else
            return '/';
#endif
        }

        /// Get the platform path separator as a string.
        constexpr const char* separator_string()
        {
#ifdef PNQ_PLATFORM_WINDOWS
            return "\\";
#else
            return "/";
#endif
        }

        /// Get the "other" path separator (the one NOT native to this platform).
        constexpr char other_separator()
        {
#ifdef PNQ_PLATFORM_WINDOWS
            return '/';
#else
            return '\\';
#endif
        }

        /// Get builtin path variables.
        /// @return map with CD, APPDIR, WINDIR, SYSDIR
        inline std::unordered_map<std::string, std::string> get_builtin_vars()
        {
            return {
                {"CD", directory::current()},
                {"APPDIR", directory::application()},
                {"WINDIR", directory::windows()},
                {"SYSDIR", directory::system()},
            };
        }

        /// Normalize a path pattern with variable substitution.
        /// Expands %VAR% patterns using provided vars, builtins (CD, APPDIR, WINDIR, SYSDIR),
        /// and environment variables (in that priority order).
        /// Also normalizes path separators to platform native (backslash on Windows, forward slash elsewhere).
        /// @param path_pattern path to normalize
        /// @param vars variable map for substitution (highest priority)
        /// @return normalized path with variables expanded
        inline std::string normalize(std::string_view path_pattern, const std::unordered_map<std::string, std::string> &vars)
        {
            // Merge user vars with builtins (user vars take priority)
            auto merged = get_builtin_vars();
            for (const auto &[k, v] : vars)
            {
                merged[k] = v;
            }

            // Expand variables
            std::string result = string::Expander{merged, true}.expand(path_pattern);

            // Normalize path separators to platform native
            for (char &c : result)
            {
                if (c == other_separator())
                    c = separator();
            }

            return result;
        }

        /// Normalize a path pattern using builtins and environment variables.
        inline std::string normalize(std::string_view path_pattern)
        {
            return normalize(path_pattern, {});
        }

        /// Helper class for combining path components.
        /// Handles ".." navigation.
        class PathCombiner final
        {
        public:
            PathCombiner() = default;

            /// Add a path component, handling ".." navigation.
            void push_component(std::string_view component)
            {
                const auto normalized_component{path::normalize(component)};
                for (const auto &subcomponent : string::split(normalized_component, path::separator_string()))
                {
                    if (subcomponent == "..")
                    {
                        if (!m_components.empty())
                        {
                            m_components.pop_back();
                        }
                    }
                    else
                    {
                        m_components.push_back(subcomponent);
                    }
                }
            }

            /// Get the combined path as a string.
            auto as_string() const
            {
                return string::join(m_components, path::separator_string());
            }

        private:
            PNQ_DECLARE_NON_COPYABLE(PathCombiner)

            std::vector<std::string> m_components;
        };

        inline void combine_internal_do_not_use_directly(PathCombiner &)
        {
        }

        template <typename T> void combine_internal_do_not_use_directly(PathCombiner &output, const T &x)
        {
            output.push_component(x);
        }

        template <typename T, typename... ARGS> void combine_internal_do_not_use_directly(PathCombiner &output, const T &x, const ARGS &...args)
        {
            output.push_component(x);
            combine_internal_do_not_use_directly(output, args...);
        }

        /// Combine multiple path components into a single path.
        /// Handles ".." navigation and normalizes separators.
        template <typename T, typename... ARGS> inline auto combine(const T &x, const ARGS &...args)
        {
            PathCombiner result;
            combine_internal_do_not_use_directly(result, x, args...);
            return result.as_string();
        }

        /// Change the extension of a filename.
        /// @param filename original filename
        /// @param new_extension new extension (should include the dot)
        /// @return filename with new extension
        inline std::string change_extension(std::string_view filename, std::string_view new_extension)
        {
            const auto pos = filename.rfind('.');
            if (pos == std::string_view::npos)
            {
                std::string result{filename};
                result += new_extension;
                return result;
            }
            else
            {
                std::string result{filename.substr(0, pos)};
                result += new_extension;
                return result;
            }
        }

        /// Check if file exists, trying executable extensions if needed.
        /// @param name filename to check (modified if found with different extension)
        /// @param is_executable if true, try PATHEXT extensions
        /// @return true if file exists
        inline bool determine_existing_file(std::string &name, bool is_executable)
        {
            if (file::exists(name.c_str()))
                return true;

            if (!is_executable)
                return false;

            std::string pathext{".EXE;.BAT;.CMD"};
            environment_variables::get("PATHEXT", pathext);
            for (const auto &possible_extension : string::split(pathext, ";"))
            {
                const auto temp{change_extension(name, possible_extension)};
                if (file::exists(temp))
                {
                    name = temp;
                    return true;
                }
            }
            return false;
        }

        /// Look for a file in a specific directory.
        inline bool locate_in_directory(std::string_view directory, std::string_view filename, std::string &result, bool is_executable)
        {
            if (directory.empty())
                return false;

            auto temp = combine(directory, filename);
            if (!determine_existing_file(temp, is_executable))
                return false;

            result = temp;
            return true;
        }

        /// Search for a file in standard locations and PATH.
        /// @param name filename to find
        /// @param result receives the full path if found
        /// @param is_executable if true, try executable extensions
        /// @return true if found
        inline bool find_filename(std::string_view name, std::string &result, bool is_executable)
        {
            if (file::exists(name))
            {
                result = name;
                return true;
            }

            const std::vector<std::function<std::string()>> methods{&directory::application, &directory::current, &directory::system, &directory::windows};
            for (const auto &method : methods)
            {
                if (locate_in_directory(method(), name, result, is_executable))
                    return true;
            }
            std::string path;
            if (!environment_variables::get("PATH", path))
            {
                PNQ_LOG_ERROR("Didn't get the PATH variable");
                return false;
            }

            for (const auto &path_element : string::split(path, ";"))
            {
                if (locate_in_directory(path_element, name, result, is_executable))
                    return true;
            }
            return false;
        }

        /// Search for an executable in standard locations and PATH.
        /// Adds .exe extension if none provided.
        inline bool find_executable(std::string_view name, std::string &result)
        {
            if (file::get_extension(name).empty())
            {
                std::string combined_filename{name};
                combined_filename += ".exe";
                return find_filename(combined_filename, result, true);
            }
            return find_filename(name, result, true);
        }

        /// Get a known folder path by FOLDERID.
        /// @param folder_id KNOWNFOLDERID (e.g. FOLDERID_RoamingAppData, FOLDERID_LocalAppData)
        /// @return path to the known folder, or empty path on failure
        inline std::filesystem::path get_known_folder(const KNOWNFOLDERID &folder_id)
        {
            PWSTR folder_path = nullptr;
            HRESULT hr = SHGetKnownFolderPath(folder_id, 0, nullptr, &folder_path);
            if (FAILED(hr))
            {
                return {};
            }

            const std::wstring wpath{folder_path};
            CoTaskMemFree(folder_path);

            return std::filesystem::path(string::encode_as_utf8(wpath));
        }

        /// Get a known folder path with an app subfolder, creating it if needed.
        /// @param folder_id KNOWNFOLDERID (e.g. FOLDERID_RoamingAppData, FOLDERID_LocalAppData)
        /// @param app_name application folder name
        /// @return path to the application subfolder, or empty path on failure
        inline std::filesystem::path get_known_folder(const KNOWNFOLDERID &folder_id, std::string_view app_name)
        {
            auto base = get_known_folder(folder_id);
            if (base.empty())
            {
                return {};
            }

            const auto path = base / std::string(app_name);

            if (!std::filesystem::exists(path))
            {
                std::filesystem::create_directories(path);
            }

            return path;
        }

        /// Get the roaming app data folder for a specific application.
        /// Creates the folder if it doesn't exist.
        /// @param app_name application folder name
        /// @return path to the application's roaming app data folder
        inline std::filesystem::path get_roaming_app_data(std::string_view app_name)
        {
            return get_known_folder(FOLDERID_RoamingAppData, app_name);
        }
    } // namespace path
} // namespace pnq
