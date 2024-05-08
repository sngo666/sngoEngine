#ifndef __SNGO_CHECK_H
#define __SNGO_CHECK_H

#define CHECK(x) (!(!(x) && (LOG_FATAL("Check failed: %s", #x), true)))

#define CHECK_EQ(a, b) CHECK_IMPL(a, b, ==)
#define CHECK_NE(a, b) CHECK_IMPL(a, b, !=)
#define CHECK_GT(a, b) CHECK_IMPL(a, b, >)
#define CHECK_GE(a, b) CHECK_IMPL(a, b, >=)
#define CHECK_LT(a, b) CHECK_IMPL(a, b, <)
#define CHECK_LE(a, b) CHECK_IMPL(a, b, <=)

#define CHECK_IMPL(a, b, op)                                                                    \
  do                                                                                            \
    {                                                                                           \
      auto va = a;                                                                              \
      auto vb = b;                                                                              \
      if (!(va op vb))                                                                          \
        LOG_FATAL("Check failed: %s " #op " %s with %s = %s, %s = %s", #a, #b, #a, va, #b, vb); \
  } while (false)



#endif
