/* Generated by conf2struct (https://www.rutschle.net/tech/conf2struct/README)
 * on Sat May  8 07:03:13 2021. 

# conf2struct: generate libconf parsers that read to structs
# Copyright (C) 2018-2019  Yves Rutschle
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

*/

#define _GNU_SOURCE
#include <string.h>
#ifdef LIBCONFIG
#    include <libconfig.h>
#endif
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include "echosrv-conf.h"
#include "argtable3.h"
#ifdef LIBPCRE
#include <pcreposix.h>
#else
#include <regex.h>
#endif


/* This gets included in the output .c file */


/* Libconfig 1.4.9 is still used by major distributions
 * (e.g. CentOS7) and had a different name for
 * config_setting_lookup */
#if LIBCONFIG_VER_MAJOR == 1
#if LIBCONFIG_VER_MINOR == 4
#if LIBCONFIG_VER_REVISION == 9
#define config_setting_lookup config_lookup_from
#endif
#endif
#endif


/* config_type, lookup_fns, type2str are related, keep them together */
typedef enum {
    CFG_BOOL,
    CFG_INT,
    CFG_INT64,
    CFG_FLOAT,
    CFG_STRING,
    CFG_GROUP,
    CFG_ARRAY,
    CFG_LIST,
} config_type;
/* /config_type */

const char* type2str[] = {
    "boolean",
    "int",
    "int64",
    "float",
    "string",
    "group",
    "array",
    "list",
};

typedef union {
    int def_bool;
    int def_int;
    long long def_int64;
    double def_float;
    char* def_string;
} any_val;

struct config_desc {
    const char* name;
    int type;
    struct config_desc * sub_group; /* Table for compound types (list and group) */
    void* arg_cl; /* command-line argument for this setting */
    void* base_addr; /* Base of the structure (filled at runtime). Probably not useable for list elements */
    size_t offset;  /* Offset of setting in the structure */
    size_t offset_len; /* Offset of *_len field, for arrays and lists */
    size_t offset_present; /* offset of *_is_present field, for optional settings */
    size_t size;   /* Size of element, or size of group for groups and lists */
    int array_type; /* type of array elements, when type == CFG_ARRAY */
    int mandatory;
    int optional;
    any_val default_val;
};

#ifndef LIBCONFIG
/* Stubs in case you don't want libconfig */

typedef void config_setting_t;
typedef int config_t;
#define CONFIG_TRUE 1
#define CONFIG_FALSE 0

#define make_config_setting_lookup(type) \
    int config_setting_lookup_##type(const config_setting_t* a, const char* b, void* c) { \
        return 0; \
    }

#define make_config_setting_get(type, ret_type) \
    ret_type config_setting_get_##type(const config_setting_t* a) { \
        return 0; \
    }

make_config_setting_lookup(bool);
make_config_setting_lookup(int);
make_config_setting_lookup(int64);
make_config_setting_lookup(float);
make_config_setting_lookup(string);

make_config_setting_get(bool, int);
make_config_setting_get(int, int);
make_config_setting_get(int64, int);
make_config_setting_get(float, double);
make_config_setting_get(string, char*);

config_setting_t* config_lookup(config_t* c, const char* b) {
    return NULL;
}

void config_init(config_t* c) {
    return;
}

config_setting_t* config_setting_lookup(config_setting_t* a, char* b) {
    return NULL;
}

int config_setting_length(config_setting_t* a) {
    return 0;
}

config_setting_t* config_setting_get_elem(config_setting_t* a, int i) {
    return NULL;
}

int config_read_file(config_t* a, const char* b) {
    return CONFIG_TRUE;
}

int config_error_line(config_t* c) {
    return 0;
}

char* config_error_text(config_t* c) {
    return NULL;
}
#endif

/* This is the same as config_setting_lookup_string() except
it allocates a new string which belongs to the caller */
static int myconfig_setting_lookup_stringcpy(
        const config_setting_t* setting, 
        const char* name, 
        char** value)
{
    const char* str;
    *value = NULL;
    if (config_setting_lookup_string(setting, name, &str) == CONFIG_TRUE) {
        asprintf(value, "%s", str);
        return CONFIG_TRUE;
    } else {
        return CONFIG_FALSE;
    }
}

typedef int (*lookup_fn)(const config_setting_t*, const char*, void*);
lookup_fn lookup_fns[] = {
    (lookup_fn)config_setting_lookup_bool,
    (lookup_fn)config_setting_lookup_int,
    (lookup_fn)config_setting_lookup_int64,
    (lookup_fn)config_setting_lookup_float,
    (lookup_fn)myconfig_setting_lookup_stringcpy,
    NULL,  /* CFG_GROUP */
    NULL,  /* CFG_ARRAY */
    NULL,  /* CFG_LIST */
};

/* Copy an any_val to arbitrary memory location */
/* 0: success
 * <0: error */
static int any_valcpy(config_type type, void* target, any_val val)
{
    switch(type) {
    case CFG_BOOL:
        *(int*)target = val.def_bool;
        break;

    case CFG_INT:
        *(int*)target = val.def_int;
        break;

    case CFG_INT64:
        *(long long*)target = val.def_int64;
        break;

    case CFG_FLOAT:
        *(double*)target = val.def_float;
        break;

    case CFG_STRING:
        *(char**)target = val.def_string;
        break;

    default:
        fprintf(stderr, "Unknown type specification %d\n", type);
        return -1;
    }
    return 1;
}


/* Copy the value of a setting to an arbitrary memory that
* must be large enough */
/* 0: success
 * <0: error */
static int settingcpy(config_type type, void* target, const config_setting_t* setting)
{
    any_val val;
    char* str;

    switch(type) {
    case CFG_BOOL:
        val.def_bool = config_setting_get_bool(setting);
        *(int*)target = val.def_bool;
        break;

    case CFG_INT:
        val.def_int = config_setting_get_int(setting);
        *(int*)target = val.def_int;
        break;

    case CFG_INT64:
        val.def_int64 = config_setting_get_int64(setting);
        *(long long*)target = val.def_int64;
        break;

    case CFG_FLOAT:
        val.def_float = config_setting_get_float(setting);
        *(double*)target = val.def_int64;
        break;

    case CFG_STRING:
        asprintf(&str, "%s", config_setting_get_string(setting));
        val.def_string = str;
        *(char**)target = val.def_string;
        break;

    default:
        fprintf(stderr, "Unknown type specification %d\n", type);
        return -1;
    }
    return 0;
}

/* Copy the value of a command line arg to arbitrary memory
* that must be large enough for the type */
/* 0: success
 * <0: error */
static int clcpy(config_type type, void* target, const void* cl_arg)
{
    any_val val;
    char* str;

    switch(type) {
    case CFG_BOOL:
        val.def_bool = (*(struct arg_lit**)cl_arg)->count;
        *(int*)target = val.def_bool;
        break;

    case CFG_INT:
        val.def_int = (*(struct arg_int**)cl_arg)->ival[0];
        *(int*)target = val.def_int;
        break;

    case CFG_INT64:
        val.def_int64 = (*(struct arg_int**)cl_arg)->ival[0];
        *(long long*)target = val.def_int64;
        break;

    case CFG_FLOAT:
        val.def_float = (*(struct arg_dbl**)cl_arg)->dval[0];
        *(double*)target = val.def_float;
        break;

    case CFG_STRING:
        asprintf(&str, "%s", (*(struct arg_str**)cl_arg)->sval[0]);
        val.def_string = str;
        *(char**)target = val.def_string;
        break;

    default:
        fprintf(stderr, "Unknown type specification %d\n", type);
        return -1;
    }
    return 0;
}

/* Copy the value of a string argument to arbitary memory
* location that must be large enough, converting on the way
* (i.e. CFG_INT gets atoi() and so on) */
/* 0: success
 * <0: error */
static int stringcpy(config_type type, void* target, char* from)
{
    any_val val;
    
    switch(type) {
    case CFG_BOOL:
        val.def_bool = (*from != '0');
        *(int*)target = val.def_bool;
        break;

    case CFG_INT:
        val.def_int = strtol(from, NULL, 10);
        *(int*)target = val.def_int;
        break;

    case CFG_INT64:
        val.def_int64 = strtoll(from, NULL, 10);
        *(long long*)target = val.def_int64;
        break;

    case CFG_FLOAT:
        val.def_float = strtod(from, NULL);
        *(double*)target = val.def_float;
        break;

    case CFG_STRING:
        val.def_string = from;
        *(char**)target = val.def_string;
        break;

    default:
        fprintf(stderr, "Unknown type specification %d\n", type);
        return -1;
    }
    return 0;
}


/* Element to describe the target of a compound element
* element: which config entry is being changed
* match: if >0, index in pmatch to set
*        if 0, don't match but init with value
* value: constant if not matching */
struct compound_cl_target {
    struct config_desc * element;
    int match;
    any_val value;
};

/* Element to describe one compound command line argument
 * An argument is string that gets matched against a regex,
 * then match-groups get evaluated to each targets[].
 * For lists, base_entry points to the config_setting so we
 * can append to it */
struct compound_cl_arg {
    const char* regex;
    struct arg_str** arg_cl; /* arg_str entry for this compound option */
    struct config_desc * base_entry;
    struct compound_cl_target* targets;

    /* If override_desc is set, it points to the descriptor of the element in
    the group which will be checked for override. Then, override_matchindex
    indicates the command-line parameter match used to compare against
    override_desc to know if this group is overridden. If override_matchindex
    is 0, we don't match from the command-line but from a constant stored in
    override_const instead */
    struct config_desc * override_desc;
    int override_matchindex;
    char* override_const;
};


struct arg_file* echocfg_conffile;
 struct arg_lit* echocfg_udp;
 struct arg_str* echocfg_prefix;
 struct arg_str* echocfg_listen_host;
 struct arg_str* echocfg_listen_port;
 	struct arg_str* echocfg_listen;
 struct arg_end* echocfg_end;

                          
static struct config_desc table_echocfg_listen[] = {


        { 
            /* name */          "host", 
            /* type */          CFG_STRING, 
            /* sub_group*/      NULL,
            /* arg_cl */        & echocfg_listen_host,
            /* base_addr */     NULL,
            /* offset */        offsetof(struct echocfg_listen_item, host),
            /* offset_len */    0,
            /* offset_present */ 0,
            /* size */          sizeof(char*), 
            /* array_type */    -1,
            /* mandatory */     1, 
            /* optional */      0, 
            /* default_val*/    .default_val.def_string = NULL 
        },

        { 
            /* name */          "port", 
            /* type */          CFG_STRING, 
            /* sub_group*/      NULL,
            /* arg_cl */        & echocfg_listen_port,
            /* base_addr */     NULL,
            /* offset */        offsetof(struct echocfg_listen_item, port),
            /* offset_len */    0,
            /* offset_present */ 0,
            /* size */          sizeof(char*), 
            /* array_type */    -1,
            /* mandatory */     1, 
            /* optional */      0, 
            /* default_val*/    .default_val.def_string = NULL 
        },
	{ 0 }
};
                   
static struct config_desc table_echocfg[] = {


        { 
            /* name */          "udp", 
            /* type */          CFG_BOOL, 
            /* sub_group*/      NULL,
            /* arg_cl */        & echocfg_udp,
            /* base_addr */     NULL,
            /* offset */        offsetof(struct echocfg_item, udp),
            /* offset_len */    0,
            /* offset_present */ 0,
            /* size */          sizeof(int), 
            /* array_type */    -1,
            /* mandatory */     0, 
            /* optional */      0, 
            /* default_val*/    .default_val.def_bool = 0 
        },

        { 
            /* name */          "prefix", 
            /* type */          CFG_STRING, 
            /* sub_group*/      NULL,
            /* arg_cl */        & echocfg_prefix,
            /* base_addr */     NULL,
            /* offset */        offsetof(struct echocfg_item, prefix),
            /* offset_len */    0,
            /* offset_present */ 0,
            /* size */          sizeof(char*), 
            /* array_type */    -1,
            /* mandatory */     1, 
            /* optional */      0, 
            /* default_val*/    .default_val.def_string = NULL 
        },

        { 
            /* name */          "listen", 
            /* type */          CFG_LIST, 
            /* sub_group*/      table_echocfg_listen,
            /* arg_cl */        NULL,
            /* base_addr */     NULL,
            /* offset */        offsetof(struct echocfg_item, listen),
            /* offset_len */    offsetof(struct echocfg_item, listen_len),
            /* offset_present */ 0,
            /* size */          sizeof(struct echocfg_listen_item), 
            /* array_type */    -1,
            /* mandatory */     1, 
            /* optional */      0, 
            /* default_val*/    .default_val.def_int = 0 
        },
	{ 0 }
};
static struct compound_cl_target echocfg_listen_targets [] = {
	{ & table_echocfg_listen[0], 1, .value.def_string = "0" },
	{ & table_echocfg_listen[1], 2, .value.def_string = "0" },
	{ 0 }
};

static struct compound_cl_arg compound_cl_args[] = {
        {   /* arg: listen */
            .regex =           "(.+):(\\w+)",
            .arg_cl =          & echocfg_listen,
            .base_entry =      & table_echocfg [2],
            .targets =         echocfg_listen_targets,


            .override_desc =   NULL,
            .override_matchindex = 0,
            .override_const = NULL,
        },

	{ 0 }
};


/* Enable debug to follow the parsing of tables */
#if 0
#define TRACE_READ(x) printf x
#define TRACE_READ_PRINT_SETTING 1
#else
#define TRACE_READ(x)
#define TRACE_READ_PRINT_SETTING 0
#endif

/* Enable debug to follow the parsing of compound options */
#if 0
#define TRACE_CMPD(x) printf x
#define TRACE_CMPD_PRINT_SETTING 1
#else
#define TRACE_CMPD(x)
#define TRACE_CMPD_PRINT_SETTING 0
#endif

static void print_setting(config_type type, void* val)
{
    if (TRACE_READ_PRINT_SETTING || TRACE_CMPD_PRINT_SETTING) {
        switch(type) {
        case CFG_BOOL:
        case CFG_INT:
            printf("%d", *(int*)val);
            break;
        case CFG_INT64:
            printf("%lld", *(long long*)val);
            break;
        case CFG_FLOAT:
            printf("%f", *(double*)val);
            break;
        case CFG_STRING:
            printf("`%s'", *(char**)val);
            break;
        case CFG_GROUP:
        case CFG_LIST:
        case CFG_ARRAY:
            break;
        }
    }
}

/* Changes all dashes to underscores in a string of
* vice-versa */
static void strswap_ud(const char target, char* str)
{
    char* c;
    for (c = str; *c; c++)
        if (*c == (target == '_' ? '-' : '_'))
             *c = (target == '_' ? '_' : '-');
}

/* Same as config_setting_lookup() but looks up with dash or
* underscore so `my_setting` and `my-setting` match the same */
static config_setting_t* config_setting_lookup_ud(config_setting_t* cfg, struct config_desc* desc)
{
    config_setting_t* setting;
    char name[strlen(desc->name)+1];;
    strcpy(name, desc->name);

    strswap_ud('_', name);
    setting = config_setting_lookup(cfg, name);
    if (setting)
        return setting;

    strswap_ud('-', name);
    setting = config_setting_lookup(cfg, name);
    return setting;
}

static int lookup_typed_ud(config_setting_t* cfg, void* target, struct config_desc *desc)
{
    lookup_fn lookup_fn = lookup_fns[desc->type];
    char name[strlen(desc->name)+1];;
    strcpy(name, desc->name);

    strswap_ud('_', name);
    if (lookup_fn(cfg, name, ((char*)target) + desc->offset) == CONFIG_TRUE)
        return CONFIG_TRUE;

    strswap_ud('-', name);
    return lookup_fn(cfg, name, ((char*)target) + desc->offset);
}

/* Removes a setting, trying both underscores and dashes as
* name (so deleting 'my-setting' deletes both 'my_setting'
* and 'my-setting') */
static int setting_delete_ud(config_setting_t* cfg, struct config_desc *desc)
{
    char name[strlen(desc->name)+1];;
    strcpy(name, desc->name);

    strswap_ud('_', name);
    if (config_setting_remove(cfg, name) == CONFIG_TRUE)
        return CONFIG_TRUE;

    strswap_ud('-', name);
    return config_setting_remove(cfg, name);
}

/* When traversing configuration, allocate memory for plural
* types, init for scalars */
static void read_block_init(void* target, config_setting_t* cfg, struct config_desc* desc)
{
    size_t len = 0;
    void* block;
    config_setting_t* setting;

    switch (desc->type) {
    case CFG_LIST:
    case CFG_ARRAY:
        if (cfg) {
            setting = config_setting_lookup_ud(cfg, desc);
            if (setting)
                len = config_setting_length(setting);
        } 
        block = calloc(len, desc->size);

        *(size_t*)(((char*)target) + desc->offset_len) = len;
        *(void**)(((char*)target) + desc->offset) = block;
        TRACE_READ((" sizing for %zu elems ", len));
        break;

    case CFG_GROUP:
        block = calloc(1, desc->size);
        *(void**)(((char*)target) + desc->offset) = block;
        TRACE_READ((" sizing for %zu elems ", len));
        break;

    default: 
        /* scalar types: copy default */
        memcpy(((char*)target) + desc->offset, &desc->default_val, desc->size);
        TRACE_READ(("setting %s to default ", desc->name));
        print_setting(desc->type,(char*)target + desc->offset);
        break;
    }
}

static int read_block(config_setting_t* cfg, 
                      void* target, 
                      struct config_desc* desc, 
                      char** errmsg);

/* When traversing configuration, set value from config
* file, or command line 
* return: 0 if not set, 1 if set somehow */
static int read_block_setval(void* target, 
                             config_setting_t* cfg, 
                             struct config_desc* desc, 
                             char** errmsg)
{
    int i;
    size_t len = 0;
    void* block;
    int in_cfg = 0, in_cl = 0; /* Present in config file?  present on command line? */
    config_setting_t* setting = NULL;

    switch (desc->type) {
    case CFG_LIST:
        if (cfg) {
            setting = config_setting_lookup_ud(cfg, desc);
            if (setting) 
                len = config_setting_length(setting);
            block = *(void**)(((char*)target) + desc->offset);
            for (i = 0; i < len; i++) {
                config_setting_t* elem = config_setting_get_elem(setting, i);
                if (!read_block(elem, (char*)block + desc->size * i, desc->sub_group, errmsg))
                    return 0;
            }
        }
        break;

    case CFG_ARRAY:
        if (cfg) {
            setting = config_setting_lookup_ud(cfg, desc);
            if (setting)
                len = config_setting_length(setting);
            block = *(void**)(((char*)target) + desc->offset);
            for (i = 0; i < len; i++) {
                config_setting_t* elem = config_setting_get_elem(setting, i);
                settingcpy(desc->array_type, (char*)block + desc->size * i, elem);
                TRACE_READ(("[%d] = ", i));
                print_setting(desc->array_type, (char*)block + desc->size *i); TRACE_READ(("\n"));
            }
            setting_delete_ud(cfg, desc);
        }
        break;

    case CFG_GROUP:
        if (cfg) setting = config_setting_lookup_ud(cfg, desc);
        block = *(void**)(((char*)target) + desc->offset);
        if (!read_block(setting, block, desc->sub_group, errmsg)) return 0;
        break;

    default: /* scalar types */
        TRACE_READ((" `%s'", desc->name));
        if (cfg && config_setting_lookup_ud(cfg, desc)) {
            TRACE_READ((" in config file: "));
            /* setting is present in cfg, look it up */
            if (lookup_typed_ud(cfg, target, desc) != CONFIG_TRUE) {
                TRACE_READ((" but wrong type (expected %s) ", type2str[desc->type]));
                asprintf(errmsg, "Option \"%s\" wrong type, expected %s\n", 
                    desc->name, type2str[desc->type]);
                return 0;
            }
            print_setting(desc->type, (((char*)target) + desc->offset));
            setting_delete_ud(cfg, desc);
            in_cfg = 1;
        } else {
            TRACE_READ((" not in config file"));
        }
        if (desc->arg_cl && (*(struct arg_int**)desc->arg_cl)->count) {
            clcpy(desc->type, ((char*)target) + desc->offset, desc->arg_cl);
            TRACE_READ((", command line sets to "));
            print_setting(desc->type, (((char*)target) + desc->offset));
            in_cl = 1;
        } else {
            TRACE_READ((", not in command line"));
        }
        if (!(in_cfg || in_cl)) {
            TRACE_READ(("\n"));
            return 0;
        }
        TRACE_READ(("\n"));
        break;
    }
    return 1;
}

/* Set *_is_present flag for target */
static void target_is_present(void* target, struct config_desc* desc, int val)
{
    if (desc->optional) {  /* _is_present only exists in target for optional settings */
        TRACE_READ((" mark as set"));
        *(int*)((char*)target + desc->offset_present) = val;
    }
}

/* traverses the configuration; allocates memory if needed,
* set to default if exists,
* fill from configuration file if present, overrides or set from
* command line if present, verifies mandatory options have
* been set
* target: base address of the group being processed
*/
static int read_block(config_setting_t* cfg, void* target, struct config_desc* desc, char** errmsg)
{
    int set;

    for (; desc->name; desc++) {
        TRACE_READ(("reading %s%s%s: ", desc->optional ? "optional " : "", desc->mandatory ? "mandatory " : "",  desc->name));
        desc->base_addr = target;


        read_block_init(target, cfg, desc);
        set = read_block_setval(target, cfg, desc, errmsg);

        if (!set && desc->mandatory) {
            asprintf(errmsg, "Mandatory option \"%s\" not found", desc->name);
            return 0;
        }

        if (desc->optional) target_is_present(target, desc, set && desc->optional);
    }
    return 1;
}

/* Copy regex match into newly allocated string
 * out: newly allocated string (caller has to free it)
 * in: string into which the match was made
 * pmatch: the match to extract */
static void pmatchcpy(char** out, const char* in, regmatch_t* pmatch)
{
    int len = pmatch->rm_eo - pmatch->rm_so;
    *out = calloc(len+1, 1);
    memcpy(*out, in + pmatch->rm_so, len);
}

/* Processes a list of targets within one element, setting
* the values in the target setting 
* target: where to put the data
* arg: CL arg containing the target fields
* clval: command line parameter
* pmatch: regex match array into clval
*/
static int set_target_fields(void* target_addr, struct compound_cl_arg* arg, const char* clval, regmatch_t* pmatch)
{
    int pmatch_cnt = 1;
    struct compound_cl_target* target;

    for (target = arg->targets; target->element; target++) {
        struct config_desc * element = target->element;
        if (target->match) {
            TRACE_CMPD(("    match %d rm_so %d rm_eo %d type %d\n", 
                        pmatch_cnt, pmatch[pmatch_cnt].rm_so, pmatch[pmatch_cnt].rm_eo, element->type ));
            if (pmatch[pmatch_cnt].rm_so == -1) {
                /* This should not happen as regexec() did
                * match before, unless there is a
                * discrepency between the regex and the
                * number of backreferences */
                return 0;
            }
            char* str;
            pmatchcpy(&str, clval, &pmatch[pmatch_cnt]);

            stringcpy(element->type, (char*)target_addr + element->offset, str);
            TRACE_CMPD(("setting %p+%zu to : ", target_addr , element->offset));
            print_setting(element->type , (char*)target_addr + element->offset);
            TRACE_CMPD(("\n"));

            /* str is temporary buffer for type conversion, except for strings which we
            * need to keep around so don't free them */
            if (element->type != CFG_STRING)
                free(str);
            pmatch_cnt++;
        } else { /* don't use matching, set constant */
            any_valcpy(element->type, (char*)target_addr + element->offset,
                        target->value);
        }
    }

    return 1;
}

/* Goes over a list, finds if a group matches the specified string and overwrite
* it if it does. */
static int override_on_str(struct compound_cl_arg* arg, const char* str, regmatch_t* pmatch)
{
    struct config_desc * desc = arg->base_entry;
    void* list_base = *(void**)(desc->base_addr + desc->offset);
    size_t list_len = *(size_t*)(desc->base_addr + desc->offset_len);
    size_t elem_size = desc->size;
    int i;

    for (i = 0; i < list_len; i++) {
        char* group_base = ((char*)list_base + i * elem_size);

        char* cfg_member = *(char**)(group_base + arg->override_desc->offset);
        if (!strcmp(str, cfg_member)) {
            memset(group_base, 0, elem_size);
            struct arg_str* arg_cl = *arg->arg_cl;
            if (!set_target_fields(group_base, arg, arg_cl->sval[0], pmatch))
                return 0;
            return 1;
        }
    }
    return 0;
}

/* Goes over a list and override group if needed */
static int override_elem(struct compound_cl_arg* arg, int arg_index, regmatch_t* pmatch) 
{
    char* str;
    int allocated = 0;
    int res;

    if (arg->override_matchindex) {
        struct arg_str* arg_cl = *arg->arg_cl;
        pmatchcpy(&str, arg_cl->sval[arg_index], &pmatch[arg->override_matchindex]);
        allocated = 1;
    } else {
        str = arg->override_const;
    }

    res = override_on_str(arg, str, pmatch);

    if (allocated) free(str);

    return res;
}

/* Add an argument to a list, overriding if required or
* appending otherwise */
static int add_arg_to_list(struct compound_cl_arg* arg, int arg_index, regmatch_t* pmatch)
{
    struct config_desc * desc = arg->base_entry;
    void* list_base = *(void**)(desc->base_addr + desc->offset);
    size_t list_len = *(size_t*)(desc->base_addr + desc->offset_len);
    size_t elem_size = desc->size;

    /* are we overriding an existing group? */
    if (arg->override_desc)
        if (override_elem(arg, arg_index, pmatch))
            return 1;

    /* override not found or no override, append element and * zero it out */
    list_len++;
    list_base = realloc(list_base, list_len * elem_size);
    *(size_t*)(desc->base_addr + desc->offset_len) = list_len;
    *(void**)(desc->base_addr + desc->offset) = list_base;
    memset(list_base + (list_len - 1) * elem_size, 0, elem_size);

    struct arg_str* arg_cl = *arg->arg_cl;
    if (!set_target_fields((char*)list_base + (list_len - 1) * elem_size, arg, arg_cl->sval[arg_index], pmatch)) {
        return 0;
    }
    return 1;
}

/* TODO: pass pmatch size as parameter or something */
#define MAX_MATCH 10

/* Regex fiddling: uses info in arg to fill pmatch
* arg: description of the command line argument
* arg_index: occurence of this argument on the command line
*/
static int regcompmatch(regmatch_t* pmatch, 
                        struct compound_cl_arg* arg, 
                        int arg_index, 
                        char** errmsg)
{
    char* regerr;
    struct arg_str* arg_cl = *arg->arg_cl;
    regex_t preg;
    int res = regcomp(&preg, arg->regex, REG_EXTENDED);
    if (res) {
        int errlen = regerror(res, &preg, NULL, 0);
        regerr = malloc(errlen);
        regerror(res, &preg, regerr, errlen);
        asprintf(errmsg, "compiling pattern /%s/:%s", arg->regex, regerr);
        free(regerr);
        return 0;
    }
    res = regexec(&preg, arg_cl->sval[arg_index], MAX_MATCH, &pmatch[0], 0);
    if (res) {
        asprintf(errmsg, "--%s %s: Illegal argument", 
        arg_cl->hdr.longopts, 
        arg->regex); 
        return 0;
    }
    return 1;
}

/* Read compound options described in `arg`, from `cfg`, to `setting` */
static int read_compounds(config_setting_t* cfg, 
                          void* setting, 
                          struct compound_cl_arg* arg, 
                          char** errmsg)
{
    int arg_i;
    struct arg_str* arg_cl;
    regmatch_t pmatch[MAX_MATCH];

    for (; arg->regex; arg++) {
        arg_cl = *arg->arg_cl;
        TRACE_CMPD(("Compound %s occurs %d : ", arg_cl->hdr.longopts, arg_cl->count));
        for (arg_i = 0; arg_i < arg_cl->count; arg_i++) {
            if (!regcompmatch(&pmatch[0], arg, arg_i, errmsg))
                return 0;
            TRACE_CMPD(("`%s' matched\n", arg_cl->sval[arg_i]));

            switch (arg->base_entry->type) {
            case CFG_LIST:
                /* In a list, find the end or the element to override */
                if (!add_arg_to_list(arg, arg_i, pmatch)) {
                    return 0;
                }
                break;

            /* Semantics for CFG_ARRAY TBD */

            case CFG_GROUP:
                if (!set_target_fields(
                            /* base_addr is the same for all elements in the group */
                            arg->targets[0].element->base_addr, 
                            arg, 
                            arg_cl->sval[arg_i], 
                            pmatch))
                    return 0;

            default:
                TRACE_CMPD(("error, compound on type %d\n", arg->base_entry->type));
                break;
            }
        }
        TRACE_CMPD(("done %s\n", arg_cl->hdr.longopts));
    }
    return 1;
}

/* read config file `filename` into `c` */
static int c2s_parse_file(const char* filename, config_t* c, char**errmsg)
{
    /* Read config file */
    if (config_read_file(c, filename) == CONFIG_FALSE) {
        if (config_error_line(c) != 0) {
           asprintf(errmsg, "%s:%d:%s", 
                    filename,
                    config_error_line(c),
                    config_error_text(c));
           return 0;
        }
        asprintf(errmsg, "%s:%s", filename, config_error_text(c));
        return 0;
    }
    return 1;
}

/* Allocates a new string that represents the setting value, which must be a scalar */
static void scalar_to_string(char** strp, config_setting_t* s)
{
    switch(config_setting_type(s)) {
    case CONFIG_TYPE_INT:
        asprintf(strp, "%d\n", config_setting_get_int(s));
        break;

    case CONFIG_TYPE_BOOL:
        asprintf(strp, "%s\n", config_setting_get_bool(s) ?  "[true]" : "[false]" );
        break;

    case CONFIG_TYPE_INT64:
        asprintf(strp, "%lld\n", config_setting_get_int64(s));
        break;

    case CONFIG_TYPE_FLOAT:
        asprintf(strp, "%lf\n", config_setting_get_float(s));
        break;

    case CONFIG_TYPE_STRING:
        asprintf(strp, "%s\n", config_setting_get_string(s));
        break;

    default: /* This means a bug */
        fprintf(stderr, "Unexpected type %d\n", config_setting_type(s));
        exit(1);
    }
}

/* Typesets all the settings in a configuration as a
* newly-allocated string. The string management is caller's
* responsability. 
* Returns the number of scalars in the configuration */
static int cfg_as_string(config_setting_t* parent, const char* path, char** strp)
{
    int i, len, res = 0;
    config_setting_t* child;
    char* subpath, *value, *old;
    const char* name;

    len = config_setting_length(parent);
    for (i = 0; i < len; i++) {
        child = config_setting_get_elem(parent, i);
        name = config_setting_name(child);
        if (!name) name = "";

        if(config_setting_is_list(parent) ||
           config_setting_is_array(parent)) {
            asprintf(&subpath, "%s[%d]%s", path, config_setting_index(child), name);
        } else {
            asprintf(&subpath, "%s/%s", path, name);
        }

        if (config_setting_is_scalar(child)) {
            scalar_to_string(&value, child);

            /* Add value to the output string  */
            if (*strp) {
                asprintf(&old, "%s", *strp);
                free(*strp);
            }  else {
                asprintf(&old, "%s", "");
            }
            asprintf(strp, "%s%s:%s", old, subpath, value);
            free(value);
            free(old);

            res++; /* At least one scalar was found */
        } else {
            /* It's an aggregate -- descend into it */
            res += cfg_as_string(child, subpath, strp);
        }

        free(subpath);
    }
    return res;
}


/* 0: success
   <0: error */
int echocfg_cl_parse(int argc, char* argv[], struct echocfg_item* cfg)
{
    int nerrors, res;
    config_t c;
    char* errmsg;
    config_setting_t* s;
    void* argtable[] = {
            #ifdef LIBCONFIG
        echocfg_conffile = arg_filen("F", "config", "<file>", 0, 1, "Specify configuration file"),
    #endif
         echocfg_udp = arg_litn(NULL, "udp", 0, 1, ""),
         echocfg_prefix = arg_strn(NULL, "prefix", "<str>", 0, 1, ""),
         echocfg_listen_host = arg_strn(NULL, "listen-host", "<str>", 0, 10, ""),
         echocfg_listen_port = arg_strn(NULL, "listen-port", "<str>", 0, 10, ""),
 	echocfg_listen = arg_strn("p", "listen", "<host:port>", 0, 10, "Listen on host:port"),
 	echocfg_end = arg_end(10)

    };

    /* Parse command line */
    nerrors = arg_parse(argc, argv, argtable);
    if (nerrors) {
        arg_print_errors(stdout, echocfg_end, "echocfg"); 
        arg_print_syntax(stdout, argtable, "\n");
        arg_print_glossary(stdout, argtable, "  %-25s\t%s\n");
        return -1;
    }


    config_init(&c);
    if (echocfg_conffile && echocfg_conffile->count) {
        if (!c2s_parse_file(echocfg_conffile->filename[0], &c, &errmsg)) {
            fprintf(stderr, "%s\n", errmsg);
            return -1;
        }
    }

    s = config_lookup(&c, "/");

    res = read_block(s, cfg, table_echocfg, &errmsg);
    if (!res) {
        fprintf(stderr, "%s\n", errmsg);
        return -1;
    }

    res = read_compounds(s, cfg, compound_cl_args, &errmsg);
    if (!res) {
        fprintf(stderr, "%s\n", errmsg);
        return -1;
    }

    errmsg = NULL;
    res = cfg_as_string(s, "", &errmsg);
    if (res)
        fprintf(stderr, "Unknown settings:\n%s\n", errmsg);

    return 0;
}


static void indent(FILE* out, int depth) 
{
    int i;
    for (i = 0; i < depth; i++)
        fprintf(out, "    ");
}

static void echocfg_listen_fprint(
        FILE* out,
        struct echocfg_listen_item* echocfg_listen,
        int depth) 
{
    
        indent(out, depth);
        fprintf(out, "host: %s", echocfg_listen->host);
        fprintf(out, "\n");
        indent(out, depth);
        fprintf(out, "port: %s", echocfg_listen->port);
        fprintf(out, "\n");
}

void echocfg_fprint(
        FILE* out,
        struct echocfg_item* echocfg,
        int depth) 
{
    int i;
        indent(out, depth);
        fprintf(out, "udp: %d", echocfg->udp);
        fprintf(out, "\n");
        indent(out, depth);
        fprintf(out, "prefix: %s", echocfg->prefix);
        fprintf(out, "\n");

        indent(out, depth);
        fprintf(out, "listen [%zu]:\n", echocfg->listen_len);
        for (i = 0; i < echocfg->listen_len; i++) {
            echocfg_listen_fprint(out, &echocfg->listen[i], depth+1);
        }
}