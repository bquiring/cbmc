#include <assert.h>
#include <math.h>
#include <float.h>

int main() {
  double xxx;

  // Visual Studio needs to be 2013 onwards
  #if defined(_MSC_VER) && !defined(__CYGWIN__) && _MSC_VER < 1800

  // see http://www.johndcook.com/math_h.html

  #else
  assert(fpclassify(DBL_MAX+DBL_MAX)==FP_INFINITE);
  assert(fpclassify(0*(DBL_MAX+DBL_MAX))==FP_NAN);
  assert(fpclassify(1.0)==FP_NORMAL);
  assert(fpclassify(DBL_MIN)==FP_NORMAL);
  assert(fpclassify(DBL_MIN/2)==FP_SUBNORMAL);
  assert(fpclassify(-0.0)==FP_ZERO);
  #endif

  #if !defined(__clang__) && defined(__GNUC__)
  assert(__builtin_fpclassify(0, 1, 2, 3, 4, DBL_MAX+DBL_MAX)==1);
  assert(__builtin_fpclassify(0, 1, 2, 3, 4, 0*(DBL_MAX+DBL_MAX))==0);
  assert(__builtin_fpclassify(0, 1, 2, 3, 4, 1.0)==2);
  assert(__builtin_fpclassify(0, 1, 2, 3, 4, DBL_MIN)==2);
  assert(__builtin_fpclassify(0, 1, 2, 3, 4, DBL_MIN/2)==3);
  assert(__builtin_fpclassify(0, 1, 2, 3, 4, -0.0)==4);

  // these are compile-time
  _Static_assert(__builtin_fpclassify(0, 1, 2, 3, 4, -0.0)==4,
                 "__builtin_fpclassify is constant");
  #endif

  assert(signbit(-1.0)!=0);
  assert(signbit(1.0)==0);
}
