#include "version/version.h"
#include "version/version_tree.h"
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

        static version copy_into_new_version(const version_context_t& vc, bool update_version = true)
        {
            auto& vec = *vc.vtree->get_value(vc.v);
            version new_version = vc.vtree->insert(vc.v, vector_ptr_t(new std::vector<value_type>()));
            auto vptr = vc.vtree->get_value(new_version);
            *vptr = vec;
            if (update_version)
            {
                vc.vs->set_version(new_version);
            }
            return new_version;
        }

        version_context_t get_vc()
        {
            return version_context_t(this, get_version(), vtree.get());
        }

    public:
        class iterator
        {
            version_context_t vc;
            vector_ptr_t vector_ptr;
            int index;

            value_type get_value(value_type& val)
            {
                return get_value_sfinae<value_type>(val);
            }

            template <class T>
            T get_value_sfinae(typename T::persistent_type& val)
            {
                cout << "pds" << endl;
                auto& pds = (persistent_structure<value_type>&)val;
                pds.set_parent_version(vc.v);
                int index = this->index;
                version_context_t vc = this->vc;
                pds.add_parent(vc.vs,
                    [&, vc, index](version node_version, const value_type& new_value)
                    {
                        auto root = vc.vtree->get_value(vc.v);
                        auto new_version = vector<value_type>::copy_into_new_version(vc, false);
                        auto& new_vec = *vc.vtree->get_value(new_version);
                        new_vec[index] = new_value;
                        return new_version;
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

            iterator(const version_context_t& vc, int index) :
                vc(vc),
                vector_ptr(vc.vtree->get_value(vc.v)),
                index(index)
            {
            }

            iterator(version v, const iterator& it) :
                iterator(version_context_t(it.vc.vs, v, it.vc.vtree), it.index)
            {
            }

            iterator& operator++()
            {
                index++;
                return *this;
            }

            value_type operator*()
            {
                return get_value((*vector_ptr)[index]);
            }

            bool operator==(const iterator& it) const
            {
                return index == it.index;
            }

            bool operator!=(const iterator& it) const
            {
                return index != it.index;
            }

            version get_version() const
            {
                return vc.v;
            }
        };

        vector() :
            vtree(new version_tree<vector_ptr_t>),
            current_version(vtree->root_version())
        {
            vtree->update(vtree->root_version(), vector_ptr_t(new std::vector<value_type>()));
        }

        value_type& operator[](int index)
        {
            auto& v = get_std_vector();
            return v[index];
        }

        void set_version(const version& v)
        {
            current_version = v;
            version_changed();
        }

        version get_version() const
        {
            return current_version;
        }

        bool operator==(const vector& v)
        {
            return vtree == v.vtree && current_version == v.current_version;
        }

        void resize(size_t new_size, value_type val = value_type())
        {
            auto& v = get_std_vector();
            v.resize(new_size, val);
        }

        void update(int index, const value_type& val)
        {
            copy_into_new_version(get_vc());
            auto& v = get_std_vector();
            v[index] = val;
        }

        iterator erase(iterator it)
        {
            copy_into_new_version(get_vc());
            auto& v = get_std_vector();
            int index = it.index;
            v.erase(v.begin() + index);
            return iterator(get_version(), vtree, index);
        }

        void push_back(const value_type& val)
        {
            copy_into_new_version(get_vc());
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
            return iterator(get_vc(), 0);
        }

        iterator end()
        {
            return iterator(get_vc(), size());
        }
    };
}
