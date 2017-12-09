#include <stdio.h>

const char* file = "/usr/local/service/readme.txt";
int main()
{
    unlink(file);
    return 0;
}
