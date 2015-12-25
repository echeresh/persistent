#include "version.h"
#include "persistent/persistent_structure.h"
#include <vector>

namespace persistent
{
    template <class value_type>
    class vector :
        public persistent_structure<vector<value_type>>
    {
        typedef std::shared_ptr<std::vector<value_type>> vector_ptr_t;
        typedef typename version_context<vector_ptr_t> version_context_t;

        std::shared_ptr<version_tree<vector_ptr_t>> vtree;
        version current_version;

        version_context_t get_vc()
        {
            return version_context_t(this, get_version(), vtree.get());
        }

    public:
        class iterator
        {
            vector<value_type>* vec;
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
                auto* vec = this->vec;
                pds.add_parent(vec,
                               [&, vec, index](version node_version, const value_type& new_value)
                               {
                                   vec->update(index, new_value);
                                   return vec->get_version();
                               });
                return val;
            }

            template <class T>
            T get_value_sfinae(T& val)
            {
                cout << "not pds" << endl;
                return val;
            }

        public:
            friend class vector;

            iterator(vector<value_type>* vec, int index) :
                vec(vec),
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

        vector() :
            vtree(new version_tree<vector_ptr_t>),
            current_version(vtree->root_version())
        {
            vtree->update(vtree->root_version(), vector_ptr_t(new std::vector<value_type>()));
        }

        vector(vector& vec, version v) :
            vtree(vec.vtree),
            current_version(v)
        {
        }

        vector<value_type> create_with_version(version v) override
        {
            return vector<value_type>(*this, v);
        }

        value_type& operator[](int index)
        {
            auto& v = get_std_vector();
            return v[index];
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
            auto& vec = *vc.vtree->get_value(vc.v);
            version new_version = vc.vtree->insert(vc.v, vector_ptr_t(new std::vector<value_type>()));
            auto vptr = vc.vtree->get_value(new_version);
            *vptr = vec;
            current_version = new_version;
        }

        bool operator==(const vector& v)
        {
            return vtree == v.vtree && current_version == v.current_version;
        }

        void resize(size_t new_size, value_type val = value_type())
        {
            version_changed_notifier vcn(*this);
            switch_new_version();
            auto& vec = get_std_vector();
            vec.resize(new_size, val);
        }

        void update(int index, const value_type& val)
        {
            version_changed_notifier vcn(*this);
            switch_new_version();
            auto& vec = get_std_vector();
            vec[index] = val;
        }

        iterator erase(iterator it)
        {
            version_changed_notifier vcn(*this);
            switch_new_version();
            auto& vec = get_std_vector();
            int index = it.index;
            vec.erase(ec.begin() + index);
            return iterator(get_version(), vtree, index);
        }

        void push_back(const value_type& val)
        {
            version_changed_notifier vcn(*this);
            switch_new_version();
            auto& v = get_std_vector();
            v.push_back(val);
        }

        std::vector<value_type>& get_std_vector()
        {
            return *vtree->get_value(current_version);
        }

        const std::vector<value_type>& get_std_vector() const
        {
            return *vtree->get_value(current_version);
        }

        size_t size() const
        {
            return get_std_vector().size();
        }

        iterator begin()
        {
            return iterator(this, 0);
        }

        iterator end()
        {
            return iterator(this, size());
        }
    };
}
