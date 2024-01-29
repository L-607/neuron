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

/*
 * DO NOT EDIT THIS FILE MANUALLY!
 * It was automatically generated by `json-autotype`.
 */

#ifndef _NEU_JSON_API_NEU_JSON_TAG_H_
#define _NEU_JSON_API_NEU_JSON_TAG_H_

#include "json/json.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *           address;
    char *           name;
    char *           description;
    int64_t          type;
    int64_t          attribute;
    int64_t          precision;
    double           decimal;
    neu_json_type_e  t;
    neu_json_value_u value;
} neu_json_tag_t;

int  neu_json_encode_tag(void *json_obj, void *param);
int  neu_json_decode_tag_json(void *json_obj, neu_json_tag_t *tag_p);
void neu_json_decode_tag_fini(neu_json_tag_t *tag);
int  neu_json_tag_check_type(neu_json_tag_t *tag);

typedef struct {
    int             len;
    neu_json_tag_t *tags;
} neu_json_tag_array_t;

int  neu_json_encode_tag_array(void *json_obj, void *param);
int  neu_json_decode_tag_array_json(void *json_obj, neu_json_tag_array_t *arr);
void neu_json_decode_tag_array_fini(neu_json_tag_array_t *arr);

typedef struct {
    char *          node;
    char *          group;
    int             n_tag;
    neu_json_tag_t *tags;
} neu_json_add_tags_req_t;

int  neu_json_decode_add_tags_req(char *buf, neu_json_add_tags_req_t **result);
void neu_json_decode_add_tags_req_free(neu_json_add_tags_req_t *req);

typedef struct {
    uint16_t index;
    int      error;
} neu_json_add_tag_res_t, neu_json_update_tag_res_t;

int neu_json_encode_au_tags_resp(void *json_object, void *param);

typedef struct {
    char *          group;
    int             n_tag;
    int             interval;
    neu_json_tag_t *tags;
} neu_json_gtag_t;

int  neu_json_decode_gtag_json(void *json_obj, neu_json_gtag_t *gtag_p);
void neu_json_decode_gtag_fini(neu_json_gtag_t *gtag);

typedef struct {
    int              len;
    neu_json_gtag_t *gtags;
} neu_json_gtag_array_t;

int neu_json_decode_gtag_array_json(void *json_obj, neu_json_gtag_array_t *arr);
void neu_json_decode_gtag_array_fini(neu_json_gtag_array_t *arr);

typedef struct {
    char *           node;
    int              n_group;
    neu_json_gtag_t *groups;
} neu_json_add_gtags_req_t;

int neu_json_decode_add_gtags_req(char *buf, neu_json_add_gtags_req_t **result);
void neu_json_decode_add_gtags_req_free(neu_json_add_gtags_req_t *req);

typedef struct {
    uint16_t index;
    int      error;
} neu_json_add_gtag_res_t, neu_json_update_gtag_res_t;

int neu_json_encode_au_gtags_resp(void *json_object, void *param);

typedef char *neu_json_del_tags_req_name_t;

typedef struct {
    char *                        node;
    char *                        group;
    int                           n_tags;
    neu_json_del_tags_req_name_t *tags;
} neu_json_del_tags_req_t;

int  neu_json_decode_del_tags_req(char *buf, neu_json_del_tags_req_t **result);
void neu_json_decode_del_tags_req_free(neu_json_del_tags_req_t *req);

typedef struct {
    int             n_tag;
    neu_json_tag_t *tags;
} neu_json_get_tags_resp_t;

int neu_json_encode_get_tags_resp(void *json_object, void *param);

typedef struct {
    char *          node;
    char *          group;
    int             n_tag;
    neu_json_tag_t *tags;
} neu_json_update_tags_req_t;

int  neu_json_decode_update_tags_req(char *                       buf,
                                     neu_json_update_tags_req_t **result);
void neu_json_decode_update_tags_req_free(neu_json_update_tags_req_t *req);

#ifdef __cplusplus
}
#endif

#endif
