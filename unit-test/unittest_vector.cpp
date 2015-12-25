#include "gtest/gtest.h"
#include "persistent.h"

TEST(test_vector, test_construction)
{
    persistent::vector<int> v;
    v.resize(10);
    ASSERT_TRUE(true);
}

TEST(test_vector, test_push_back)
{
    persistent::vector<int> v;
    persistent::version ver0 = v.get_version();
    v.push_back(1);
    persistent::version ver1 = v.get_version();
    ASSERT_TRUE(ver0 != ver1);
    ASSERT_TRUE(v.size() == 1);
    ASSERT_TRUE(v[0] == 1);
    v.set_version(ver0);
    ASSERT_TRUE(v.size() == 0);
}

TEST(test_vector, test_erase)
{
    const int n = 10;
    persistent::vector<int> v;
    persistent::version ver0 = v.get_version();
    persistent::version ver1;
    persistent::version ver = v.get_version();
    for (int i = 0; i < n; i++)
    {
        v.push_back(i);
        ASSERT_TRUE(v.get_version() != ver);
        ver = v.get_version();
        if (i == 0)
        {
            ver1 = ver;
        }
    }
    ASSERT_TRUE(v.size() == n);
    v.set_version(ver0);
    ASSERT_TRUE(v.size() == 0);
    v.set_version(ver1);
    ASSERT_TRUE(v.size() == 1);
}

TEST(test_vector, test_nested)
{
    persistent::vector<persistent::vector<int>> v;
    persistent::vector<int> v1;
    persistent::vector<int> v2;
    v.push_back(v1);
    persistent::version ver1 = v.get_version();
    v.push_back(v2);
    persistent::version ver2 = v.get_version();
    v1 = *v.begin();
    v1.push_back(1);
    ASSERT_TRUE(v[0].size() == 1);
    v.set_version(ver2);
    ASSERT_TRUE(v[0].size() == 0);
}

TEST(test_vector, test_undo_redo)
{
    persistent::vector<int> v1;
    persistent::vector<int> v2;

    for (int i = 0; i < 10; i++)
    {
        v1.push_back(i);
        v2.push_back(i);
    }

    v1.push_back(-1);
    v1.undo();

    ASSERT_TRUE(v1.size() == v2.size());

    v1.redo();
    ASSERT_TRUE(v1[(int)v1.size() - 1] == -1);

    v1.undo();
    v1.push_back(1);
    v1.push_back(2);

    ASSERT_EQ(v1.size(), 12);
}
