#include "utils/zlog.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* Stub implementation of zlog for Windows porting */

struct zlog_category_s {
    char name[64];
};

static zlog_category_t s_default_category;

/* On Cygwin, neuron-base is a DLL and cannot import data symbols from the
 * parent executable (unlike ELF shared libraries on Linux). Define the
 * `neuron` log category pointer here so it is part of the DLL itself.
 * main.c guards its own definition with #ifndef __CYGWIN__. */
zlog_category_t *neuron = NULL;

int zlog_init(const char *config) {
    (void)config;
    // Just simple init
    strcpy(s_default_category.name, "default");
    return 0;
}

int zlog_reload(const char *config) {
    (void)config;
    return 0;
}

void zlog_fini(void) {
}

void zlog_profile(void) {
}

zlog_category_t *zlog_get_category(const char *cname) {
    return &s_default_category;
}

int zlog_level_enabled(zlog_category_t *category, const int level) {
    (void)category;
    (void)level;
    return 1;
}

int zlog_put_mdc(const char *key, const char *value) {
    (void)key;
    (void)value;
    return 0;
}

char *zlog_get_mdc(const char *key) {
    (void)key;
    return NULL;
}

void zlog_remove_mdc(const char *key) {
    (void)key;
}

void zlog_clean_mdc(void) {
}

int zlog_level_switch(zlog_category_t * category, int level) {
    (void)category;
    (void)level;
    return 0;
}

void zlog(zlog_category_t * category,
	const char *file, size_t filelen,
	const char *func, size_t funclen,
	long line, int level,
	const char *format, ...) {
    
    (void)category;
    (void)file; (void)filelen;
    (void)func; (void)funclen;
    (void)line;

    va_list args;
    va_start(args, format);
    
    const char *lvl_str = "UNK";
    switch(level) {
        case ZLOG_LEVEL_DEBUG: lvl_str = "DEBUG"; break;
        case ZLOG_LEVEL_INFO: lvl_str = "INFO"; break;
        case ZLOG_LEVEL_NOTICE: lvl_str = "NOTICE"; break;
        case ZLOG_LEVEL_WARN: lvl_str = "WARN"; break;
        case ZLOG_LEVEL_ERROR: lvl_str = "ERROR"; break;
        case ZLOG_LEVEL_FATAL: lvl_str = "FATAL"; break;
    }

    // Windows console output
    fprintf(stderr, "[%s] ", lvl_str);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");

    va_end(args);
}

void vzlog(zlog_category_t * category,
	const char *file, size_t filelen,
	const char *func, size_t funclen,
	long line, int level,
	const char *format, va_list args) {
    
    (void)category;
    (void)file; (void)filelen;
    (void)func; (void)funclen;
    (void)line;

    const char *lvl_str = "UNK";
    switch(level) {
        case ZLOG_LEVEL_DEBUG: lvl_str = "DEBUG"; break;
        case ZLOG_LEVEL_INFO: lvl_str = "INFO"; break;
        case ZLOG_LEVEL_NOTICE: lvl_str = "NOTICE"; break;
        case ZLOG_LEVEL_WARN: lvl_str = "WARN"; break;
        case ZLOG_LEVEL_ERROR: lvl_str = "ERROR"; break;
        case ZLOG_LEVEL_FATAL: lvl_str = "FATAL"; break;
    }
    
    fprintf(stderr, "[%s] ", lvl_str);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
}

/* dzlog functions are used if zlog.h was built with default zlog support, stubbing them too just in case */

int dzlog_init(const char *confpath, const char *cname) { return 0; }
int dzlog_set_category(const char *cname) { return 0; }
void dzlog(const char *file, size_t filelen,
	const char *func, size_t funclen,
	long line, int level,
	const char *format, ...) {
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");
        va_end(args);
}
void vdzlog(const char *file, size_t filelen,
	const char *func, size_t funclen,
	long line, int level,
	const char *format, va_list args) {
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");
}
int zlog_set_record(const char *rname, zlog_record_fn record) {
    (void)rname;
    (void)record;
    return 0;
}

const char *zlog_version(void) {
    return "stub-1.0";
}