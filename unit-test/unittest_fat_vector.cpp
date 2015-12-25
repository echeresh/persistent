#include "gtest/gtest.h"
#include "persistent.h"

TEST(test_fat_vector, test_construction)
{
    persistent::fat_vector<int> v;
    v.resize(10);
    ASSERT_TRUE(v.size() == 10);
}

TEST(test_fat_vector, test_conversion)
{
    const int n = 10;
    persistent::linked_list<int, true> l;
    for (int i = 0; i < n; i++)
    {
        l.push_front(i);
    }

    persistent::fat_vector<int> v(l);
    v.push_back(101);
    auto it = v.begin();
    ++it;
    ++it;
    v.erase(it);
    for (int i : v)
    {
        cout << i << endl;
    }
}
