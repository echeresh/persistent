#pragma once
#include "version.h"
#include <stack>

namespace persistent
{
    class version_history
    {
        std::stack<version> undo_stack;
        std::stack<version> redo_stack;

    public:
        void add_item(version& v)
        {
            undo_stack.push(v);
        }

        version pop_undo()
        {
            if (undo_stack.empty())
            {
                return version();
            }

            version v = undo_stack.top();
            redo_stack.push(v);
            undo_stack.pop();
            return v;
        }

        version pop_redo()
        {
            if (redo_stack.empty())
            {
                return version();
            }
            version v = redo_stack.top();
            undo_stack.push(v);
            redo_stack.pop();
            return v;
        }
    };
}