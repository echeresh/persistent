#include "gtest/gtest.h"
#include "persistent.h"

TEST(test_version_tree, test_insertion)
{
    persistent::version_tree<int> vtree;
    auto root_version = vtree.root_version();
    vtree.update(root_version, 1);
    auto v2 = vtree.insert(root_version, 2);
    ASSERT_LE(root_version, v2);
}
