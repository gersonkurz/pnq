#pragma once

#include <memory>
#include <string>
#include <vector>

#include <pnq/config/value_interface.h>

namespace pnq
{
    namespace config
    {
        class Section; // Forward declaration

        /// @brief Vector of configuration sections.
        /// @tparam SectionType The type of section to hold (must derive from Section).
        template <typename SectionType> class TypedValueVector : public ValueInterface
        {
        public:
            TypedValueVector(Section *parentSection, std::string keyName)
                : m_keyName{std::move(keyName)},
                  m_parentSection{parentSection}
            {
                if (parentSection)
                {
                    parentSection->addChildItem(this);
                }
            }

            ~TypedValueVector() override
            {
            }

            // Access methods
            const std::vector<std::unique_ptr<SectionType>> &get() const
            {
                return m_items;
            }

            bool empty() const
            {
                return m_items.empty();
            }

            void clear()
            {
                m_items.clear();
            }

            void add(std::unique_ptr<SectionType> item)
            {
                m_items.push_back(std::move(item));
            }

            SectionType *addNew()
            {
                auto item = std::make_unique<SectionType>(nullptr, m_keyName + "/" + std::to_string(m_items.size()));
                SectionType *ptr = item.get();
                m_items.push_back(std::move(item));
                return ptr;
            }

            size_t size() const
            {
                return m_items.size();
            }

            SectionType *operator[](size_t index)
            {
                return (index < m_items.size()) ? m_items[index].get() : nullptr;
            }

            const SectionType *operator[](size_t index) const
            {
                return (index < m_items.size()) ? m_items[index].get() : nullptr;
            }

            // ValueInterface implementation
            bool load(ConfigBackend &settings) override
            {
                // Clear existing items
                m_items.clear();

                // Discover how many items exist by probing
                int index = 0;
                while (settings.sectionExists(getConfigPath() + "/" + std::to_string(index)))
                {
                    // Create new item with indexed path - parent is the Section that owns this vector
                    auto item = std::make_unique<SectionType>(nullptr, getConfigPath() + "/" + std::to_string(index));

                    // Load the item's data
                    if (!item->load(settings))
                    {
                        // continue with other items
                    }

                    m_items.push_back(std::move(item));
                    index++;
                }
                return true;
            }

            bool save(ConfigBackend &settings) const override
            {
                // 1. Determine old count by probing existing sections
                int oldCount = 0;
                while (settings.sectionExists(getConfigPath() + "/" + std::to_string(oldCount)))
                {
                    oldCount++;
                }

                // 2. Save current items (with updated indices)
                for (size_t i = 0; i < m_items.size(); ++i)
                {
                    // Update the item's section name to match current index
                    // Note: This assumes SectionType allows updating its group name

                    if (!m_items[i]->save(settings))
                    {
                        return false;
                    }
                }

                // 3. Clean up stale entries (if we have fewer items now)
                for (int i = static_cast<int>(m_items.size()); i < oldCount; ++i)
                {
                    if (!settings.deleteSection(getConfigPath() + "/" + std::to_string(i)))
                    {
                        // Continue cleanup even if one fails
                    }
                }

                return true;
            }

            std::string getConfigPath() const override
            {
                return m_parentSection->getConfigPath() + "/" + m_keyName;
            }

            void revertToDefault() override
            {
                // Clear all items and revert to empty vector
                m_items.clear();
            }

            void addChildItem(ValueInterface * /*item*/) override
            {
                // TypedValueVector manages its own children, ignore external additions
            }

        private:
            std::string m_keyName;
            Section *m_parentSection{nullptr};
            std::vector<std::unique_ptr<SectionType>> m_items;
        };
    } // namespace config
} // namespace pnq
