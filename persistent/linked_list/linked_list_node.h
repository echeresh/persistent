#pragma once
#include "version/version_tree.h"

namespace persistent
{
    template <class value_type>
    struct linked_list_node :
        std::enable_shared_from_this<linked_list_node<value_type>>
    {
        typedef typename linked_list_node<value_type> node_t;
        typedef typename std::shared_ptr<node_t> node_ptr_t;
        typedef typename version_tree<node_ptr_t> version_tree_t;
        typedef typename version_context<node_ptr_t> version_context_t;

        enum class mod_type
        {
            empty_mod,
            value_mod,
            prev_mod,
            next_mod
        };

        struct mod_box_entry
        {
            mod_type type;
            version v;
            value_type value;
            node_ptr_t prev;
            node_ptr_t next;

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
            mod_box_entry(mod_type type, version v, const std::shared_ptr<linked_list_node>& new_value) :
                type(type),
                v(v)
            {
                switch (type)
                {
                case mod_type::prev_mod:
                    prev = new_value;
                    break;
                case mod_type::next_mod:
                    next = new_value;
                    break;
                }
            }

            bool is_empty() const
            {
                return type == mod_type::empty_mod;
            }
        };

        value_type value;

        node_ptr_t prev;
        node_ptr_t next;
        std::vector<mod_box_entry> mod_box;

        linked_list_node(const value_type& value,
            const version_context_t& vc,
            node_ptr_t prev = node_ptr_t(),
            node_ptr_t next = node_ptr_t(),
            const std::vector<mod_box_entry>& mod_box = std::vector<mod_box_entry>(2 * (2 + 1 + 1))) :
            value(value),
            prev(prev),
            next(next),
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
            auto new_prev = get_prev(mod_box, vc);
            auto new_next = get_next(mod_box, vc);

            std::vector<mod_box_entry> new_mod_box(old_mod_box.begin() + old_mod_box.size() / 2, old_mod_box.end());
            new_mod_box.resize(old_mod_box.size());
            mod_box = old_mod_box;

            auto new_node = node_ptr_t(new node_t(new_value, vc, new_prev, new_next, new_mod_box));
            return new_node;
        }

        static void update_node(node_ptr_t old_node, node_ptr_t new_node, const version_context_t& vc)
        {
            auto prev = new_node->get_prev(vc);
            if (prev)
            {
                prev->set_prev(new_node, vc);
            }
            auto next = new_node->get_next(vc);
            if (next)
            {
                next->set_prev(new_node, vc);
            }
        }

        node_ptr_t split_and_update(const version_context_t& vc)
        {
            auto new_node = split(vc);
            if (!get_prev(vc))
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

        void set_prev(const node_ptr_t& l, const version_context_t& vc)
        {
            if (!is_mod_box_full())
            {
                add_mod(mod_type::prev_mod, vc.v, l);
            }
            else
            {
                node_ptr_t new_node = split_and_update(vc);
                new_node->set_prev(l, vc);
            }
        }

        void set_next(const node_ptr_t& r, const version_context_t& vc)
        {
            if (!is_mod_box_full())
            {
                add_mod(mod_type::next_mod, vc.v, r);
            }
            else
            {
                node_ptr_t new_node = split_and_update(vc);
                new_node->set_next(r, vc);
            }
        }

        value_type& get_value(const version_context_t& vc)
        {
            mod_box_entry* m = get_lastest_mod(mod_type::value_mod, mod_box, vc.v);
            value_type& val = !m ? value : m->value;
            register_callbacks<value_type>(val, vc);
            return val;
        }

        node_ptr_t get_prev(const version_context_t& vc)
        {
            mod_box_entry* m = get_lastest_mod(mod_type::prev_mod, mod_box, vc.v);
            return !m ? prev : m->prev;
        }

        node_ptr_t get_next(const version_context_t& vc)
        {
            mod_box_entry* m = get_lastest_mod(mod_type::next_mod, mod_box, vc.v);
            return !m ? next : m->next;
        }

        value_type& get_value(std::vector<mod_box_entry>& box, const version_context_t& vc)
        {
            mod_box_entry* m = get_lastest_mod(mod_type::value_mod, box, vc.v);
            value_type& val = !m ? value : m->value;
            register_callbacks<value_type>(val, vc);
            return val;
        }

        node_ptr_t get_prev(std::vector<mod_box_entry>& box, const version_context_t& vc)
        {
            mod_box_entry* m = get_lastest_mod(mod_type::prev_mod, box, vc.v);
            return !m ? prev : m->prev;
        }

        node_ptr_t get_next(std::vector<mod_box_entry>& box, const version_context_t& vc)
        {
            mod_box_entry* m = get_lastest_mod(mod_type::next_mod, box, vc.v);
            return !m ? next : m->next;
        }

        size_t size(const version_context_t& vc)
        {
            auto next = get_next(vc);
            size_t next_size = next ? next->size(vc) : 0;
            return next_size + 1;
        }

        std::string str(const version_context_t& vc)
        {
            std::ostringstream oss;
            oss << get_value(vc);
            return oss.str();
        }
    };
}
