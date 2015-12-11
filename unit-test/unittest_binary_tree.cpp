#include "gtest/gtest.h"
#include "persistent.h"

TEST(test_binary_tree, test_initialization)
{
    persistent::binary_tree<int, int> bst;
    auto v0 = bst.insert(1, 1).get_version();
    auto v1 = bst.insert(1, 2).get_version();
    ASSERT_TRUE(bst[1] == 2);
    ASSERT_TRUE(bst.create_by_version(v0)[1] == 1);
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
    persistent::binary_tree<int, int> bst;
    for (int i = 0; i < 10; i++)
    {
        bst.insert(i, i);
    }
    for (auto& e : bst)
    {

    }
    ASSERT_TRUE(true);
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

TEST(test_binary_tree, nested)
{
    persistent::binary_tree<int, persistent::binary_tree<int, int>> bst;
    persistent::binary_tree<int, int> nbst;
    auto v0 = bst.insert(0, nbst).get_version();
    auto nested = bst.find(0).value_by_value();
    auto pv = nested.get_parent_version();
    ASSERT_EQ(v0, pv);

    nested.insert(0, 0);
    pv = nested.get_parent_version();

    ASSERT_NE(v0, pv);
    ASSERT_EQ(bst.find(0)->value.size(), 0);

    bst.switch_version2(pv);
    ASSERT_EQ(bst.find(0)->value.size(), 1);
    
}