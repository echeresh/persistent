#pragma once
#include "version/version.h"
#include <functional>

namespace persistent
{
    template <class T>
    class persistent_structure
    {
        std::function<version(version, const T&)> callback;
        version parent_version;

    public:
        virtual void version_changed()
        {
            if (callback)
            {
                parent_version = callback(get_version(), (const T&)*this);
            }
        }

        virtual void notify_version_changed(std::function<version(version, const T&)> callback)
        {
            this->callback = callback;
        }

        void set_parent_version(version parent_version)
        {
            this->parent_version = parent_version;
        }

        typedef T type;
        virtual version get_version() const = 0;

        version get_parent_version() const
        {
            return parent_version;
        }
    };
}