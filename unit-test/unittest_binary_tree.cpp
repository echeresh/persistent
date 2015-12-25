#include "gtest/gtest.h"
#include "persistent.h"

static persistent::binary_tree<int, int> construct_random_tree(int size)
{
    persistent::binary_tree<int, int> bst;
    for (int i = 0; i < size; i++)
    {
        int key = rand() % 100;
        int value = rand() % 100;
        bst.insert(key, value);
    }
    return bst;
}

TEST(test_binary_tree, test_construction1)
{
    const int size = 5;
    auto bst = construct_random_tree(size);
    ASSERT_TRUE(bst.size() == size);
    bst.insert(100, 0);
    bst.insert(100, 0);
    ASSERT_TRUE(bst.size() == size + 1);
    bst.erase(bst.find(100));
    ASSERT_TRUE(bst.size() == size);
}

TEST(test_binary_tree, test_construction2)
{
    const int size = 100;
    auto bst = construct_random_tree(0);
    ASSERT_TRUE(bst.size() == 0);
    for (int i = 0; i < size; i++)
    {
        bst.insert(i, i);
        ASSERT_TRUE(bst.size() == i + 1);
    }

    for (int i = 0; i < size; i++)
    {
        auto it = bst.find(i);
        ASSERT_TRUE(it != bst.end());
        bst.erase(it);
        ASSERT_TRUE(bst.size() == size - i - 1);
    }
}

TEST(test_binary_tree, test_construction3)
{
    persistent::binary_tree<int, int> bst;
    auto v0 = bst.insert(1, 0).get_version();
    auto v1 = bst.insert(1, 1).get_version();
    ASSERT_TRUE(bst[1] == 1);
    ASSERT_TRUE(v0 != v1);
    auto v2 = bst.insert(1, 2).get_version();
    ASSERT_TRUE(bst[1] == 2);
    ASSERT_TRUE(bst.create_with_version(v0)[1] == 0);
    ASSERT_TRUE(bst.create_with_version(v1)[1] == 1);
    ASSERT_TRUE(bst.create_with_version(v2)[1] == 2);
}

TEST(test_binary_tree, test_iterator)
{
    persistent::binary_tree<int, int> bst;
    ASSERT_TRUE(bst.begin() == bst.end());

    const int n = 10;
    for (int i = 0; i < n; i++)
    {
        bst.insert(i, i + 1);
    }
    ASSERT_EQ(bst.size(), n);
    for (auto& e : bst)
    {
        ASSERT_EQ(e.key + 1, e.value);
    }
}

TEST(test_binary_tree, test_insert)
{
    const int size = 10;
    persistent::binary_tree<int, int> bst;
    for (int i = 0; i < size; i++)
    {
        bst.insert(i, -i);
    }
    ASSERT_TRUE(bst.size() == size);
    for (auto& e : bst)
    {
        ASSERT_TRUE(e.key == -e.value);
    }
}

TEST(test_binary_tree, test_erase)
{
    persistent::binary_tree<int, int> bst;
    const int n = 10;
    for (int i = 0; i < n; i++)
    {
        bst.insert(i, i);
    }
    ASSERT_EQ(bst.size(), n);

    auto begin_key = bst.begin()->key;
    ASSERT_FALSE(bst.find(begin_key) == bst.end());
    bst.erase(bst.begin());

    ASSERT_TRUE(bst.find(begin_key) == bst.end());
    ASSERT_EQ(bst.size(), n - 1);
}

TEST(test_binary_tree, test_nested1)
{
    persistent::binary_tree<int, persistent::binary_tree<int, int>> bst;
    persistent::binary_tree<int, int> nested_bst;
    auto v0 = bst.insert(0, nested_bst).get_version();
    ASSERT_EQ(bst.find(0)->value.size(), 0);

    auto nested = bst.find(0)->value;
    auto pv = nested.get_parent_version();
    ASSERT_EQ(v0, pv);

    nested.insert(0, 0);
    pv = nested.get_parent_version();

    ASSERT_NE(v0, pv);
    ASSERT_EQ(bst.find(0)->value.size(), 1);

    bst.set_version(v0);
    ASSERT_EQ(bst.find(0)->value.size(), 0);
}

TEST(test_binary_tree, test_nested2)
{
    int size = 10;
    persistent::binary_tree<int, persistent::binary_tree<double, int>> bst;

    for (int i = 0; i < size; i++)
    {
        persistent::binary_tree<double, int> dbst;
        auto dkey = i / (double)size;
        dbst.insert(dkey, i);
        bst.insert(i, dbst);
    }

    persistent::version init_version = bst.get_version();
    persistent::version last_version;
    for (int i = 0; i < size; i++)
    {
        auto dkey = i / (double)size;
        auto vbst = bst.find(i)->value;
        vbst.insert(dkey, i + 1);
        last_version = vbst.get_parent_version();
        bst.set_version(last_version);
    }

    bst.set_version(init_version);

    for (int i = 0; i < size; i++)
    {
        auto val = bst[i];
        ASSERT_TRUE(val.begin()->value == i);
    }

    bst.set_version(last_version);

    for (int i = 0; i < size; i++)
    {
        auto val = bst[i];
        ASSERT_TRUE(val.begin()->value == i + 1);
    }
}
