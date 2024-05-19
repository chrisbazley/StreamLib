/*
 * StreamLib: Miscellaneous macro definitions
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

/*
History:
  CJB: 04-Nov-18: Created this source file.
  CJB: 26-Aug-19: Now requires "PseudoFlex.h" too.
  CJB: 07-Jun-20: Added support for verbose debugging output.
*/

#ifndef StreamMisc_h
#define StreamMisc_h

#ifdef FORTIFY
#include "fortify.h"
#endif

#ifdef USE_CBDEBUG

#include "Debug.h"
#include "PseudoIO.h"
#include "PseudoFlex.h"

#else /* USE_CBDEBUG */

#include <stdio.h>
#include <assert.h>

#if defined(DEBUG_VERBOSE_OUTPUT) && defined (DEBUG_OUTPUT)
#define DEBUG_VERBOSEF if (1) printf
#else
#define DEBUG_VERBOSEF if (0) printf
#endif /* DEBUG_VERBOSE_OUTPUT && DEBUG_OUTPUT */

#ifdef DEBUG_OUTPUT
#define DEBUGF if (1) printf
#else
#define DEBUGF if (0) printf
#endif /* DEBUG_OUTPUT */

#endif /* USE_CBDEBUG */

#define NOT_USED(x) ((void)(x))

#endif /* StreamMisc_h */
