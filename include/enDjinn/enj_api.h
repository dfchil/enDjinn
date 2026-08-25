#ifndef ENJ_API_H
#define ENJ_API_H

/* Public enDjinn headers can be included directly from C or C++. */
#ifdef __cplusplus
#define ENJ_BEGIN_DECLS extern "C" {
#define ENJ_END_DECLS }
#else
#define ENJ_BEGIN_DECLS
#define ENJ_END_DECLS
#endif

#endif // ENJ_API_H
