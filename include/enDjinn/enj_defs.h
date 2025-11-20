#ifndef ENJ_DEFS_H
#define ENJ_DEFS_H

/*! \file
 *  \brief   Basic definitions for enDjinn library.
 *
 *  This file contains basic definitions, macros, and
 *  includes that are used throughout the enDjinn library.
 *
 *  \author    2025 Daniel Fairchild
 */

#include <stdio.h>

#ifdef ENJ_DEBUG
#include <dc/perf_monitor.h>
#endif

#ifndef ENJ_MODE_STACK_SIZE
#define ENJ_MODE_STACK_SIZE 16
#endif

#ifndef ENJ_SHOWFRAMETIMES
#define ENJ_SHOWFRAMETIMES 0
#endif

#ifndef ENJ_SUPERSAMPLING
#define ENJ_SUPERSAMPLING 1
#endif

#if ENJ_SUPERSAMPLING == 1
#define ENJ_XSCALE 2.0f
#else
#define ENJ_XSCALE 1.0f
#endif

#ifdef ENJ_DEBUG
#define ENJ_DEBUG_PRINT(...)                                                       \
do {                                                                         \
  fprintf(stdout, __VA_ARGS__);                                              \
} while (0)
#else
#define ENJ_DEBUG_PRINT(...)                                                       \
do {                                                                         \
} while (0)
#endif

#ifndef ENJ_CBASEPATH
// fail build
#error "ENJ_CBASEPATH not defined"
#endif
#endif // ENJ_DEFS_H