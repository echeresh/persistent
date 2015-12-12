#include "binary_tree/binary_tree.h"

namespace persistent
{
    template <class key_type, class value_type>
    class map
    {
        binary_tree<key_type, value_type> bst;

    public:
        typedef typename binary_tree<key_type, value_type>::version version_t;
        typedef typename binary_tree<key_type, value_type>::iterator iterator;
        typedef typename binary_tree<key_type, value_type>::const_iterator const_iterator;

        map()
        {
        }

        value_type& operator[](const key_type& key)
        {
            auto it = bst.find(key);
            if (it == bst.end())
            {
                auto it = bst.insert(key, value_type());
                return it->value;
            }
            return it->value;
        }

        iterator find(const key_type& key)
        {
            return bst.find(key);
        }

        const_iterator begin() const
        {
            return bst.begin();
        }

        const_iterator end() const
        {
            return bst.end();
        }

        iterator begin()
        {
            return bst.begin();
        }

        iterator end()
        {
            return bst.end();
        }
    };
}
