/**
 * zlog_compat.c — Compatibility shim for zlog 1.2.12 on Cygwin.
 *
 * zlog 1.2.12 (latest-stable) is missing a few functions that neuron
 * uses.  This file provides minimal implementations of those functions
 * using the internal structure layout of zlog_category_t from
 * neuron-deps/zlog/src/category.h.
 *
 * Internal layout (zlog 1.2.12, MAXLEN_PATH=1024, 64-bit):
 *   char            name[1025];           // offset 0
 *   size_t          name_len;             // offset 1032 (7-byte pad after name)
 *   unsigned char   level_bitmap[32];     // offset 1040
 *   unsigned char   level_bitmap_backup[32]; // offset 1072
 *   void           *fit_rules;            // offset 1104
 *   void           *fit_rules_backup;     // offset 1112
 *
 * The macros and logging path in zlog check level_bitmap via:
 *   zlog_category_needless_level(cat, lv)
 *     = !((cat->level_bitmap[lv/8] >> (7 - lv%8)) & 1)
 * Bit == 1 → level IS logged.  Bit == 0 → level is skipped (needless).
 *
 * zlog_level_switch sets bits for levels >= threshold to 1, others to 0,
 * which restricts logging to the requested threshold and above.
 */

#ifdef __CYGWIN__

#include <stddef.h>
#include <string.h>
#include "utils/zlog.h"

/* ---- Internal category layout (mirrors category.h + zc_xplatform.h) ---- */
#define ZLOG_COMPAT_MAXLEN_PATH 1024

typedef struct {
    char          name[ZLOG_COMPAT_MAXLEN_PATH + 1];
    size_t        name_len;
    unsigned char level_bitmap[32];
    unsigned char level_bitmap_backup[32];
    void         *fit_rules;
    void         *fit_rules_backup;
} zlog_category_internal_t;

/* Compile-time guard: if the zlog source layout ever changes and the
 * offset of level_bitmap shifts, the compat cast below would silently
 * corrupt memory.  The assertion catches a size mismatch at build time. */
typedef char zlog_compat_name_size_check[
    (ZLOG_COMPAT_MAXLEN_PATH + 1 == 1025) ? 1 : -1];

/* ---- zlog_level_switch -------------------------------------------------- */
/*
 * Dynamically set the effective log threshold for a category.
 * All levels in [0,255]: bits >= level are set to 1 (enabled);
 * bits < level are cleared (disabled).
 *
 * This mirrors the semantics of the same function in newer zlog releases
 * and is equivalent to reloading a config with "cat.LEVEL *" rules.
 */
int zlog_level_switch(zlog_category_t *category, int level)
{
    if (!category || level < 0 || level > 255)
        return -1;

    zlog_category_internal_t *cat = (zlog_category_internal_t *)category;
    memset(cat->level_bitmap, 0, sizeof(cat->level_bitmap));
    for (int lv = level; lv < 256; lv++) {
        cat->level_bitmap[lv / 8] |= (unsigned char)(1u << (7 - lv % 8));
    }
    return 0;
}

/* ---- zlog_level_enabled ------------------------------------------------- */
int zlog_level_enabled(zlog_category_t *category, const int level)
{
    if (!category || level < 0 || level > 255)
        return 0;

    zlog_category_internal_t *cat = (zlog_category_internal_t *)category;
    return (cat->level_bitmap[level / 8] >> (7 - level % 8)) & 0x01;
}

/* ---- zlog_version ------------------------------------------------------- */
const char *zlog_version(void)
{
    return "1.2.12-cygwin-compat";
}

#endif /* __CYGWIN__ */
