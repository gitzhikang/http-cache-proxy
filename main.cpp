#include <iostream>

#include "proxy.h"
int main()
{
    Proxy proxy("12345","log.txt");
    proxy.run(64);
}
