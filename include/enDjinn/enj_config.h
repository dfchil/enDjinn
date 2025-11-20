/*! \file
 *  \brief   Configuration settings for enDjinn library.
 *
 *  This file contains configuration settings, macros, and
 *  definitions that control the behavior of the enDjinn library.
 *
 *  \author    2025 Daniel Fairchild
 *  \copyright MIT License
 */
#ifndef ENDJINN_ENJ_CONFIG_H
#define ENDJINN_ENJ_CONFIG_H

typedef enum {
  ENJ_LOGLEVEL_NONE = 0,
  ENJ_LOGLEVEL_ERROR,
  ENJ_LOGLEVEL_WARNING,
  ENJ_LOGLEVEL_INFO,
  ENJ_LOGLEVEL_ENJ_DEBUG,
} enj_loglevel_e;

#define ENJ_DEFAULT_LOGLEVEL ENJ_LOGLEVEL_ENJ_DEBUG

typedef struct enj_config_s {
  enj_loglevel_e loglevel;

} enj_config_t;

#endif // ENDJINN_ENJ_CONFIG_H