#pragma once

/// @file pnq/regis3/key_entry.h
/// @brief Registry key tree node - in-memory representation of a registry key

#include <pnq/regis3/types.h>
#include <pnq/regis3/value.h>
#include <pnq/ref_counted.h>
#include <pnq/string.h>
#include <pnq/pnq.h>

#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cassert>

namespace pnq
{
    namespace regis3
    {
        /// In-memory representation of a registry key.
        ///
        /// Forms a tree structure with parent/child relationships.
        /// Uses reference counting for memory management.
        /// Keys and values are stored case-insensitively (lowercase keys).
        class key_entry final : public RefCountImpl
        {
        public:
            /// Default constructor creates an unnamed root key.
            key_entry()
                : m_parent{nullptr},
                  m_default_value{nullptr},
                  m_remove_flag{false}
            {
            }

            /// Construct a named key with parent.
            /// @param parent Parent key (may be nullptr for root)
            /// @param name Key name (stored as-is, lookups are case-insensitive)
            key_entry(key_entry* parent, std::string_view name)
                : m_parent{parent},
                  m_name{name},
                  m_default_value{nullptr},
                  m_remove_flag{false}
            {
                if (m_parent)
                {
                    m_parent->retain();
                }
            }

            ~key_entry()
            {
                // Release parent reference
                if (m_parent)
                {
                    m_parent->release();
                    m_parent = nullptr;
                }

                // Clean up values
                for (auto& [key, val] : m_values)
                {
                    delete val;
                }
                m_values.clear();

                delete m_default_value;
                m_default_value = nullptr;

                // Release child key references
                for (auto& [key, child] : m_keys)
                {
                    child->release();
                }
                m_keys.clear();
            }

            PNQ_DECLARE_NON_COPYABLE(key_entry)

            // =================================================================
            // Accessors
            // =================================================================

            /// Get the key name (not the full path).
            const std::string& name() const
            {
                return m_name;
            }

            /// Get the parent key (may be nullptr for root).
            key_entry* parent() const
            {
                return m_parent;
            }

            /// Check if this key should be removed (for diff/merge operations).
            bool remove_flag() const
            {
                return m_remove_flag;
            }

            /// Set the remove flag.
            void set_remove_flag(bool flag)
            {
                m_remove_flag = flag;
            }

            /// Get the full registry path.
            /// Reconstructs path by walking up the parent chain.
            /// @return Full path like "HKEY_LOCAL_MACHINE\\SOFTWARE\\MyApp"
            std::string get_path() const
            {
                std::vector<std::string_view> path_components;
                const key_entry* k = this;

                while (k)
                {
                    if (!k->m_parent)
                    {
                        // Root node - only add if it has a name
                        if (!k->m_name.empty())
                        {
                            path_components.push_back(k->m_name);
                        }
                        break;
                    }
                    assert(!k->m_name.empty());
                    path_components.push_back(k->m_name);
                    k = k->m_parent;
                }

                // Reverse and join with backslashes
                string::Writer result;
                bool first = true;
                for (auto it = path_components.rbegin(); it != path_components.rend(); ++it)
                {
                    if (first)
                        first = false;
                    else
                        result.append('\\');
                    result.append(*it);
                }
                return result.as_string();
            }

            // =================================================================
            // Key Navigation
            // =================================================================

            /// Find or create a subkey by path.
            /// Creates intermediate keys as needed.
            /// Handles "-PATH" syntax for remove flag.
            /// @param path Relative path (backslash-separated)
            /// @return Pointer to the (possibly newly created) key entry
            key_entry* find_or_create_key(std::string_view path)
            {
                assert(!path.empty());

                std::string copy_of_path{path};
                bool remove_this = false;

                // Handle leading minus sign for remove flag
                if (!copy_of_path.empty() && copy_of_path[0] == '-')
                {
                    remove_this = true;
                    copy_of_path = copy_of_path.substr(1);
                }

                key_entry* result = this;
                auto tokens = string::split(copy_of_path, "\\");

                for (const auto& token : tokens)
                {
                    std::string key = string::lowercase(token);
                    auto it = result->m_keys.find(key);

                    key_entry* subkey = nullptr;
                    if (it != result->m_keys.end())
                    {
                        subkey = it->second;
                    }
                    else
                    {
                        subkey = PNQ_NEW key_entry(result, token);
                        result->m_keys[key] = subkey;
                    }

                    assert(subkey->m_parent == result);
                    result = subkey;
                }

                if (remove_this)
                {
                    result->m_remove_flag = true;
                }

                return result;
            }

            /// Find or create a named value.
            /// @param name Value name (empty string for default value)
            /// @return Pointer to the (possibly newly created) value
            value* find_or_create_value(std::string_view name)
            {
                if (name.empty())
                {
                    if (!m_default_value)
                    {
                        m_default_value = PNQ_NEW value();
                    }
                    return m_default_value;
                }

                std::string name_as_key = string::lowercase(name);
                auto it = m_values.find(name_as_key);
                if (it != m_values.end())
                {
                    return it->second;
                }

                value* v = PNQ_NEW value(name);
                m_values[name_as_key] = v;
                return v;
            }

            // =================================================================
            // Deep Copy
            // =================================================================

            /// Create a deep copy of this key entry.
            /// @param new_parent Parent for the cloned key
            /// @return New key_entry with reference count of 1
            key_entry* clone(key_entry* new_parent) const
            {
                key_entry* result = PNQ_NEW key_entry(new_parent, m_name);
                result->m_remove_flag = m_remove_flag;

                // Clone subkeys
                for (const auto& [key, child] : m_keys)
                {
                    result->m_keys[key] = child->clone(result);
                }

                // Clone values
                for (const auto& [key, val] : m_values)
                {
                    result->m_values[key] = PNQ_NEW value(*val);
                }

                // Clone default value
                if (m_default_value)
                {
                    result->m_default_value = PNQ_NEW value(*m_default_value);
                }

                return result;
            }

            // =================================================================
            // Diff/Merge Operations
            // =================================================================

            /// Create or find a key for adding content.
            /// Used when building diff/merge output.
            /// @param add_this Source key to copy content from
            /// @return Key entry in this tree at the same path
            key_entry* ask_to_add_key(const key_entry* add_this)
            {
                key_entry* key = find_or_create_key(add_this->get_path());

                // Copy subkeys
                for (const auto& [subkey_name, subkey] : add_this->m_keys)
                {
                    key_entry* cloned = subkey->clone(key);
                    key->m_keys[subkey_name] = cloned;
                }

                // Copy values
                for (const auto& [val_name, val] : add_this->m_values)
                {
                    key->m_values[val_name] = PNQ_NEW value(*val);
                }

                // Copy default value
                if (add_this->m_default_value)
                {
                    delete key->m_default_value;
                    key->m_default_value = PNQ_NEW value(*add_this->m_default_value);
                }

                return key;
            }

            /// Create key entry marked for removal.
            /// @param remove_this Source key
            /// @return Key entry with remove_flag set
            key_entry* ask_to_remove_key(const key_entry* remove_this)
            {
                key_entry* key = ask_to_add_key(remove_this);
                key->m_remove_flag = true;
                return key;
            }

            /// Add a value to the diff/merge output.
            /// @param key Source key containing the value
            /// @param v Value to add
            void ask_to_add_value(const key_entry* key, const value* v)
            {
                key_entry* k = ask_to_add_key(key);

                if (v->is_default_value())
                {
                    delete k->m_default_value;
                    k->m_default_value = PNQ_NEW value(*v);
                }
                else
                {
                    std::string val_name = string::lowercase(v->name());
                    delete k->m_values[val_name];  // safe even if nullptr
                    k->m_values[val_name] = PNQ_NEW value(*v);
                }
            }

            /// Add a value marked for removal.
            /// @param key Source key containing the value
            /// @param v Value to mark as removed
            void ask_to_remove_value(const key_entry* key, const value* v)
            {
                key_entry* k = ask_to_add_key(key);

                if (v->is_default_value())
                {
                    delete k->m_default_value;
                    k->m_default_value = PNQ_NEW value(*v);
                    k->m_default_value->set_remove_flag(true);
                }
                else
                {
                    std::string val_name = string::lowercase(v->name());
                    value* nv = PNQ_NEW value(*v);
                    nv->set_remove_flag(true);
                    delete k->m_values[val_name];
                    k->m_values[val_name] = nv;
                }
            }

            // =================================================================
            // Container Access (for iteration)
            // =================================================================

            /// Get subkeys map (for iteration).
            const std::unordered_map<std::string, key_entry*>& keys() const
            {
                return m_keys;
            }

            /// Get values map (for iteration).
            const std::unordered_map<std::string, value*>& values() const
            {
                return m_values;
            }

            /// Get default value (may be nullptr).
            value* default_value() const
            {
                return m_default_value;
            }

            /// Check if key has any values (including default).
            bool has_values() const
            {
                return !m_values.empty() || m_default_value != nullptr;
            }

            /// Check if key has any subkeys.
            bool has_keys() const
            {
                return !m_keys.empty();
            }

        private:
            friend class regfile_exporter;
            friend class registry_exporter;
            friend class registry_importer;

            /// Parent key (nullptr for root).
            key_entry* m_parent;

            /// Key name (not the full path).
            std::string m_name;

            /// Subkeys indexed by lowercase name.
            std::unordered_map<std::string, key_entry*> m_keys;

            /// Named values indexed by lowercase name.
            std::unordered_map<std::string, value*> m_values;

            /// Default (unnamed) value, or nullptr.
            value* m_default_value;

            /// Flag indicating this key should be removed.
            bool m_remove_flag;
        };

    } // namespace regis3
} // namespace pnq
