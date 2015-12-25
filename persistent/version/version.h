#pragma once
#include <limits>
#include <vector>
#include <list>
#include <utility>
#include <memory>
#include <cassert>
#include <sstream>
#include <ostream>

namespace persistent
{
    typedef unsigned label_type;

    class version_impl
    {
    public:
        virtual label_type get_begin_label() const = 0;
        virtual label_type get_end_label() const = 0;
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
            if (!impl || !v.impl)
            {
                return true;
            }
            return impl->get_begin_label() < v.impl->get_begin_label() &&
                   v.impl->get_end_label() < impl->get_end_label();
        }

        bool operator<=(const version& v) const
        {
            return *this == v || *this < v;
        }

        bool operator==(const version& v) const
        {
            assert(impl && v.impl);
            if (!impl || !v.impl)
            {
                return impl == v.impl;
            }
            return impl->get_begin_label() == v.impl->get_begin_label() &&
                   impl->get_end_label() == v.impl->get_end_label();;
        }

        bool operator!=(const version& v) const
        {
            return !operator==(v);
        }


        bool is_empty() const
        {
            return !impl;
        }

        std::string str() const
        {
            std::ostringstream oss;
            oss << "(" << impl->get_begin_label() << ", " << impl->get_end_label() << ")";
            return oss.str();
        }
    };

    template <class value_type>
    struct version_internal : public version_impl
    {
        typedef typename std::list<version_internal<value_type>> list_t;
        typedef typename std::list<version_internal<value_type>>::iterator iterator_t;
        static list_t default_version_list;

        label_type begin_label;
        label_type end_label;
        label_type free_range;
        value_type value;
        iterator_t list_iterator;

        version_internal() :
            list_iterator(default_version_list.begin())
        {
        }

        version_internal(const label_type& begin_label,
                         const label_type& end_label,
                         const label_type& free_size,
                         const value_type& value = value_type(),
                         iterator_t list_iterator = iterator_t()) :
            begin_label(begin_label),
            end_label(end_label),
            free_range(free_size),
            value(value),
            list_iterator(list_iterator)
        {
        }

        operator version()
        {
            return version(this);
        }

        label_type get_begin_label() const
        {
            return begin_label;
        }

        label_type get_end_label() const
        {
            return end_label;
        }
    };

    template <class value_type>
    typename version_internal<value_type>::list_t version_internal<value_type>::default_version_list = { version_internal<value_type>() };
}

//std::ostream& operator<<(std::ostream& out, persistent::version v);