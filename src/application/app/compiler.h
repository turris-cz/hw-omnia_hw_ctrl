#ifndef COMPILER_H
#define COMPILER_H

#define ARRAY_SIZE(__x) (sizeof((__x)) / sizeof((__x)[0]))

#define for_each(__m, __a)			\
    for (typeof((__a)[0]) *(__m) = &(__a)[0];	\
         (__m) < &(__a)[ARRAY_SIZE((__a))];	\
         ++(__m))

#define for_each_const(__m, __a)			\
    for (const typeof((__a)[0]) *(__m) = &(__a)[0];	\
         (__m) < &(__a)[ARRAY_SIZE((__a))];		\
         ++(__m))

#define unreachable() __builtin_unreachable()

#define __force_inline inline __attribute__((__always_inline__))

typedef _Bool bool;

#endif /* COMPILER_H */
