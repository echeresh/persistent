#pragma once
#include "version.h"

namespace persistent
{
    class version_structure
    {
    public:
        virtual version get_version() const = 0;
        virtual void set_version(const version& v) = 0;
        virtual void version_changed() = 0;
    };
}
