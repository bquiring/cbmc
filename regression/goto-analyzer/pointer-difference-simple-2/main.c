#include <assert.h>

int main()
{
  int a[2];

  int *p = &(a[0]);
  int *q = &(a[1]);
  int *r = p + 1;

  assert(p == q);
  assert(r == q);

  assert(p != q);
  assert(r != q);
}
