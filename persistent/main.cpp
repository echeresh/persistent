#include <iostream>
#include <memory>
#include <list>
#include "persistent.h"
using namespace std;

int main()
{
    persistent::binary_tree<int, int> bst;
    for (int i = 5; i < 10; i++)
    {
        bst.insert(i, i);
    }

    for (int i = 0; i < 5; i++)
    {
        bst.insert(i, i);
    }

    cout << bst.str() << endl;

    auto it = bst.find(5);

    cout << it->value << endl;
    auto bst_old = bst;
    bst.erase(it);

    cout << bst.str() << endl;
    cout << bst_old.str() << endl;
    return 0;
}
