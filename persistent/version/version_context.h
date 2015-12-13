#include "version_tree.h"

namespace persistent
{
    template <class value_type>
    struct version_context
    {
        version_structure* vs;
        version v;
        version_tree<value_type>* vtree;

        version_context(version_structure* vs, const version& v,
            version_tree<value_type>* vtree) :
            vs(vs), v(v), vtree(vtree)
        {
        }

    };
}