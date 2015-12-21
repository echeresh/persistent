#pragma once
#include "key_value_entry.h"
#include "version/version_tree.h"
#include "version/version_context.h"

namespace persistent
{

    template <class key_type, class value_type>
    struct binary_tree_node :
        std::enable_shared_from_this<binary_tree_node<key_type, value_type>>
    {
        typedef typename binary_tree_node<key_type, value_type> node_t;
        typedef typename std::shared_ptr<node_t> node_ptr_t;
        typedef typename version_tree<node_ptr_t> version_tree_t;
        typedef typename version_context<node_ptr_t> version_context_t;

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
            const version_context_t& vc,
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
            register_callbacks<value_type>(this->value, vc);
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

        node_ptr_t split(const version_context_t& vc)
        {
            assert(is_mod_box_full());
            auto old_mod_box = mod_box;
            mod_box.resize(mod_box.size() / 2);

            auto& new_value = get_value(mod_box, vc);
            auto new_back_pointer = get_back_pointer(mod_box, vc);
            auto new_left = get_left(mod_box, vc);
            auto new_right = get_right(mod_box, vc);

            std::vector<mod_box_entry> new_mod_box(old_mod_box.begin() + old_mod_box.size() / 2, old_mod_box.end());
            new_mod_box.resize(old_mod_box.size());
            mod_box = old_mod_box;

            auto new_node = node_ptr_t(new node_t(key, new_value, vc, new_back_pointer, new_left, new_right, new_mod_box));
            return new_node;
        }

        static void update_node(node_ptr_t old_node, node_ptr_t new_node, const version_context_t& vc)
        {
            auto back_pointer = new_node->get_back_pointer(vc);
            if (back_pointer)
            {
                if (back_pointer->get_left(vc) == old_node)
                {
                    back_pointer->set_left(new_node, vc);
                }
                else
                {
                    assert(back_pointer->get_right(vc) == old_node);
                    back_pointer->set_right(new_node, vc);
                }
            }
            auto left = new_node->get_left(vc);
            if (left)
            {
                left->set_back_pointer(new_node, vc);
            }
            auto right = new_node->get_right(vc);
            if (right)
            {
                right->set_back_pointer(new_node, vc);
            }
        }

        node_ptr_t split_and_update(const version_context_t& vc)
        {
            auto new_node = split(vc);
            //todo: call get_back_pointer
            if (!back_pointer)
            {
                //node is root so root pointer of version v should be updated
                vc.vtree->update(vc.v, new_node);
            }
            update_node(shared_from_this(), new_node, vc);
            return new_node;
        }

        //use SFINAE to find out whether or not value_type is persistent structure
        template <class T>
        void register_callbacks(typename T::persistent_type& val, const version_context_t& vc)
        {
            auto& pds = (persistent_structure<value_type>&)val;
            pds.set_parent_version(vc.v);
            pds.add_parent(vc.vs,
                [&, vc](version node_version, const value_type& new_value)
                {
                    auto root = vc.vtree->get_value(vc.v);
                    auto new_version = vc.vtree->insert(vc.v, root);
                    set_value(new_value, version_context_t(vc.vs, new_version, vc.vtree));
                    return new_version;
                });
        }

        template <class T>
        void register_callbacks(T& val, const version_context_t& vc)
        {
        }


        void set_value(const value_type& val, const version_context_t& vc)
        {
            if (!is_mod_box_full())
            {
                add_mod(mod_type::value_mod, vc.v, val);
            }
            else
            {
                node_ptr_t new_node = split_and_update(vc);
                new_node->set_value(val, vc);
            }
            auto& inserted_val = get_value(vc);
            register_callbacks<value_type>(inserted_val, vc);
        }

        void set_back_pointer(const node_ptr_t& bp, const version_context_t& vc)
        {
            if (!is_mod_box_full())
            {
                add_mod(mod_type::back_pointer_mod, vc.v, bp);
            }
            else
            {
                node_ptr_t new_node = split_and_update(vc);
                new_node->set_back_pointer(bp, vc);
            }
        }

        void set_left(const node_ptr_t& l, const version_context_t& vc)
        {
            if (!is_mod_box_full())
            {
                add_mod(mod_type::left_mod, vc.v, l);
            }
            else
            {
                node_ptr_t new_node = split_and_update(vc);
                new_node->set_left(l, vc);
            }
        }

        void set_right(const node_ptr_t& r, const version_context_t& vc)
        {
            if (!is_mod_box_full())
            {
                add_mod(mod_type::right_mod, vc.v, r);
            }
            else
            {
                node_ptr_t new_node = split_and_update(vc);
                new_node->set_right(r, vc);
            }
        }

        const key_type& get_key(const version_context_t& vc) const
        {
            return key;
        }

        value_type& get_value(const version_context_t& vc)
        {
            mod_box_entry* m = get_lastest_mod(mod_type::value_mod, mod_box, vc.v);
            value_type& val = !m ? value : m->value;
            register_callbacks<value_type>(val, vc);
            return val;
        }

        node_ptr_t get_back_pointer(const version_context_t& vc)
        {
            mod_box_entry* m = get_lastest_mod(mod_type::back_pointer_mod, mod_box, vc.v);
            return !m ? back_pointer : m->back_pointer;
        }

        node_ptr_t get_left(const version_context_t& vc)
        {
            mod_box_entry* m = get_lastest_mod(mod_type::left_mod, mod_box, vc.v);
            return !m ? left : m->left;
        }

        node_ptr_t get_right(const version_context_t& vc)
        {
            mod_box_entry* m = get_lastest_mod(mod_type::right_mod, mod_box, vc.v);
            return !m ? right : m->right;
        }

        value_type& get_value(std::vector<mod_box_entry>& box, const version_context_t& vc)
        {
            mod_box_entry* m = get_lastest_mod(mod_type::value_mod, box, vc.v);
            value_type& val = !m ? value : m->value;
            register_callbacks<value_type>(val, vc);
            return val;
        }

        node_ptr_t get_back_pointer(std::vector<mod_box_entry>& box, const version_context_t& vc)
        {
            mod_box_entry* m = get_lastest_mod(mod_type::back_pointer_mod, box, vc.v);
            return !m ? back_pointer : m->back_pointer;
        }

        node_ptr_t get_left(std::vector<mod_box_entry>& box, const version_context_t& vc)
        {
            mod_box_entry* m = get_lastest_mod(mod_type::left_mod, box, vc.v);
            return !m ? left : m->left;
        }

        node_ptr_t get_right(std::vector<mod_box_entry>& box, const version_context_t& vc)
        {
            mod_box_entry* m = get_lastest_mod(mod_type::right_mod, box, vc.v);
            return !m ? right : m->right;
        }

        size_t get_height(const version_context_t& vc) const
        {
            auto left = get_left(vc);
            auto right = get_right(vc);
            size_t left_height = left ? left->get_height(vc) : 0;
            size_t right_height = right ? right->get_height(vc) : 0;
            return (left_height > right_height) ? left_height + 1 : right_height + 1;
        }

        size_t size(const version_context_t& vc)
        {
            auto left = get_left(vc);
            auto right = get_right(vc);
            size_t left_size = left ? left->size(vc) : 0;
            size_t right_size = right ? right->size(vc) : 0;
            return left_size + right_size + 1;
        }

        node_ptr_t next_parent(const version_context_t& vc)
        {
            auto parent = get_back_pointer(vc);
            if (!parent)
            {
                return node_ptr_t();
            }

            if (parent->get_left(vc).get() == this)
            {
                return parent;
            }
            return parent->next_parent(vc);
        }

        node_ptr_t leftmost_child(const version_context_t& vc)
        {
            auto left = get_left(vc);
            if (!left)
            {
                return shared_from_this();
            }
            return left->leftmost_child(vc);
        }

        node_ptr_t next_node(const version_context_t& vc)
        {
            if (auto right = get_right(vc))
            {
                return right->leftmost_child(vc);
            }

            return next_parent(vc);
        }

        std::string str(const version_context_t& vc)
        {
            std::ostringstream oss;
            oss << get_key(vc);
            return oss.str();
        }
    };
}
