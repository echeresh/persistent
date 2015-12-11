#pragma once
#include <list>
#include "version.h"

namespace persistent
{
    template <class value_type>
    class version_tree
    {
        typedef typename std::list<version_internal<value_type>> list_t;
        typedef typename std::list<version_internal<value_type>>::iterator iterator_t;

        list_t version_list;

        void redistribute()
        {
            auto sz = version_list.size();
            auto min_value = std::numeric_limits<label_type>::min() + 1;
            auto max_value = std::numeric_limits<label_type>::max();
            auto step = (label_type)((max_value - min_value) / sz);
            assert(step > 1);
            auto label = min_value;
            for (auto& v : version_list)
            {
                v.label = label;
                label += step;
            }
        }

    public:
        version_tree(const value_type& root_value = value_type())
        {
            label_type min_value = std::numeric_limits<label_type>::min();
            //support only unsigned types
            assert(min_value == 0);
            //reserve 0 value for default version
            version_list.push_back(version_internal<value_type>(min_value + 1, root_value));
            version_list.begin()->list_iterator = version_list.begin();
        }

        version root_version()
        {
            return *version_list.begin();
        }

        value_type get_value(version v)
        {
            version_internal<value_type>* impl = (version_internal<value_type>*)v.get_impl();
            return impl->value;
        }

        void update(version where, const value_type& value)
        {
            version_internal<value_type>* impl = (version_internal<value_type>*)where.get_impl();
            impl->value = value;
        }

        version insert(version where, const value_type& value)
        {
            version_internal<value_type>* impl = (version_internal<value_type>*)where.get_impl();
            auto where_it = impl->list_iterator;
            auto where_next = where_it;
            where_next++;
            auto where_version = where_it->label;
            auto where_next_version = std::numeric_limits<label_type>::max();
            if (where_next != version_list.end())
            {
                where_next_version = where_next->label;
            }
            if (where_version + 1 == where_next_version)
            {
                redistribute();
                if (where_next != version_list.end())
                {
                    where_next_version = where_next->label;
                }
                where_version = where_it->label;
                
            }
            assert(where_version + 1 != where_next_version);
            auto mid_label = where_version + (where_next_version - where_version) / 2;
            auto it = version_list.insert(where_next, version_internal<value_type>(mid_label, value));
            it->list_iterator = it;
            return *it;
        }
    };
}
