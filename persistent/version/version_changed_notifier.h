#pragma once
#include "version.h"
#include "version_structure.h"

namespace persistent
{
    class version_changed_notifier
    {
        version_structure& vs;
        version v;

    public:
        version_changed_notifier(version_structure& vs) :
            vs(vs),
            v(vs.get_version())
        {
        }

        ~version_changed_notifier()
        {
            if (vs.get_version() != v)
            {
                vs.version_changed();
            }
        }
    };
}