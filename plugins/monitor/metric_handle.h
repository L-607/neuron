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

#ifndef NEU_PLUGIN_MONITOR_METRIC_H
#define NEU_PLUGIN_MONITOR_METRIC_H

#include <nng/nng.h>

#include "json/neu_json_fn.h"

#define VALIDATE_JWT(aio)                                                \
    {                                                                    \
        if (!disable_jwt) {                                              \
            char *jwt =                                                  \
                (char *) http_get_header(aio, (char *) "Authorization"); \
                                                                         \
            NEU_JSON_RESPONSE_ERROR(neu_jwt_validate(jwt), {             \
                if (error_code.error != NEU_ERR_SUCCESS) {               \
                    http_response(aio, error_code.error, result_error);  \
                    free(result_error);                                  \
                    return;                                              \
                }                                                        \
            });                                                          \
        }                                                                \
    }

void handle_get_metric(nng_aio *aio);

#endif