#pragma once
#include <limits>
#include <vector>
#include <list>
#include <utility>
#include <memory>

namespace persistent
{
    typedef unsigned label_type;

    class version_impl
    {
    public:
        virtual label_type get_label() const = 0;
    };

    class version
    {
        version_impl* impl;

    public:
        version(version_impl* impl = nullptr) :
            impl(impl)
        {
        }

        version_impl* get_impl()
        {
            return impl;
        }

        bool operator<(const version& v) const
        {
            if (!impl)
            {
                return true;
            }
            if (!v.impl)
            {
                return false;
            }
            return impl->get_label() < v.impl->get_label();
        }

        bool operator<=(const version& v) const
        {
            if (!impl)
            {
                return true;
            }
            if (!v.impl)
            {
                return false;
            }
            return impl->get_label() <= v.impl->get_label();
        }

        bool operator==(const version& v) const
        {
            if (!impl || !v.impl)
            {
                return impl == v.impl;
            }
            return impl->get_label() == v.impl->get_label();
        }

        bool operator!=(const version& v) const
        {
            if (!impl || !v.impl)
            {
                return impl != v.impl;
            }
            return impl->get_label() != v.impl->get_label();
        }


        bool is_empty() const
        {
            return !impl;
        }
    };

    template <class value_type>
    struct version_internal : public version_impl
    {
        typedef typename std::list<version_internal<value_type>> list_t;
        typedef typename std::list<version_internal<value_type>>::iterator iterator_t;
        static list_t default_version_list;

        label_type label;
        value_type value;
        iterator_t list_iterator;

        version_internal() :
            list_iterator(default_version_list.begin())
        {
        }

        version_internal(const label_type& label,
                         const value_type& value = value_type(),
                         iterator_t list_iterator = iterator_t()) :
            label(label),
            value(value),
            list_iterator(list_iterator)
        {
        }

        operator version()
        {
            return version(this);
        }

        label_type get_label() const
        {
            return label;
        }
    };

    template <class value_type>
    typename version_internal<value_type>::list_t version_internal<value_type>::default_version_list = { version_internal<value_type>(0) };
}
