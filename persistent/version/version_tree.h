#pragma once
#include <list>
#include <map>
#include <utility>
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
            auto step = (label_type)((max_value - min_value) / sz / 2);
            assert(step > 1);

            std::map<label_type, version_internal<value_type>*> m;
            for (auto& v : version_list)
            {
                m[v.begin_label] = &v;
                m[v.end_label] = &v;
            }

            label_type label = min_value;
            for (auto& e : m)
            {
                auto* v_int = e.second;
                if (v_int->end_label != 0)
                {
                    v_int->begin_label = label;
                    v_int->end_label = 0;
                }
                else
                {
                    v_int->end_label = label;
                    v_int->free_range = step - 1;
                }
                label += step;
            }
        }

    public:
        version_tree(const value_type& root_value = value_type())
        {
            auto min_value = std::numeric_limits<label_type>::min();
            auto max_value = std::numeric_limits<label_type>::max();
            assert(min_value == 0);
            //reserve 0 value jic
            auto free_range = max_value - min_value - 2;
            version_list.push_back(version_internal<value_type>(min_value + 1, max_value, free_range, root_value));
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
            auto e = impl->end_label;
            auto f = impl->free_range;
            if (f < 2)
            {
                redistribute();
            }
            e = impl->end_label;
            f = impl->free_range;
            assert(f >= 2);
            auto f_step = (f + 1) / 3;
            assert(f_step >= 1);
            auto new_beg = e - f - 1 + f_step;
            auto new_end = new_beg + f_step;
            auto new_free = new_end - new_beg - 1;
            assert(new_end < e);
            impl->free_range = e - new_end - 1;
            auto it = version_list.insert(where_next, version_internal<value_type>(new_beg, new_end, new_free, value));
            it->list_iterator = it;
            return *it;
        }
    };
}
