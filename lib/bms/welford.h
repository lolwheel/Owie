// From https://github.com/patrickmineault/welford/blob/master/src/Welford.h
// A basic implementation of Welford's algorithm for numerically stable
// incremental mean and variance computation.
// Usage example:
// Welford<float> accum = Welford<float>{};
// for(int i = 0; i < 100; i++) {
//     accum.add_value(float(i));
// }
// accum.mean() // = 50
// accum.var() // = 10000 / 12
#include <math.h>

template <class T>
class Welford {
 public:
  void add_value(T x) {
    n++;
    T delta = x - m;
    m += delta / T(n);
    T delta2 = x - m;
    ss += delta * delta2;
  }

  float mean() const { return m; }

  float var() const {
    // Unbiased estimate.
    return ss / T(n - 1);
  }

  float sd() const { return sqrt(ss / T(n - 1)); }

 private:
  // Mean.
  T m = 0;
  // Sum-of-squares.
  T ss = 0;
  // N.
  int n = 0;
};