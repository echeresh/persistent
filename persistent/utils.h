#include <iostream>
#include <deque>
#include <iomanip>
using namespace std;

namespace utils
{
    template <class node_ptr_type, class versionype>
    void print_tree(node_ptr_type node, versionype v, std::ostream& out = cout, const string& prefix = "", bool last = true)
    {
        std::istringstream iss(node->str());
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

        auto left = node->get_left(v);
        auto right = node->get_right(v);
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
                print_tree(left, v, out, indent, false);
            }
            else
            {
                print_tree(left, v, out, prefix + (last ? "    " : "|   "), true);
                return;
            }
        }
        print_tree(right, v, out, prefix + (last ? "    " : "|   "), true);
    }
}
