#pragma once
#include <memory>
#include <vector>
#include <cassert>
#include "persistent/persistent_structure.h"
#include "version.h"
#include "binary_tree_node.h"
#include "utils.h"

namespace persistent
{
    template <class key_type, class value_type>
    class binary_tree :
        public persistent_structure<binary_tree<key_type, value_type>>
    {
    public:
        typedef typename binary_tree_node<key_type, value_type> node_t;
        typedef typename std::shared_ptr<node_t> node_ptr_t;
        typedef typename version_context<node_ptr_t> version_context_t;

    private:
        std::shared_ptr<version_tree<node_ptr_t>> vtree;
        version current_version;

        node_ptr_t find_parent(const key_type& key, node_ptr_t node, node_ptr_t parent)
        {
            assert(node);
            auto left = node->get_left(get_vc());
            auto right = node->get_right(get_vc());
            if (node->key == key)
            {
                return node;
            }

            if (node->key < key)
            {
                if (!right)
                {
                    return node;
                }
                return find_parent(key, right, node);
            }
            if (!left)
            {
                return node;
            }
            return find_parent(key, left, node);
        }

        node_ptr_t root() const
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
            binary_tree<key_type, value_type>* bst;
            node_ptr_t node;

            std::shared_ptr<key_value_entry<key_type, value_type>> kve;

            value_type& get_value_ref() const
            {
                return node->get_value(bst->get_vc());
            }

        public:
            friend class binary_tree;

            iterator(binary_tree<key_type, value_type>* bst, node_ptr_t node = node_ptr_t()) :
                bst(bst),
                node(node)
            {
                if (node)
                {
                    auto vc = bst->get_vc();
                    kve = std::shared_ptr<key_value_entry<key_type, value_type>>
                        (new key_value_entry<key_type, value_type>(node->get_key(vc), node->get_value(vc)));
                }
            }

            iterator& operator++()
            {
                auto vc = bst->get_vc();
                node = node->next_node(vc);
                kve.reset();
                if (node)
                {
                    kve = std::shared_ptr<key_value_entry<key_type, value_type>>
                        (new key_value_entry<key_type, value_type>(node->get_key(vc), node->get_value(vc)));
                }
                return *this;
            }

            key_value_entry<key_type, value_type>& operator*()
            {
                return *kve;
            }

            bool operator==(const iterator& it) const
            {
                return node == it.node && get_version() == it.get_version();
            }

            bool operator!=(const iterator& it) const
            {
                return !operator==(it);
            }

            key_value_entry<key_type, value_type>* operator->() const
            {
                return kve.get();
            }

            version get_version() const
            {
                return bst->get_version();
            }
        };

        binary_tree() :
            vtree(new version_tree<node_ptr_t>),
            current_version(vtree->root_version())
        {
        }

        binary_tree(binary_tree& bst, version v) :
            vtree(bst.vtree),
            current_version(v)
        {
        }

        binary_tree<key_type, value_type> create_with_version(version v) override
        {
            return binary_tree<key_type, value_type>(*this, v);
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

        void switch_new_version() override
        {
            auto root_node = root();
            auto new_version = vtree->insert(current_version, root_node);
            current_version = new_version;
        }

        iterator find(const key_type& key)
        {
            auto root_node = root();
            if (!root_node)
            {
                return end();
            }
            auto parent = find_parent(key, root_node, root_node);
            if (parent->key == key)
            {
                return iterator(this, parent);
            }
            else if (key < parent->key)
            {
                return iterator(this, parent->get_left(get_vc()));
            }
            return iterator(this, parent->get_right(get_vc()));
        }

        iterator insert(const key_type& key, const value_type& value)
        {
            version_changed_notifier vcn(*this);
            switch_new_version();
            auto root_node = root();
            if (!root_node)
            {
                root_node = node_ptr_t(new node_t(key, value, get_vc()));
                vtree->update(get_version(), root_node);
                return iterator(this, root_node);
            }

            auto parent = find_parent(key, root_node, root_node);
            if (parent->key == key && parent->value == value)
            {
                return iterator(this, parent);
            }

            node_ptr_t inserted_node;
            if (parent->key == key)
            {
                parent->set_value(value, get_vc());
                inserted_node = parent;
            }
            else if (key < parent->key)
            {
                auto left = parent->get_left(get_vc());
                if (left)
                {
                    left->set_value(value, get_vc());
                    inserted_node = left;
                }
                else
                {
                    auto child = node_ptr_t(new node_t(key, value, get_vc(), parent));
                    parent->set_left(child, get_vc());
                    inserted_node = child;
                }
            }
            else
            {
                auto right = parent->get_right(get_vc());
                if (right)
                {
                    right->set_value(value, get_vc());
                    inserted_node = right;
                }
                else
                {
                    auto child = node_ptr_t(new node_t(key, value, get_vc(), parent));
                    parent->set_right(child, get_vc());
                    inserted_node = child;
                }
            }
            return iterator(this, inserted_node);
        }

        iterator erase(iterator it)
        {
            if (it == end())
            {
                return it;
            }

            version_changed_notifier vcn(*this);
            switch_new_version();

            auto& key = it->key;
            auto node = it.node;
            auto left = node->get_left(get_vc());
            auto right = node->get_right(get_vc());
            auto next = node->next_node(get_vc());
            auto bp = node->get_back_pointer(get_vc());
            node_ptr_t new_bp = left;
            if (!left && right)
            {
                new_bp = right;
            }

            //new_bp child becomes parent
            //second child subtree will be inserted in a tree again
            if (bp)
            {
                //node is not root
                auto bpl = bp->get_left(get_vc());
                auto bpr = bp->get_right(get_vc());
                if (bpl == node)
                {
                    bp->set_left(new_bp, get_vc());
                }
                else
                {
                    assert(bpr == node);
                    bp->set_right(new_bp, get_vc());
                }
                if (new_bp)
                {
                    new_bp->set_back_pointer(bp, get_vc());
                }
            }
            else
            {
                //new node is root
                vtree->update(current_version, new_bp);
                if (new_bp)
                {
                    new_bp->set_back_pointer(node_ptr_t(), get_vc());
                }
            }

            auto root_node = root();
            if (left && right)
            {
                auto child2 = new_bp == left ? right : left;
                auto parent = find_parent(child2->get_key(get_vc()), root_node, root_node);
                assert(parent->key != key);
                if (key < parent->key)
                {
                    parent->set_left(child2, get_vc());
                }
                else
                {
                    parent->set_right(child2, get_vc());
                }
                child2->set_back_pointer(parent, get_vc());
            }
            return iterator(this, next);
        }

        std::string str()
        {
            std::ostringstream oss;
            utils::print_tree(root(), get_vc());
            return oss.str();
        }

        value_type& operator[](const key_type& key)
        {
            auto it = find(key);
            if (it == end())
            {
                version_changed_notifier vcn(*this);
                switch_new_version();
                it = insert(key, value_type());
            }
            return it.get_value_ref();
        }

        iterator begin()
        {
            auto root_node = root();
            return iterator(this, root_node ? root_node->leftmost_child(get_vc()) : root_node);
        }

        iterator end()
        {
            return iterator(this);
        }

        size_t size()
        {
            auto root_node = root();
            if (!root_node)
            {
                return 0;
            }
            return root_node->size(get_vc());
        }

        bool operator==(const binary_tree& bst) const
        {
            return vtree == bst.vtree && current_version == bst.current_version;
        }
    };
}

template <class key_type, class value_type>
std::ostream& operator<<(std::ostream& out, persistent::binary_tree<key_type, value_type>& bst)
{
    out << bst.str();
    return  out;
}
