#pragma once
#include <vector>
#include "linked_list/linked_list.h"

namespace persistent
{
    template <class value_type>
    class fat_vector :
        public persistent_structure<fat_vector<value_type>>
    {
        typedef typename linked_list_node<value_type, true> node_t;
        typedef typename std::shared_ptr<node_t> node_ptr_t;
        typedef typename version_context<node_ptr_t> version_context_t;

        std::shared_ptr<version_tree<node_ptr_t>> vtree;
        std::vector<node_ptr_t> vec;
        version current_version;

        version_context_t get_vc()
        {
            return version_context_t(this, get_version(), vtree.get());
        }

        node_ptr_t head() const
        {
            return vtree->get_value(current_version);
        }

    public:
        class iterator
        {
            fat_vector<value_type>* vec;
            int index;

            value_type get_value(value_type& val)
            {
                return get_value_sfinae<value_type>(val);
            }

            template <class T>
            T get_value_sfinae(typename T::persistent_type& val)
            {
                auto& pds = (persistent_structure<value_type>&)val;
                pds.set_parent_version(vec->get_version());
                int index = this->index;
                auto* vec1 = this->vec;
                pds.add_parent(vec1,
                    [&, vec1, index](version node_version, const value_type& new_value)
                {
                    vec1->update(index, new_value);
                    return vec1->get_version();
                });
                return val;
            }

            template <class T>
            T get_value_sfinae(T& val)
            {
                //cout << "not pds" << endl;
                return val;
            }

        public:
            friend class fat_vector;

            iterator(fat_vector<value_type>* vec1, int index) :
                vec(vec1),
                index(index)
            {
            }

            iterator& operator++()
            {
                index++;
                return *this;
            }

            value_type operator*()
            {
                return get_value((*vec)[index]);
            }

            bool operator==(const iterator& it) const
            {
                return index == it.index && get_version() == it.get_version();
            }

            bool operator!=(const iterator& it) const
            {
                return !operator==(it);
            }

            version get_version() const
            {
                return vec->get_version();
            }
        };

        fat_vector() :
            vtree(new version_tree<node_ptr_t>),
            current_version(vtree->root_version())
        {
        }

        fat_vector(fat_vector& vec, version v) :
            vtree(vec.vtree),
            current_version(v)
        {
        }

        fat_vector(linked_list<value_type, true>& l) :
            vtree(l.vtree),
            current_version(l.get_version())
        {
            vec.resize(l.size());
            int index = 0;
            for (auto it = l.begin(); it != l.end(); ++it)
            {
                vec[index++] = it.node;
            }
        }

        fat_vector<value_type> create_with_version(version v) override
        {
            return fat_vector<value_type>(*this, v);
        }

        value_type& operator[](int index)
        {
            return vec[index]->get_value(get_vc());
        }

        void set_version(const version& v) override
        {
            version_changed_notifier vcn(*this);
            current_version = v;
        }

        version get_version() const override
        {
            return current_version;
        }

        void switch_new_version() override
        {
            auto vc = get_vc();
            version new_version = vc.vtree->insert(vc.v, head());
            current_version = new_version;
        }

        bool operator==(const fat_vector& v)
        {
            return vtree == v.vtree && current_version == v.current_version;
        }

        void resize(size_t new_size, value_type val = value_type())
        {
            version_changed_notifier vcn(*this);
            switch_new_version();
            size_t old_size = vec.size();
            vec.resize(new_size);
            for (size_t i = old_size; i < new_size; i++)
            {
                vec[i] = node_ptr_t(new node_t(val, get_vc(), node_ptr_t(), node_ptr_t()));
            }
        }

        void update(int index, const value_type& val)
        {
            version_changed_notifier vcn(*this);
            switch_new_version();
            vec[index]->set_value(val, get_vc());
        }

        iterator erase(iterator it)
        {
            version_changed_notifier vcn(*this);
            switch_new_version();

            int index = it.index;
            for (int i = index; i < size() - 1; i++)
            {
                vec[i]->set_value((*this)[i + 1], get_vc());
            }
            vec.resize(size() - 1);
            return iterator(this, index);
        }

        void push_back(const value_type& val)
        {
            version_changed_notifier vcn(*this);
            switch_new_version();
            vec.push_back(node_ptr_t(new node_t(val, get_vc(), node_ptr_t(), node_ptr_t())));
        }

        size_t size() const
        {
            return vec.size();
        }

        iterator begin()
        {
            return iterator(this, 0);
        }

        iterator end()
        {
            return iterator(this, (int)size());
        }
    };
}
