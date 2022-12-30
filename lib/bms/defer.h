// Taken from
// https://stackoverflow.com/questions/32432450/what-is-standard-defer-finalizer-implementation-in-c
#ifndef DEFER_H
struct defer_dummy {};

template <class F>
struct deferrer {
  F f;
  ~deferrer() { f(); }
};
template <class F>
deferrer<F> operator*(defer_dummy, F f) {
  return {f};
}
#define DEFER_(LINE) zz_defer##LINE
#define DEFER(LINE) DEFER_(LINE)
#define defer auto DEFER(__LINE__) = defer_dummy{} *[&]()
#endif  // DEFER_H
