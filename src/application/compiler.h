#ifndef COMPILER_H
#define COMPILER_H

#define _VARIADIC_SEL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, x, ...) x

#define VARIADIC(_N, ...)					\
	_VARIADIC_SEL(__VA_ARGS__ __VA_OPT__(,) _N ## 10,	\
		      _N ## 9, _N ## 8, _N ## 7, _N ## 6,	\
		      _N ## 5, _N ## 4, _N ## 3, _N ## 2,	\
		      _N ## 1, _N ## 0)(__VA_ARGS__)

#define ARRAY_SIZE(__x) (sizeof((__x)) / sizeof((__x)[0]))

#define for_each(__m, __a)				\
	for (typeof((__a)[0]) *(__m) = &(__a)[0];	\
	     (__m) < &(__a)[ARRAY_SIZE((__a))];		\
	     ++(__m))

#define for_each_const(__m, __a)			\
	for (const typeof((__a)[0]) *(__m) = &(__a)[0];	\
	     (__m) < &(__a)[ARRAY_SIZE((__a))];		\
	      ++(__m))

#define unreachable() __builtin_unreachable()
#define fallthrough __attribute__((__fallthrough__))

#define __force_inline inline __attribute__((__always_inline__))

#define compiletime_assert(condition, msg) \
	_compiletime_assert(condition, msg, __compiletime_assert_, __COUNTER__)
#define _compiletime_assert(condition, msg, prefix, suffix) \
	__compiletime_assert(condition, msg, prefix, suffix)
# define __compiletime_assert(condition, msg, prefix, suffix)			\
	do {									\
		extern void prefix ## suffix(void) __compiletime_error(msg);	\
		if (!(condition))						\
			prefix ## suffix();					\
	} while (0)
#define __compiletime_error(message) __attribute__((__error__(message)))

typedef _Bool bool;

#endif /* COMPILER_H */
