#pragma once

#include <pnq/config/value_interface.h>
#include <memory>
#include <string>
#include <vector>

namespace pnq
{
    namespace config
    {
        /// @brief A Section is a group of configuration items, which can be
        /// nested. It can contain both TypedValue items and other Sections.
        class Section : public ValueInterface
        { // A Section is also a ValueInterface for nesting
        public:
            explicit Section()
                : m_parent{nullptr}
            {
            }

            // Constructor for child section
            Section(Section *parent, std::string groupNameAsKey)
                : m_parent{parent},
                  m_groupName{std::move(groupNameAsKey)}

            {
                if (parent)
                {
                    parent->addChildItem(this);
                }
            }

            const std::string &getGroupName() const
            {
                return m_groupName;
            }

            bool load(ConfigBackend &settings) override
            {
                bool success = true;
                for (const auto item : m_childItems)
                {
                    if (!item->load(settings))
                    {
                        success = false;
                    }
                }
                return success;
            }

            bool save(ConfigBackend &settings) const override
            {
                bool success = true;
                for (const auto item : m_childItems)
                {
                    if (!item->save(settings))
                    {
                        success = false;
                    }
                }
                return success;
            }
            std::string getConfigPath() const override
            {
                if (!m_parent)
                {
                    return m_groupName;
                }

                const auto parentPath{m_parent->getConfigPath()};
                if (parentPath.empty())
                {
                    return m_groupName;
                }

                return parentPath + "/" + m_groupName;
            }

            void revertToDefault() override
            {
                for (const auto item : m_childItems)
                {
                    item->revertToDefault();
                }
            }

            const auto &getChildItems() const
            {
                return m_childItems;
            }

            // Called by TypedValue<T> and nested Section constructors
            void addChildItem(ValueInterface *item) override
            {
                m_childItems.push_back(item);
            }

        private:
            Section *m_parent{nullptr};
            std::string m_groupName;
            std::vector<ValueInterface *> m_childItems; // Pointers to members (TypedValue or sub-Section)
        };

    } // namespace config
} // namespace pnq
