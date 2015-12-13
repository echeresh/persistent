#pragma once
#include "version/version.h"
#include <functional>
#include <cassert>

namespace persistent
{
    class version_structure
    {
    public:
        virtual version get_version() const = 0;
        virtual void set_version(const version& v) = 0;
    };

    template <class T>
    class persistent_structure : public version_structure
    {
        std::function<version(version, const T&)> version_changed_notifier;
        version parent_version;
        version_structure* parent;

    public:
        virtual void version_changed()
        {
            if (version_changed_notifier)
            {
                auto fixed_version = parent_version;
                parent_version = version_changed_notifier(get_version(), (const T&)*this);
                if (parent->get_version() == fixed_version)
                {
                    parent->set_version(parent_version);
                }
            }
        }

        virtual void add_parent(version_structure* parent, std::function<version(version, const T&)> version_changed_notifier)
        {
            this->parent = parent;
            this->version_changed_notifier = version_changed_notifier;
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
    };
}
