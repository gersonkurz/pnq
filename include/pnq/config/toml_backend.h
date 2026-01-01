#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <toml++/toml.hpp>
#include <pnq/config/config_backend.h>

namespace pnq
{
    namespace config
    {
        class TomlBackend : public ConfigBackend
        {
        public:
            explicit TomlBackend(const std::string &filename)
                : m_filename{filename},
                  m_config{}
            {
                loadFromFile();
            }

            bool load(const std::string &path, int32_t &value) override
            {
                if (auto val = getValueAtPath(path))
                {
                    if (val->is_integer())
                    {
                        value = static_cast<int32_t>(val->as_integer()->get());
                        return true;
                    }
                }
                return false;
            }

            bool save(const std::string &path, int32_t value) override
            {
                return setValueAtPath(path, value);
            }

            bool load(const std::string &path, bool &value) override
            {
                if (auto val = getValueAtPath(path))
                {
                    if (val->is_boolean())
                    {
                        value = val->as_boolean()->get();
                        return true;
                    }
                }
                return false;
            }

            bool save(const std::string &path, bool value) override
            {
                return setValueAtPath(path, value);
            }

            bool load(const std::string &path, std::string &value) override
            {
                if (auto val = getValueAtPath(path))
                {
                    if (val->is_string())
                    {
                        value = val->as_string()->get();
                        return true;
                    }
                }
                return false;
            }

            bool save(const std::string &path, const std::string &value) override
            {
                return setValueAtPath(path, value);
            }

            bool sectionExists(const std::string &path) override
            {
                auto parts = splitPath(path);
                if (parts.empty())
                {
                    return false;
                }

                toml::node *current = &m_config;

                // Navigate through all path parts
                for (const auto &part : parts)
                {
                    if (auto table = current->as_table())
                    {
                        auto it = table->find(part);
                        if (it != table->end())
                        {
                            current = &it->second;
                        }
                        else
                        {
                            return false; // Path doesn't exist
                        }
                    }
                    else
                    {
                        return false; // Not a table
                    }
                }

                // Check if final node is a table (section)
                return current->is_table();
            }

            bool deleteKey(const std::string &path) override
            {
                auto parts = splitPath(path);
                if (parts.empty())
                    return false;

                toml::table *current = &m_config;

                // Navigate to parent table
                for (size_t i = 0; i < parts.size() - 1; ++i)
                {
                    auto it = current->find(parts[i]);
                    if (it != current->end())
                    {
                        if (auto table = it->second.as_table())
                        {
                            current = table;
                        }
                        else
                        {
                            return false; // Path doesn't exist or not a table
                        }
                    }
                    else
                    {
                        return false; // Path doesn't exist
                    }
                }

                // Remove the final key
                bool success = current->erase(parts.back());
                if (success)
                {
                    saveToFile();
                }
                return success;
            }

            bool deleteSection(const std::string &path) override
            {
                auto parts = splitPath(path);
                if (parts.empty())
                    return false;

                toml::table *current = &m_config;

                // Navigate to parent table
                for (size_t i = 0; i < parts.size() - 1; ++i)
                {
                    auto it = current->find(parts[i]);
                    if (it != current->end())
                    {
                        if (auto table = it->second.as_table())
                        {
                            current = table;
                        }
                        else
                        {
                            return false; // Path doesn't exist or not a table
                        }
                    }
                    else
                    {
                        return false; // Path doesn't exist
                    }
                }

                // Remove the entire section
                bool success = current->erase(parts.back());
                if (success)
                {
                    saveToFile();
                }
                return success;
            }

        private:
            const std::string m_filename;
            toml::table m_config;

            void loadFromFile()
            {
                try
                {
                    if (std::filesystem::exists(m_filename))
                    {
                        m_config = toml::parse_file(m_filename);
                    }
                }
                catch (const toml::parse_error&)
                {
                    // Log error but continue with empty config
                    m_config = toml::table{};
                }
            }

            void saveToFile()
            {
                std::ofstream file(m_filename);
                if (file.is_open())
                {
                    file << m_config;
                }
            }

            static std::vector<std::string> splitPath(const std::string &path)
            {
                std::vector<std::string> parts;
                std::stringstream ss{path};
                std::string part;

                while (std::getline(ss, part, '/'))
                {
                    if (!part.empty())
                    {
                        parts.push_back(part);
                    }
                }
                return parts;
            }

            toml::node *getValueAtPath(const std::string &path)
            {
                auto parts = splitPath(path);
                if (parts.empty())
                {
                    return nullptr;
                }

                toml::node *current = &m_config;

                // Navigate to the parent table
                for (size_t i = 0; i < parts.size() - 1; ++i)
                {
                    if (const auto& table = current->as_table())
                    {
                        auto it = table->find(parts[i]);
                        if (it != table->end())
                        {
                            current = &it->second;
                        }
                        else
                        {
                            return nullptr; // Path doesn't exist
                        }
                    }
                    else
                    {
                        return nullptr; // Not a table
                    }
                }

                // Get the final value
                if (auto table = current->as_table())
                {
                    auto it = table->find(parts.back());
                    if (it != table->end())
                    {
                        return &it->second;
                    }
                }
                return nullptr;
            }

            template <typename T> bool setValueAtPath(const std::string &path, const T &value)
            {
                auto parts = splitPath(path);
                if (parts.empty())
                    return false;

                toml::table *current = &m_config;

                // Navigate/create the nested structure
                for (size_t i = 0; i < parts.size() - 1; ++i)
                {
                    auto it = current->find(parts[i]);
                    if (it != current->end())
                    {
                        // Path exists, check if it's a table
                        if (auto table = it->second.as_table())
                        {
                            current = table;
                        }
                        else
                        {
                            // Exists but not a table, can't continue
                            return false;
                        }
                    }
                    else
                    {
                        // Create new table
                        auto [inserted_it, success] = current->insert(parts[i], toml::table{});
                        if (success)
                        {
                            current = inserted_it->second.as_table();
                        }
                        else
                        {
                            return false;
                        }
                    }
                }

                // Set the final value
                current->insert_or_assign(parts.back(), value);
                saveToFile();
                return true;
            }
        };
    } // namespace config
} // namespace pnq
