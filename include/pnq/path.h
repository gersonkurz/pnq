#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <pnq/string.h>
#include <pnq/wstring.h>
#include <pnq/environment_variables.h>
#include <pnq/directory.h>
#include <pnq/console.h>
#include <pnq/file.h>

namespace pnq
{
    namespace path
    {
        constexpr auto separator()
        {
            return '\\';
        }

        constexpr auto separator_string()
        {
            return "\\";
        }

        inline std::string normalize(std::string_view path_pattern, const std::unordered_map<std::string, std::string> &vars)
        {
            // TODO: add proper implementation
            return std::string{path_pattern};
        }

        inline std::string normalize(std::string_view path_pattern)
        {
            return normalize(path_pattern, {});
        }

        class PathCombiner final
        {
        public:
            PathCombiner()
            {
            }

            void push_component(std::string_view component)
            {
                const auto normalized_component{path::normalize(component)};
                for (auto subcomponent : string::split(normalized_component, path::separator_string()))
                {
                    if (string::equals(subcomponent, ".."))
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

            auto as_string() const
            {
                return string::join(m_components, path::separator_string());
            }

        private:
            PathCombiner(const PathCombiner &) = delete;
            PathCombiner &operator=(const PathCombiner &) = delete;
            PathCombiner(PathCombiner &&) = delete;
            PathCombiner &operator=(PathCombiner &&) = delete;

        private:
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

        template <typename T, typename... ARGS> inline auto combine(const T &x, const ARGS &...args)
        {
            PathCombiner result;
            combine_internal_do_not_use_directly(result, x, args...);
            return result.as_string();
        }

        /// <summary>
        /// Given a filename, change the path to another extension
        /// </summary>
        /// <param name="filename"></param>
        /// <param name="new_extension"></param>
        /// <returns></returns>
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

        inline bool determine_existing_file(std::string &name, bool is_executable)
        {
            if (file::exists(name.c_str()))
                return true;

            if (!is_executable)
                return false;

            std::string pathext{".EXE;.BAT;.CMD"};
            environment_variables::get("PATHEXT", pathext);
            for (const auto possible_extension : string::split(pathext, ";"))
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

        inline bool locate_in_directory(std::string_view directory, std::string_view filename, std::string &result, bool is_executable)
        {
            if (string::is_empty(directory))
                return false;

            auto temp = combine(directory, filename);
            if (!determine_existing_file(temp, is_executable))
                return false;

            result = temp;
            return true;
        }

        inline bool find_filename(std::string_view name, std::string &result, bool is_executable)
        {
            if (file::exists(name))
            {
                result = name;
                return true;
            }

            const std::vector<std::function<std::string()>> methods{&directory::application, &directory::current, &directory::system, &directory::windows};
            for (const auto method : methods)
            {
                if (locate_in_directory(method(), name, result, is_executable))
                    return true;
            }
            std::string path;
            if (!environment_variables::get("PATH", path))
            {
                spdlog::error("Didn't get the PATH variable");
                return false;
            }

            for (const auto path_element : string::split(path, ";"))
            {
                if (locate_in_directory(path_element, name, result, is_executable))
                    return true;
            }
            return false;
        }

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

        inline std::filesystem::path get_roaming_app_data()
        {
            // Get ROAMINGAPPDATA path
            PWSTR roamingAppDataPath = nullptr;
            HRESULT hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &roamingAppDataPath);
            if (FAILED(hr))
            {
                return {};
            }

            const std::wstring wpath{roamingAppDataPath};
            CoTaskMemFree(roamingAppDataPath);

            // Convert to UTF-8 and append pserv5 folder
            const std::string appDataPath = string::encode_as_utf8(wpath);
            const std::filesystem::path path = std::filesystem::path(appDataPath) / "pserv5";

            // Create directory if it doesn't exist
            if (!std::filesystem::exists(path))
            {
                std::filesystem::create_directories(path);
            }

            return path;
        }
    } // namespace path
} // namespace pnq
