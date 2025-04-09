/*
 * StreamLib test: Macro and test suite definitions
 * Copyright (C) 2018 Christopher Bazley
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef Tests_h
#define Tests_h

#ifdef FORTIFY
#include "fortify.h"
#else
#define Fortify_SetAllocationLimit(x)
#define Fortify_SetNumAllocationsLimit(x)
#define Fortify_EnterScope()
#define Fortify_LeaveScope()
#define Fortify_OutputStatistics()
#endif

#ifdef USE_CBDEBUG

#include "Debug.h"
#include "PseudoIO.h"
#include "PseudoFlex.h"

#else /* USE_CBDEBUG */

#include <assert.h>

#define DEBUG_SET_OUTPUT(output_mode, log_name)

#endif /* USE_CBDEBUG */

#ifdef USE_OPTIONAL
#include <stdlib.h>

#undef NULL
#define NULL ((_Optional void *)0)

static inline void optional_free(_Optional void *x)
{
    free((void *)x);
}
#undef free
#define free(x) optional_free(x)

static inline _Optional void *optional_malloc(size_t n)
{
    return malloc(n);
}
#undef malloc
#define malloc(n) optional_malloc(n)

#else
#define _Optional
#endif

#define NOT_USED(x) ((void)(x))
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

extern void Reader_tests(void);
extern void ReaderNull_tests(void);
extern void Writer_tests(void);
extern void WriterGKC_tests(void);

#endif /* Tests_h */
