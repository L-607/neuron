/**
 * NEURON IIoT System for Industry 4.0
 * Copyright (C) 2020-2022 EMQ Technologies Co., Ltd All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 **/

/**
 * Cygwin-specific global variable definitions.
 *
 * On Linux, global variables shared between the main executable and plugin
 * DSOs are defined in src/main.c and exported via the --dynamic-list-data
 * linker flag.  On Cygwin/Windows PE, DLLs cannot import data symbols from
 * the parent executable; all globals that plugin DLLs reference must live
 * inside neuron-base.dll (this file).
 *
 * src/main.c guards its own copies with #ifndef __CYGWIN__ to avoid
 * duplicate-symbol errors.
 */

#ifdef __CYGWIN__

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "define.h"       /* neu_manager_t, ZLOG_LEVEL_*, bool, ... */
#include "utils/zlog.h"   /* ZLOG_LEVEL_NOTICE                       */

/* ---- define.h externs -------------------------------------------------- */
neu_manager_t *g_manager         = NULL;
bool           disable_jwt       = false;
int            default_log_level = ZLOG_LEVEL_NOTICE;
char           host_port[32]     = { 0 };
char           g_status[32]      = { 0 };

/* ---- zlog category (plugin DLLs reference this from neuron-base.dll) --- */
zlog_category_t *neuron          = NULL;

/* ---- metrics.h / plugin.h externs --------------------------------------- */
int64_t        global_timestamp  = 0;

/* ---- src/adapter/driver/cache.c local extern ---------------------------- */
bool           sub_filter_err    = false;

#endif /* __CYGWIN__ */
