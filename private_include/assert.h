#ifndef __DUMMY_ASSERT_H
#define __DUMMY_ASSERT_H
#ifdef NDEBUG
#define assert(x) ((void)(0))
#else
#include_next <assert.h>
#endif
#endif

