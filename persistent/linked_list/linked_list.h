#pragma once
#include <memory>
#include "persistent/persistent_structure.h"
#include "version/version_tree.h"
#include "linked_list_node.h"
#include "utils.h"

namespace persistent
{
    template <class value_type>
    class linked_list :
        public persistent_structure<linked_list<value_type>>
    {
    public:
        typedef typename linked_list_node<value_type> node_t;
        typedef typename std::shared_ptr<node_t> node_ptr_t;
        typedef typename version_context<node_ptr_t> version_context_t;

    private:
        std::shared_ptr<version_tree<node_ptr_t>> vtree;
        version current_version;

        node_ptr_t head() const
        {
            return vtree->get_value(current_version);
        }

        version_context_t get_vc()
        {
            return version_context_t(this, get_version(), vtree.get());
        }

    public:
        class iterator
        {
            version_context_t vc;
            node_ptr_t node;

            std::shared_ptr<value_type> kve;

            const value_type& get_const_value_ref() const
            {
                return node->get_value(vc);
            }

        public:
            friend class linked_list;

            iterator(const version_context_t& vc, node_ptr_t node = node_ptr_t()) :
                vc(vc),
                node(node)
            {
                if (node)
                {
                    kve = std::shared_ptr<value_type>(new value_type(node->get_value(vc)));
                }
            }

            iterator& operator++()
            {
                node = node->get_next(vc);
                kve.reset();
                if (node)
                {
                    kve = std::shared_ptr<value_type>(new value_type(node->get_value(vc)));
                }
                return *this;
            }

            value_type& operator*()
            {
                return *kve;
            }

            bool operator==(const iterator& it) const
            {
                return node == it.node;
            }

            bool operator!=(const iterator& it) const
            {
                return node != it.node;
            }

            value_type* operator->() const
            {
                //todo: check
                return kve.get();
            }

            version get_version() const
            {
                return vc.v;
            }
        };

        linked_list() :
            vtree(new version_tree<node_ptr_t>),
            current_version(vtree->root_version())
        {
        }

        linked_list(linked_list& l, version v) :
            vtree(l.vtree),
            current_version(v)
        {
        }

        linked_list<value_type> create_by_version(version v)
        {
            return linked_list<value_type>(*this, v);
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

        std::string str()
        {
            std::ostringstream oss;
            utils::print_list(head(), get_vc());
            return oss.str();
        }

        iterator begin()
        {
            auto head_node = head();
            return iterator(get_vc(), head_node);
        }

        iterator end()
        {
            return iterator(get_vc());
        }

        size_t size()
        {
            auto head_node = head();
            if (!head_node)
            {
                return 0;
            }
            return head_node->size(get_vc());
        }

        void pop_front()
        {
            auto head_node = head();
            if (!head_node)
            {
                return;
            }
            set_version(vtree->insert(current_version, head_node));
            auto prev = head_node->get_prev(get_vc());
            auto next = head_node->get_next(get_vc());
            assert(!prev);
            if (!prev)
            {
                next->set_prev(node_ptr_t(), get_vc());
                vtree->update(current_version, next);
            }
        }

        void push_front(const value_type& value)
        {
            auto head_node = head();
            set_version(vtree->insert(current_version, head_node));

            auto new_head_node = node_ptr_t(new node_t(value, get_vc(), node_ptr_t(), head_node));
            vtree->update(get_version(), new_head_node);

            if (head_node)
            {
                head_node->set_prev(new_head_node, get_vc());
            }
        }

        iterator erase(iterator it)
        {
            auto head_node = head();
            assert(it.vc.v == get_vc().v);

            set_version(vtree->insert(current_version, head_node));

            auto vc = get_vc();
            auto erased_node = it.node;
            auto prev = erased_node->get_prev(vc);
            auto next = erased_node->get_next(vc);
            if (prev)
            {
                prev->set_next(next, vc);
            }
            if (next)
            {
                next->set_prev(prev, vc);
            }
            if (!prev)
            {
                vtree->update(get_version(), next);
            }
            return iterator(get_vc(), next);
        }

        iterator find(const value_type& value)
        {
            auto head_node = head();
            auto found = end();
            for (auto it = begin(); it != end(); ++it)
            {
                if (*it == value)
                {
                    found = it;
                }
            }
            return found;
        }
    };
}

template <class value_type>
std::ostream& operator<<(std::ostream& out, persistent::linked_list<value_type>& l)
{
    out << l.str();
    return  out;
}
