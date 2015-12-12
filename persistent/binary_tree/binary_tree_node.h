#include "key_value_entry.h"
#include "version/version_tree.h"

namespace persistent
{
    template <class key_type, class value_type>
    struct binary_tree_node :
        std::enable_shared_from_this<binary_tree_node<key_type, value_type>>
    {
        typedef typename binary_tree_node<key_type, value_type> node_t;
        typedef typename std::shared_ptr<node_t> node_ptr_t;
        typedef typename version_tree<node_ptr_t> version_tree_t;

        enum class mod_type
        {
            empty_mod,
            value_mod,
            back_pointer_mod,
            left_mod,
            right_mod
        };

        struct mod_box_entry
        {
            mod_type type;
            version v;
            value_type value;
            node_ptr_t back_pointer;
            node_ptr_t left;
            node_ptr_t right;

            mod_box_entry() :
                type(mod_type::empty_mod)
            {
            }

            mod_box_entry(mod_type type, version v, const value_type& new_value) :
                type(type),
                v(v)
            {
                switch (type)
                {
                case mod_type::value_mod:
                    value = new_value;
                    break;
                }
            }

            mod_box_entry(mod_type type, version v, const std::shared_ptr<binary_tree_node>& new_value) :
                type(type),
                v(v)
            {
                switch (type)
                {
                case mod_type::back_pointer_mod:
                    back_pointer = new_value;
                    break;
                case mod_type::left_mod:
                    left = new_value;
                    break;
                case mod_type::right_mod:
                    right = new_value;
                    break;
                }
            }

            bool is_empty() const
            {
                return type == mod_type::empty_mod;
            }
        };

        const key_type key;
        value_type value;

        std::shared_ptr<binary_tree_node> back_pointer;
        std::shared_ptr<binary_tree_node> left;
        std::shared_ptr<binary_tree_node> right;
        std::vector<mod_box_entry> mod_box;

        binary_tree_node(const key_type& key, const value_type& value,
            node_ptr_t back_pointer = node_ptr_t(),
            node_ptr_t left = node_ptr_t(),
            node_ptr_t right = node_ptr_t(),
            const std::vector<mod_box_entry>& mod_box = std::vector<mod_box_entry>(2 * (2 + 1 + 1))) :
            key(key),
            value(value),
            back_pointer(back_pointer),
            left(left),
            right(right),
            mod_box(mod_box)
        {
        }

        mod_box_entry* get_lastest_mod(mod_type type, std::vector<mod_box_entry>& box, version v)
        {
            version v_last;
            mod_box_entry* me_ptr = nullptr;
            for (auto& mod_entry : box)
            {
                if (mod_entry.type == type && mod_entry.v <= v)
                {
                    if (v_last < mod_entry.v)
                    {
                        v_last = mod_entry.v;
                        me_ptr = &mod_entry;
                    }
                }
            }
            return me_ptr;
        }

        template <class T>
        void add_mod_generic(mod_type type, version v, const T& new_value)
        {
            assert(!is_mod_box_full());
            size_t empty_index = 0;
            for (size_t i = 0; i < mod_box.size(); i++)
            {
                if (mod_box[i].is_empty())
                {
                    empty_index = i;
                    break;
                }
            }
            mod_box[empty_index] = mod_box_entry(type, v, new_value);
        }

        void add_mod(mod_type type, version v, const value_type& value)
        {
            add_mod_generic(type, v, value);
        }

        void add_mod(mod_type type, version v, const node_ptr_t& node)
        {
            add_mod_generic(type, v, node);
        }

        bool is_mod_box_full() const
        {
            return mod_box.back().type != mod_type::empty_mod;
        }

        node_ptr_t split(version v)
        {
            assert(is_mod_box_full());
            auto old_mod_box = mod_box;
            mod_box.resize(mod_box.size() / 2);

            auto new_value = get_value(mod_box, v);
            auto new_back_pointer = get_back_pointer(mod_box, v);
            auto new_left = get_left(mod_box, v);
            auto new_right = get_right(mod_box, v);

            std::vector<mod_box_entry> new_mod_box(old_mod_box.begin() + old_mod_box.size() / 2, old_mod_box.end());
            new_mod_box.resize(old_mod_box.size());
            mod_box = old_mod_box;

            auto new_node = node_ptr_t(new node_t(key, new_value, new_back_pointer, new_left, new_right, new_mod_box));
            return new_node;
        }

        static void update_node(node_ptr_t old_node, node_ptr_t new_node, version v, version_tree_t& vtree)
        {
            auto back_pointer = new_node->get_back_pointer(v);
            if (back_pointer)
            {
                if (back_pointer->get_left(v) == old_node)
                {
                    back_pointer->set_left(new_node, v, vtree);
                }
                else
                {
                    assert(back_pointer->get_right(v) == old_node);
                    back_pointer->set_right(new_node, v, vtree);
                }
            }
            auto left = new_node->get_left(v);
            if (left)
            {
                left->set_back_pointer(new_node, v, vtree);
            }
            auto right = new_node->get_right(v);
            if (right)
            {
                right->set_back_pointer(new_node, v, vtree);
            }
        }

        node_ptr_t split_and_update(version v, version_tree_t& vtree)
        {
            auto new_node = split(v);
            if (!back_pointer)
            {
                //node is root so root pointer of version v should be updated
                vtree.update(v, new_node);
            }
            update_node(shared_from_this(), new_node, v, vtree);
            return new_node;
        }

        void set_value(const value_type& val, version v, version_tree_t& vtree)
        {
            if (!is_mod_box_full())
            {
                add_mod(mod_type::value_mod, v, val);
            }
            else
            {
                node_ptr_t new_node = split_and_update(v, vtree);
                new_node->set_value(val, v, vtree);
            }
        }

        void set_back_pointer(const node_ptr_t& bp, version v, version_tree_t& vtree)
        {
            if (!is_mod_box_full())
            {
                add_mod(mod_type::back_pointer_mod, v, bp);
            }
            else
            {
                node_ptr_t new_node = split_and_update(v, vtree);
                new_node->set_back_pointer(bp, v, vtree);
            }
        }

        void set_left(const node_ptr_t& l, version v, version_tree_t& vtree)
        {
            if (!is_mod_box_full())
            {
                add_mod(mod_type::left_mod, v, l);
            }
            else
            {
                node_ptr_t new_node = split_and_update(v, vtree);
                new_node->set_left(l, v, vtree);
            }
        }

        void set_right(const node_ptr_t& r, version v, version_tree_t& vtree)
        {
            if (!is_mod_box_full())
            {
                add_mod(mod_type::right_mod, v, r);
            }
            else
            {
                node_ptr_t new_node = split_and_update(v, vtree);
                new_node->set_right(r, v, vtree);
            }
        }

        const key_type& get_key(version v) const
        {
            return key;
        }

        value_type& get_value(version v)
        {
            mod_box_entry* m = get_lastest_mod(mod_type::value_mod, mod_box, v);
            value_type& val = !m ? value : m->value;
            return val;
        }

        value_type get_value_by_value(version v, version_tree_t& vtree)
        {
            mod_box_entry* m = get_lastest_mod(mod_type::value_mod, mod_box, v);
            value_type val = !m ? value : m->value;
            set_notification<value_type>(val, v, vtree);
            return val;
        }

        //use SFINAE to find out whether or not value_type is persistent structure
        template <class T>
        void set_notification(typename T::persistent_type& val, version v, version_tree_t& vtree)
        {
            auto& pds = (persistent_structure<value_type>&)val;
            pds.set_parent_version(v);
            pds.notify_version_changed([&,v](version node_version, const value_type& new_value)
                {
                    auto root = vtree.get_value(v);
                    auto new_version = vtree.insert(v, root);
                    set_value(new_value, new_version, vtree);
                    return new_version;
                });
        }

        template <class T>
        void set_notification(T& val, version v, version_tree_t& vtree)
        {
        }

        node_ptr_t get_back_pointer(version v)
        {
            mod_box_entry* m = get_lastest_mod(mod_type::back_pointer_mod, mod_box, v);
            return !m ? back_pointer : m->back_pointer;
        }

        node_ptr_t get_left(version v)
        {
            mod_box_entry* m = get_lastest_mod(mod_type::left_mod, mod_box, v);
            return !m ? left : m->left;
        }

        node_ptr_t get_right(version v)
        {
            mod_box_entry* m = get_lastest_mod(mod_type::right_mod, mod_box, v);
            return !m ? right : m->right;
        }

        value_type get_value(std::vector<mod_box_entry>& box, version v)
        {
            mod_box_entry* m = get_lastest_mod(mod_type::value_mod, box, v);
            return !m ? value : m->value;
        }

        node_ptr_t get_back_pointer(std::vector<mod_box_entry>& box, version v)
        {
            mod_box_entry* m = get_lastest_mod(mod_type::back_pointer_mod, box, v);
            return !m ? back_pointer : m->back_pointer;
        }

        node_ptr_t get_left(std::vector<mod_box_entry>& box, version v)
        {
            mod_box_entry* m = get_lastest_mod(mod_type::left_mod, box, v);
            return !m ? left : m->left;
        }

        node_ptr_t get_right(std::vector<mod_box_entry>& box, version v)
        {
            mod_box_entry* m = get_lastest_mod(mod_type::right_mod, box, v);
            return !m ? right : m->right;
        }

        size_t get_height(version v) const
        {
            auto left = get_left(v);
            auto right = get_right(v);
            size_t left_height = left ? left->get_height(v) : 0;
            size_t right_height = right ? right->get_height(v) : 0;
            return (left_height > right_height) ? left_height + 1 : right_height + 1;
        }

        size_t size(version v)
        {
            auto left = get_left(v);
            auto right = get_right(v);
            size_t left_size = left ? left->size(v) : 0;
            size_t right_size = right ? right->size(v) : 0;
            return left_size + right_size + 1;
        }

        node_ptr_t next_parent(version v)
        {
            auto parent = get_back_pointer(v);
            if (!parent)
            {
                return node_ptr_t();
            }

            if (parent->get_left(v).get() == this)
            {
                return parent;
            }
            return parent->next_parent(v);
        }

        node_ptr_t leftmost_child(version v)
        {
            auto left = get_left(v);
            if (!left)
            {
                return shared_from_this();
            }
            return left->leftmost_child(v);
        }

        node_ptr_t next_node(version v)
        {
            if (auto right = get_right(v))
            {
                return right->leftmost_child(v);
            }

            return next_parent(v);
        }

        std::string str() const
        {
            std::ostringstream oss;
            //oss << "(" << key << " : " << value << ")";
            oss << key << endl;
            return oss.str();
        }
    };
}
