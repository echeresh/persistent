#pragma once
#include <iostream>
#include <deque>
#include <iomanip>
using namespace std;

namespace utils
{
    template <class node_ptr_t, class version_context_t>
    void print_tree(node_ptr_t node, const version_context_t& vc, std::ostream& out = cout,
                    const string& prefix = "", bool last = true)
    {
        std::istringstream iss(node->str(vc));
        bool first = true;
        std::string indent = prefix + (last ? "    " : "|   ");
        while (!iss.eof())
        {
            std::string s;
            getline(iss, s);
            if (first)
            {
                out << prefix << (last ? "+-- " : "|-- ") << s << std::endl;
                first = false;
            }
            else
            {
                out << indent << s << std::endl;
            }
        }

        auto left = node->get_left(vc);
        auto right = node->get_right(vc);
        if (left || right)
        {
            out << indent << '|' << std::endl;
        }
        else
        {
            out << indent << std::endl;
            return;
        }

        if (left)
        {
            if (right)
            {
                print_tree(left, vc, out, indent, false);
            }
            else
            {
                print_tree(left, vc, out, prefix + (last ? "    " : "|   "), true);
                return;
            }
        }
        print_tree(right, vc, out, prefix + (last ? "    " : "|   "), true);
    }

    template <class node_ptr_t, class version_context_t>
    void print_list(node_ptr_t node, const version_context_t& vc, std::ostream& out = cout)
    {
        while (node)
        {
            out << node->str(vc) << endl;
            node = node->get_next(vc);
        }
    }
}
