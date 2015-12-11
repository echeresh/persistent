#pragma once
#include <memory>
#include <vector>
#include <cassert>
#include <tuple>
#include <sstream>
#include "persistent/persistent_structure.h"
#include "version/version_tree.h"
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

    private:
        std::shared_ptr<version_tree<node_ptr_t>> vtree;
        version current_version;

        node_ptr_t find_parent(const key_type& key, node_ptr_t node, node_ptr_t parent) const
        {
            assert(node);
            auto left = node->get_left(current_version);
            auto right = node->get_right(current_version);
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

    public:
        class const_iterator
        {
            version v;
            std::shared_ptr<version_tree<node_ptr_t>> vtree;
            node_ptr_t node;
            std::shared_ptr<key_value_entry<key_type, value_type>> kve;

        public:
            const_iterator(version v, std::shared_ptr<version_tree<node_ptr_t>> vtree, node_ptr_t node = node_ptr_t()) :
                v(v),
                vtree(vtree),
                node(node)
            {
                if (node)
                {
                    kve = std::shared_ptr<key_value_entry<key_type, value_type>>(new key_value_entry<key_type, value_type>(node->get_key(v), node->get_value(v)));
                }
            }

            const_iterator(version v, const const_iterator& it) :
                const_iterator(v, it.vtree, it.node)
            {
            }

            const_iterator& operator++()
            {
                node = node->next_node(v);
                kve.reset();
                if (node)
                {
                    auto* kvep = new key_value_entry<key_type, value_type>(node->get_key(v), node->get_value(v));
                    kve = std::make_shared<key_value_entry<key_type, value_type>>(kvep);
                }
                return *this;
            }

            key_value_entry<key_type, value_type>&  operator*()
            {
                return *kve;
            }

            bool operator==(const const_iterator& it) const
            {
                return node == it.node;
            }

            bool operator!=(const const_iterator& it) const
            {
                return node != it.node;
            }

            const key_value_entry<key_type, value_type>* operator->() const
            {
                return kve.get();
            }

            version get_version() const
            {
                return v;
            }

            value_type value_by_value()
            {
                return node->get_value_by_value(v, *vtree);
            }
        };

        class iterator
        {
            version v;
            std::shared_ptr<version_tree<node_ptr_t>> vtree;
            node_ptr_t node;
            
            std::shared_ptr<key_value_entry<key_type, value_type>> kve;

        public:
            friend class binary_tree;

            iterator(version v, std::shared_ptr<version_tree<node_ptr_t>> vtree, node_ptr_t node = node_ptr_t()) :
                v(v),
                vtree(vtree),
                node(node)                
            {
                if (node)
                {
                    kve = std::shared_ptr<key_value_entry<key_type, value_type>>(new key_value_entry<key_type, value_type>(node->get_key(v), node->get_value(v)));
                }
            }

            iterator(version v, const iterator& it) :
                iterator(v, it.vtree, it.node)
            {
            }

            iterator& operator++()
            {
                node = node->next_node(v);
                kve.reset();
                if (node)
                {
                    auto* kvep = new key_value_entry<key_type, value_type>(node->get_key(v), node->get_value(v));
                    kve = std::shared_ptr<key_value_entry<key_type, value_type>>(kvep);
                }
                return *this;
            }

            key_value_entry<key_type, value_type>&  operator*()
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

            key_value_entry<key_type, value_type>* operator->() const
            {
                return kve.get();
            }

            version get_version() const
            {
                return v;
            }

            value_type value_by_value()
            {
                return node->get_value_by_value(v, *vtree);
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

        binary_tree<key_type, value_type> create_by_version(version v)
        {
            return binary_tree<key_type, value_type>(*this, v);
        }

        void switch_version2(version v)
        {
            current_version = v;
        }

        version get_version() const
        {
            return current_version;
        }

        const_iterator find(const key_type& key) const
        {
            auto root_node = root();
            if (!root_node)
            {
                return end();
            }
            auto parent = find_parent(key, root_node, root_node);
            if (parent->key == key)
            {
                return const_iterator(current_version, vtree, parent);
            }
            else if (key < parent->key)
            {
                return const_iterator(current_version, vtree, parent->get_left(current_version));
            }
            return const_iterator(current_version, vtree, parent->get_right(current_version));
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
                return iterator(current_version, vtree, parent);
            }
            else if (key < parent->key)
            {
                return iterator(current_version, vtree, parent->get_left(current_version));
            }
            return iterator(current_version, vtree, parent->get_right(current_version));
        }

        iterator insert(const key_type& key, const value_type& value)
        {
            auto root_node = root();
            if (!root_node)
            {
                root_node = node_ptr_t(new node_t(key, value));
                current_version = vtree->insert(current_version, root_node);
                vtree->update(current_version, root_node);
                version_changed();
                return iterator(current_version, vtree, root_node);
            }

            auto parent = find_parent(key, root_node, root_node);
            if (parent->key == key && parent->value == value)
            {
                version_changed();
                return iterator(current_version, vtree, parent);
            }

            node_ptr_t inserted_node;
            current_version = vtree->insert(current_version, root_node);
            if (parent->key == key)
            {
                parent->set_value(value, current_version, *vtree);
                inserted_node = parent;
            }
            else if (key < parent->key)
            {
                auto left = parent->get_left(current_version);
                if (left)
                {
                    left->set_value(value, current_version, *vtree);
                    inserted_node = left;
                }
                else
                {
                    auto child = node_ptr_t(new node_t(key, value, parent));
                    parent->set_left(child, current_version, *vtree);
                    inserted_node = child;
                }
            }
            else
            {
                auto right = parent->get_right(current_version);
                if (right)
                {
                    right->set_value(value, current_version, *vtree);
                    inserted_node = right;
                }
                else
                {
                    auto child = node_ptr_t(new node_t(key, value, parent));
                    parent->set_right(child, current_version, *vtree);
                    inserted_node = child;
                }
            }
            version_changed();
            return iterator(current_version, vtree, inserted_node);
        }

        iterator erase(iterator it)
        {
            if (it == end())
            {
                return it;
            }

            current_version = vtree->insert(current_version, root());

            auto& key = it->key;
            auto node = it.node;
            auto left = node->get_left(current_version);
            auto right = node->get_right(current_version);
            auto bp = node->get_back_pointer(current_version);
            //auto bbp = !bp ? bp : bp->get_back_pointer(current_version);
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
                auto bpl = bp->get_left(current_version);
                auto bpr = bp->get_right(current_version);
                if (bpl == node)
                {
                    bp->set_left(new_bp, current_version, *vtree);
                }
                else
                {
                    assert(bpr == node);
                    bp->set_right(new_bp, current_version, *vtree);
                }
                if (new_bp)
                {
                    new_bp->set_back_pointer(bp, current_version, *vtree);
                }
            }
            else
            {
                //new node is root
                vtree->update(current_version, new_bp);
                if (new_bp)
                {
                    new_bp->set_back_pointer(node_ptr_t(), current_version, *vtree);
                }
            }

            auto root_node = root();
            if (left && right)
            {
                auto child2 = new_bp == left ? right : left;
                auto parent = find_parent(child2->get_key(current_version), root_node, root_node);
                assert(parent->key != key);
                if (key < parent->key)
                {
                    parent->set_left(child2, current_version, *vtree);
                }
                else
                {
                    parent->set_right(child2, current_version, *vtree);
                }
                child2->set_back_pointer(parent, current_version, *vtree);
            }
            version_changed();
            return ++iterator(current_version, it);
        }

        std::string str() const
        {
            std::ostringstream oss;
            utils::print_tree(root(), current_version);
            return oss.str();
        }

        value_type& operator[](const key_type& key)
        {
            auto it = find(key);
            if (it == end())
            {
                it = insert(key, value_type());
                return it->value;
            }
            return it->value;
        }

        node_ptr_t root() const
        {
            return vtree->get_value(current_version);
        }

        const_iterator begin() const
        {
            auto root_node = root();
            return const_iterator(current_version, root_node ? root_node->leftmost_child(current_version) : root_node);
        }

        const_iterator end() const
        {
            return const_iterator(current_version);
        }

        iterator begin()
        {
            auto root_node = root();
            return iterator(current_version, vtree, root_node ? root_node->leftmost_child(current_version) : root_node);
        }

        iterator end()
        {
            return iterator(current_version, vtree);
        }

        size_t size() const
        {
            auto root_node = root();
            if (!root_node)
            {
                return 0;
            }
            return root_node->size(current_version);
        }

        bool operator==(const binary_tree& bst) const
        {
            return vtree == bst.vtree && current_version == bst.current_version;
        }
    };
}
