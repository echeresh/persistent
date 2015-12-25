#pragma once
#include "version.h"
#include <functional>
#include <cassert>

namespace persistent
{
    template <class T>
    class persistent_structure : public version_structure
    {
        std::function<version(version, const T&)> version_changed_callback;
        version parent_version;
        version_structure* parent;
        version_history history;

    public:
        virtual void version_changed()
        {
            if (version_changed_callback)
            {
                auto fixed_version = parent_version;
                parent_version = version_changed_callback(get_version(), (const T&)*this);
                if (parent->get_version() == fixed_version)
                {
                    parent->set_version(parent_version);
                }
            }
            history.add_item(get_version());
        }

        void undo()
        {
            auto v = get_version();
            set_version(history.pop_undo());
            if (get_version() == v)
            {
                undo();
            }
        }

        void redo()
        {
            auto v = get_version();
            set_version(history.pop_redo());
            if (get_version() == v)
            {
                redo();
            }
        }

        virtual void add_parent(version_structure* parent, std::function<version(version, const T&)> version_changed_callback)
        {
            this->parent = parent;
            this->version_changed_callback = version_changed_callback;
        }

        void set_parent_version(version parent_version)
        {
            this->parent_version = parent_version;
        }

        typedef T persistent_type;

        version get_parent_version() const
        {
            return parent_version;
        }

        virtual void switch_new_version() = 0;
        virtual T create_with_version(version v) = 0;
    };
}
