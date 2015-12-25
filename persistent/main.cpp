#include <iostream>
#include <memory>
#include <list>
#include "persistent.h"
using namespace std;

int main()
{
    persistent::vector<int> v;
    persistent::version ver0 = v.get_version();
    v.push_back(1);
    persistent::version ver1 = v.get_version();
    v.set_version(ver0);
    return 0;
}
