// Tiny test helpers so the tests don't need a framework installed.
// CHECK keeps going after a failure so one run shows every broken check, and main()
// returns nonzero if anything failed (which is what make / CI look at).

#pragma once

#include <cstdio>

inline int g_failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);    \
            ++g_failures;                                                    \
        }                                                                    \
    } while (0)

// passes if the expression throws an exception of type `exc`
#define CHECK_THROWS(expr, exc)                                              \
    do {                                                                     \
        bool threw_ = false;                                                 \
        try {                                                                \
            (void)(expr);                                                    \
        } catch (const exc&) {                                               \
            threw_ = true;                                                   \
        }                                                                    \
        if (!threw_) {                                                       \
            std::printf("  FAIL %s:%d  %s didn't throw %s\n", __FILE__,      \
                        __LINE__, #expr, #exc);                              \
            ++g_failures;                                                    \
        }                                                                    \
    } while (0)

#define RUN(test)                                                            \
    do {                                                                     \
        int before_ = g_failures;                                            \
        test();                                                              \
        std::printf("%s %s\n", g_failures == before_ ? "ok  " : "FAIL", #test); \
    } while (0)
