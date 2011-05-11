
// compile with gcc -x c -shared test2.cpp -o test2.o
// -c mucks it up, even though the right symbol is in there


#include <stdio.h>

int
bogus()
{
  printf("hi\n");
}
