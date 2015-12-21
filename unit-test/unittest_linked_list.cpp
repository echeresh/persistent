#include <algorithm>
#include "gtest/gtest.h"
#include "persistent.h"
using namespace persistent;

static linked_list<int> construct_random_list(int size)
{
    linked_list<int> l;
    for (int i = 0; i < size; i++)
    {
        int value = rand() % 100;
        l.push_front(value);
    }
    return l;
}

TEST(test_linked_list, test_construction1)
{
    const int size = 5;
    auto l = construct_random_list(size);
    ASSERT_TRUE(l.size() == size);
    l.push_front(0);
    l.push_front(0);
    ASSERT_TRUE(l.size() == size + 2);
    l.erase(l.find(0));
    ASSERT_TRUE(l.size() == size + 1);
}

TEST(test_linked_list, test_construction2)
{
    const int size = 5;
    auto l = construct_random_list(size);
    auto old_version = l.get_version();
    for (int i = 0; i < size; i++)
    {
        l.erase(l.begin());
    }
    cout << l.size() << endl;
    ASSERT_TRUE(l.size() == 0);
    l.set_version(old_version);
    
    ASSERT_TRUE(l.size() == size);
}

TEST(test_linked_list, test_iteration)
{
    const int size = 5;
    linked_list<int> l;
    for (int i = 0; i < size; i++)
    {
        l.push_front(i);
    }
    int i = 0;
    for (auto e : l)
    {
        ASSERT_EQ(e, size - i - 1);
        i++;
    }
}

TEST(test_linked_list, test_erase)
{
    const int size = 5;
    auto l = construct_random_list(size);
    std::vector<int> v;
    for (auto e : l)
    {
        v.push_back(e);
    }

    for (int i = 0; i < size; i++)
    {
        ASSERT_EQ(*l.begin(), v[i]);
        l.erase(l.begin());
    }
}
