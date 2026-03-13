#ifndef LEXIN_H_
#define LEXIN_H_

#include "stdint.h"
#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"
#include "string.h"
#include "ctype.h"
#include "assert.h"
#include "limits.h"
#include "errno.h"

#ifndef LEXIN_INIT_CAP
#define LEXIN_INIT_CAP 16
#endif // LEXIN_INIT_CAP

#ifdef __cplusplus
#define lexin_cast(T) (T)
#else
#define lexin_cast(T)
#endif

#define lexin_da_alloc(list, exp_cap)           \
    do {                                        \
        if ((exp_cap) > (list)->capacity) {     \
            if ((list)->capacity == 0) {        \
                (list)->capacity = LEXIN_INIT_CAP;\
            }                                   \
            while ((exp_cap) > (list)->capacity) { \
                (list)->capacity *= 2;             \
            }                                      \
            (list)->data = lexin_cast(typeof((list)->data))realloc((list)->data,(list)->capacity * sizeof(*(list)->data)); \
            assert((list)->data != NULL && "Ram is not enough!"); \
        } \
    } while (0)

#define lexin_da_append(list, element)            \
    do {                                       \
        lexin_da_alloc((list), (list)->count + 1); \
        (list)->data[(list)->count++] = (element);   \
    } while (0)

#define lexin_da_append_many(list, new_el, new_el_count)          \
    do {                                                          \
        lexin_da_alloc((list), (list)->count + (new_el_count)); \
        memcpy((list)->data + (list)->count, (new_el), (new_el_count)*sizeof(*(list)->data)); \
        (list)->count += (new_el_count); \
    } while (0)

#define lexin_da_append_null(list) lexin_da_append(list,0)
#define lexin_da_append_cstr(list,str) lexin_da_append_many(list,str,strlen(str))
#define lexin_da_clear(list) \
do {memset((list)->data,0,(list)->capacity); (list)->count = 0; } while(0)
#define lexin_da_free(list) \
do { free((list)->data); (list)->data = NULL; (list)->count = 0; (list)->capacity = 0; } while(0)

#define lexin_unreachable(...) \
do { fprintf(stderr,__VA_ARGS__);exit(1);} while(0)

typedef enum {
    token_unknown = 0,
    token_str,
    token_unified_str,
    token_lit,
    token_lit_double,
    token_id,
    token_key,
    token_op,
} lexin_token_type_t;

typedef struct {
    lexin_token_type_t type;
    union {
        uint64_t as_id;
        uint64_t as_unified_str;
        int64_t as_int;
        double as_double;
        uint64_t as_key_index;
        uint64_t as_str_index;
        uint64_t as_op_index;
    } val;
} lexin_token_t;

typedef struct {
    lexin_token_t* data;
    uint32_t count;
    uint32_t capacity;
} lexin_tokens_t;

typedef struct {
    uint32_t len;
    char* str;
} lexin_str_t;

typedef struct {
    lexin_str_t* data;
    uint32_t count;
    uint32_t capacity;
} lexin_str_list_t;

typedef struct {
    char* ctx;
    char* ctx_end;
    bool res;

    uint32_t line;
    char* cursor;
    char* last_cursor;

    FILE* err_out;
    char* file_name;

    char* ops;
    uint32_t opc;
    bool is_op[256];
    char** keys;
    uint32_t keyc;
    uint64_t* key_hashs;
    lexin_tokens_t tokens;
    lexin_str_list_t strs;

    char* sl_com;
    char* ml_com_start;
    char* ml_com_end;
} lexin_t;

bool lexin_consume_context(lexin_t* l);
bool lexin_convert_to_token(lexin_t* l);
char* get_token_type_str(lexin_token_t t);
bool lexin_is_keyword(lexin_t* l,char* ptr,uint32_t len);
uint32_t lexin_get_index_keyword(lexin_t* l,char* str,uint32_t len);
void print_token(lexin_t* l,lexin_token_t t,uint32_t i);
void lexin_free(lexin_t* l);

#define FNV_OFFSET 0xcbf29ce484222325ULL
#define FNV_PRIME 0x100000001b3ULL

#endif // LEXIN_H_

#ifdef LEXIN_IMPLEMENTATION
#ifndef LEXIN_FIRST_IMPLEMENTATION
#define LEXIN_FIRST_IMPLEMENTATION

// TODO add new look-up tables for isalpha isdigit etc
typedef struct {
    uint32_t sl_len;
    uint32_t ml_start_len;
    uint32_t ml_end_len;
    bool com_mode;
    bool str_mode;
    bool unified_str_mode;
    uint64_t ml_hash_start;
    uint64_t ml_hash_end;
    uint64_t sl_hash;
} lexin_ctx_t;

void lexin_free
(lexin_t* l)
{
    free(l->tokens.data);
    for(uint32_t i = 0;i < l->strs.count;++i)
    {free(l->strs.data[i].str);}
    free(l->strs.data);
}


uint64_t lexin_string_hash
(char *str, uint32_t len)
{
    uint64_t hash = FNV_OFFSET;
    const uint64_t prime = FNV_PRIME;
    for (uint32_t i = 0; i < len; ++i) {
        hash ^= (uint8_t)str[i];
        hash *= prime;
    }
    return hash;
}

// TODO this causes some time overhead
bool lexin_is_keyword
(lexin_t* l,char* ptr,uint32_t len)
{
    uint64_t hash = lexin_string_hash(ptr,len);
    uint32_t i = 0;
    for(;i < l->keyc;++i){
        if(l->key_hashs[i] == hash) return true;
    }
    return false;
}

uint32_t lexin_get_index_keyword
(lexin_t* l,char* ptr,uint32_t len)
{
    uint64_t hash = lexin_string_hash(ptr,len);
    uint32_t i = 0;
    for(;i < l->keyc;++i){
        if(l->key_hashs[i] == hash) return i;
    }
    lexin_unreachable("Got invalid string at lexin_get_index_keyword");
}

char* get_token_type_str
(lexin_token_t t)
{
    switch(t.type){

        case token_str: return "Str";
        case token_lit: return "Lit";
        case token_op: return "Op";
        case token_id: return "Id";
        case token_key: return "Key";
        case token_unified_str: return "UniStr";
        case token_lit_double: return "Double";
        case token_unknown:
        default: return "Unknown";
    }
}

void print_token
(lexin_t* l,lexin_token_t t,uint32_t i)
{
    switch(t.type) {
        case token_op : {
            printf("token[%d]:%s %c\n",i,get_token_type_str(t),
            l->ops[t.val.as_op_index]);
        } break;
        case token_str : {
            printf("token[%d]:%s %s\n",i,get_token_type_str(t),
            l->strs.data[t.val.as_str_index].str);
        } break;
        case token_key : {
            printf("token[%d]:%s %s\n",i,get_token_type_str(t),
            l->keys[t.val.as_key_index]);
        } break;
        case token_id : {
            printf("token[%d]:%s %08lx\n",i,get_token_type_str(t),t.val.as_id);
        } break;
        case token_lit : {
            printf("token[%d]:%s %ld\n",i,get_token_type_str(t),t.val.as_int);
        } break;
        case token_lit_double : {
            printf("token[%d]:%s %lf\n",i,get_token_type_str(t),t.val.as_double);
        } break;
        case token_unified_str : {
            printf("token[%d]:%s %lx\n",i,get_token_type_str(t),t.val.as_unified_str);
        } break;
        case token_unknown:
        default: {
            printf("token[%d]:Unknown: %s %ld\n",i,get_token_type_str(t),t.val.as_int);
            break;
        }
    }
}

void lexin_set_is_op_array
(lexin_t* l)
{
    memset(l->is_op,0,256);
    uint32_t i = 0;
    for(;i < l->opc;++i)
    {
        l->is_op[(uint8_t)l->ops[i]] = true;
    }
}

uint32_t lexin_get_index_op
(lexin_t* l,char c)
{
    uint32_t i = 0;
    for(;i < l->opc;++i)
    {
        if(c == l->ops[i]) return i;
    }
    lexin_unreachable("Got invalid char at lexin_get_index_op");
}

uint32_t lexin_get_col
(lexin_t* l)
{
    char* ptr = l->cursor;
    while(ptr-- > l->ctx){
        if(*ptr == '\n') {
            return l->cursor - ptr;
        }
    }
    return  l->cursor -l->ctx;
}

#ifdef LEXIN_NOLOG
#define lexin_printf(l,fmt,...)
#else
#define lexin_printf(l,fmt,...) \
do { fprintf((l)->err_out,"%s:%d:%d:" fmt,(l)->file_name,(l)->line,lexin_get_col((l)),__VA_ARGS__);} while(0)
#endif

bool lexin_convert_to_lit
(lexin_t* l,lexin_token_t* out)
{
    char arr[l->cursor - l->last_cursor + 1];
    memcpy(arr,l->last_cursor,l->cursor - l->last_cursor);
    arr[l->cursor - l->last_cursor] = 0;
    // TODO (MID): We can use sized strings here
    // TODO (FEATURE): Add new approach for this because this thing doesnt cover all the cases
    char* end = 0;
    int64_t val = 0;
    val = strtoul(arr,&end,10);
    if (val == (uint32_t)ULONG_MAX && errno == ERANGE) {
        lexin_printf(l,"Integer overflow \"%.*s\"\n",
        (uint32_t)(l->cursor - l->last_cursor),l->last_cursor);
        l->last_cursor = l->cursor+1;
        return false;
    }
    if(*end != '\0') {
        if(*end == 'x' || *end == 'X') {
            val = strtoul(end,0,16);
        } else if(*end == 'b' || *end == 'B') {
            val = strtoul(end,0,2);
        } else if(*end == 'o' || *end == 'O') {
            val = strtoul(end,0,8);
        } else {
            lexin_printf(l,"Unknown suffix %s\n",end);
            l->last_cursor = l->cursor+1;
            return false;
        }
    }
    if(l->tokens.count > 1) {
        if(
        l->tokens.data[l->tokens.count-1].type == token_op &&
        l->ops[l->tokens.data[l->tokens.count-1].val.as_op_index] == '-' &&
        l->tokens.data[l->tokens.count-2].type == token_op)
        {
            l->tokens.count--;
            val *= -1;
        }
    }
    out->val.as_int = val;
    out->type = token_lit;
    return true;
}

bool lexin_convert_to_token
(lexin_t* l)
{
    if(l->cursor == l->last_cursor) return true;
    lexin_token_t tok = (lexin_token_t){.type = token_unknown,.val = {0}};
    char lc = *l->last_cursor;
    if(isdigit(lc)) {
        if(lexin_convert_to_lit(l,&tok))
        {
            goto end;
        }
        else{
            return false;
        }
    }
    if(l->is_op[(uint8_t)lc]) {
        tok.val.as_op_index = lexin_get_index_op(l,lc);
        tok.type = token_op;
        goto end;
    }
    if ((lc >= 'A' && lc <= 'Z') || (lc >= 'a' && lc <= 'z') || lc == '_') {
        if(lexin_is_keyword(l,l->last_cursor,l->cursor - l->last_cursor)) {
            tok.type = token_key;
            tok.val.as_key_index =
            lexin_get_index_keyword(l,l->last_cursor,l->cursor - l->last_cursor);
            goto end;
        } else {
            tok.type = token_id;
            tok.val.as_id = lexin_string_hash(l->last_cursor,l->cursor - l->last_cursor);
            goto end;
        }
    }
    lexin_printf(l,"Unknown \"%.*s\"\n",(uint32_t)(l->cursor - l->last_cursor),l->last_cursor);
    l->last_cursor = l->cursor+1;
    return false;
end:
    lexin_da_append(&l->tokens,tok);
    l->last_cursor = l->cursor+1;
    return true;
}

void lexin_consume_last_one_if_possible
(lexin_t* l)
{
    if(
    !((l->cursor - 1 == l->last_cursor)
    || (l->cursor == l->last_cursor))
    && !isblank(*l->last_cursor))
    {
        if(*(l->cursor-1) != '\n')
        {if(!lexin_convert_to_token(l)){l->res = false;}}
        l->last_cursor--;
    }
}

bool check_slashes
(lexin_t* l,char* ptr)
{
    uint32_t count = 0;
    while(ptr-- > l->ctx){
        if(*ptr == '\\') {
            count++;
        }else {break;}
    }
    return (count % 2) == 0;
}


// This thing acts weird when thing like '\012332' or '\012f' came
// and it return 'a0a' and sometimes new line '\n' when i used it in
// unified_str function
int64_t lexin_handle_escape_sequence(char *ptr,uint32_t *skip_count)
{
    if (*ptr != '\\') return -1;
    char *start = ptr;
    ptr++;
    char esc = *ptr++;
    int64_t unified = -2;
    switch (esc) {
        case 'a'  : { unified = '\a'; break; }
        case 'b'  : { unified = '\b'; break; }
        case 'f'  : { unified = '\f'; break; }
        case 'n'  : { unified = '\n'; break; }
        case 'r'  : { unified = '\r'; break; }
        case 't'  : { unified = '\t'; break; }
        case 'v'  : { unified = '\v'; break; }
        case '\\' : { unified = '\\'; break; }
        case '\'' : { unified = '\''; break; }
        case '\"' : { unified = '\"'; break; }
        case '?'  : { unified = '\?'; break; }
        case 'x'  : {
            unsigned long val = 0;
            int found = 0;
            while (isxdigit(*ptr)) {
                found = 1;
                int c = *ptr++;
                val *= 16;
                if      (c >= '0' && c <= '9') val += c - '0';
                else if (c >= 'a' && c <= 'f') val += c - 'a' + 10;
                else if (c >= 'A' && c <= 'F') val += c - 'A' + 10;
            }
            if (!found){unified = 'x';}
            else{unified = (int64_t)val;}
            break;
        }
        default:
            if (esc >= '0' && esc <= '7') {
                unsigned long val = esc - '0';
                for (int i = 0;i < 2; i++) {
                    if (*ptr >= '0' && *ptr <= '7') {
                        val = val * 8 + (*ptr - '0');
                        ptr++;
                    } else {
                        break;
                    }
                }
                unified = (int64_t)val;
            }
            break;
    }
    *skip_count = ptr - start;
    return unified;
}


bool lexin_check_string
(lexin_t* l,lexin_ctx_t* ctx)
{
    if(ctx->com_mode) {return false;}
    char cc = *l->cursor;
    char cpc = *(l->cursor-1);
    char cppc = *(l->cursor-2);
    if(!(ctx->unified_str_mode) && (ctx->str_mode) && cc == '"' && check_slashes(l,l->cursor) &&
    ((cpc != '\'' && cppc != '\'') ^
    ((cpc != '\'') ^ (cppc != '\'')))) {
        lexin_token_t tok = {.type = token_str,.val.as_str_index = l->strs.count};
        uint32_t i = 0,skip,count = 0;
        int64_t tmp = 0;
        uint32_t len = l->cursor - l->last_cursor;
        char buf[len + 1];
        buf[len] = 0;
        for(;i < len;++i){
            tmp = lexin_handle_escape_sequence(l->last_cursor + i,&skip);
            if(tmp < 0){
                buf[count++] = *(l->last_cursor + i);
            }else {
                buf[count++] = tmp;
                i += skip - 1;
            }
        }
        buf[count] = 0;
        lexin_str_t str = {.str = strndup(buf,count+1),.len = count+1};
        lexin_da_append(&l->strs,str);
        lexin_da_append(&l->tokens,tok);
        l->last_cursor = l->cursor + 1;
        l->cursor++;
        ctx->str_mode = false;
        return true;
    }
    if(!(ctx->unified_str_mode) && !(ctx->str_mode) && cc == '"' && check_slashes(l,l->cursor) &&
    (cpc != '\'' && cppc != '\'')) {
        lexin_consume_last_one_if_possible(l);
        l->last_cursor = l->cursor + 1;
        l->cursor++;
        ctx->str_mode = true;
        return true;
    }
    if(ctx->str_mode)
    {
        l->cursor++;
        return true;
    }
    return false;
}

int32_t lexin_check_comment
(lexin_t* l,lexin_ctx_t* ctx)
{
    // TODO this removed some time but now hashing costs that time but its less
    bool is_sl_com       = (ctx->sl_hash       == lexin_string_hash(l->cursor,ctx->sl_len));
    bool is_ml_com_start = (ctx->ml_hash_start == lexin_string_hash(l->cursor,ctx->ml_start_len));
    bool is_ml_com_end   = (ctx->ml_hash_end   == lexin_string_hash(l->cursor,ctx->ml_end_len));
    if(is_ml_com_start) {
        lexin_consume_last_one_if_possible(l);
        ctx->com_mode = true;
        l->cursor++;
        return 1;
    }
    if(is_ml_com_end) {
        ctx->com_mode = false;
        l->cursor++;
        l->last_cursor = l->cursor;
        l->cursor++;
        return 1;
    }
    if(ctx->com_mode) {l->cursor++;return true;}
    if(is_sl_com) {
        lexin_consume_last_one_if_possible(l);
        char* end = strchr(l->cursor,'\n');
        if(!end) {return -1;}
        l->line++;
        l->cursor = end+1;
        l->last_cursor = end+1;
        return 1;
    }
    return 0;
}

bool lexin_check_unified_string
(lexin_t* l,lexin_ctx_t* ctx)
{
    if(ctx->com_mode) {return false;}
    char cc = *l->cursor;
    // TODO Write Better Checker
    if(!(ctx->str_mode) && (ctx->unified_str_mode) && cc == '\'' && check_slashes(l,l->cursor)) {
        bool res = true;
        if(((int)(l->cursor - l->last_cursor)) > (int)sizeof(uint64_t))
        {
            lexin_printf(l,"Unified strings length(%d) exceeds max length(%d)\n String:%.*s",
            (int)(l->cursor - l->last_cursor),(int)sizeof(uint64_t),
            (int)(l->cursor - l->last_cursor),l->last_cursor);
            res = false;goto len_failed;
        }
        uint32_t i = 0,count = 0;
        int64_t tmp = 0,val;
        val = lexin_handle_escape_sequence(l->last_cursor,&count);
        if (val < 0) {val = *(l->last_cursor);i++;}else {i += count;}
        for(;i < (uint32_t)(l->cursor - l->last_cursor);++i)
        {
            tmp = lexin_handle_escape_sequence(l->last_cursor,&count);
            if (tmp < 0) {tmp = *(l->last_cursor + i);}else {i += count;i--;}
            val = (val << 8) | tmp;
        }
        lexin_token_t tok = {.type = token_unified_str,.val.as_unified_str = val};
        lexin_da_append(&l->tokens,tok);
    len_failed:
        l->last_cursor = l->cursor + 1;
        l->cursor++;
        ctx->unified_str_mode = false;
        return res;
    }
    if(!(ctx->str_mode) && !(ctx->unified_str_mode) && cc == '\'' && check_slashes(l,l->cursor)) {
        lexin_consume_last_one_if_possible(l);
        l->last_cursor = l->cursor + 1;
        l->cursor++;
        ctx->unified_str_mode = true;
        return true;
    }
    if(ctx->unified_str_mode)
    {
        l->cursor++;
        return true;
    }
    return false;
}

bool lexin_consume_context
(lexin_t* l)
{
    if(!l->ctx) {return false;}
    if(!l->cursor) {l->cursor = l->ctx;}
    if(!l->last_cursor) {l->last_cursor = l->ctx;}
    if(!l->ctx_end) {l->ctx_end = strrchr(l->ctx,'\0');}
    l->res = true;l->line = 1;
    if(!l->err_out) {l->err_out = stderr;}
    uint64_t hashs[l->keyc];
    l->key_hashs = hashs;
    for(uint32_t i = 0;i < l->keyc;++i)
    {hashs[i] = lexin_string_hash(l->keys[i],strlen(l->keys[i]));}
    lexin_set_is_op_array(l);
    lexin_ctx_t ctx = {
        .sl_len = strlen(l->sl_com),
        .ml_start_len = strlen(l->ml_com_start),
        .ml_end_len = strlen(l->ml_com_end),
        .com_mode = false,
        .str_mode = false,
    };
    ctx.sl_hash = lexin_string_hash(l->sl_com,ctx.sl_len);
    ctx.ml_hash_start = lexin_string_hash(l->ml_com_start,ctx.ml_start_len);
    ctx.ml_hash_end = lexin_string_hash(l->ml_com_end,ctx.ml_end_len);
    char cc = 0;
    while(l->ctx_end > l->cursor)
    {
        // TODO (FEATURE): Add more checking
        cc = *l->cursor;
        if(cc == '\n') {l->line++;}
        if(lexin_check_unified_string(l,&ctx)) {continue;}
        if(lexin_check_string(l,&ctx)) {continue;}
        int32_t lcc = lexin_check_comment(l,&ctx);
        if(lcc == 1) {continue;}
        if(lcc == -1) {break;}
        if(cc == '\n') {
            lexin_consume_last_one_if_possible(l);
            l->cursor+=1;
            l->last_cursor = l->cursor;
            continue;
        }
        if(isblank(cc) || isspace(cc)) {
            lexin_consume_last_one_if_possible(l);
            l->last_cursor++;
            l->cursor++;
            continue;
        }
        if(l->is_op[(uint8_t)cc]) {
            if(l->cursor != l->last_cursor && !isblank(*l->last_cursor))
            {
                if(!lexin_convert_to_token(l))
                {l->res = false;}
            }
            lexin_token_t tok = {.type = token_op,
            .val.as_op_index = lexin_get_index_op(l,cc)};
            lexin_da_append(&l->tokens,tok);
            l->cursor++;
            l->last_cursor = l->cursor;
            continue;
        }
        l->cursor++;
    }
    if(l->last_cursor != l->cursor)
    {
        if(!lexin_convert_to_token(l))
        {l->res = false;}
    }
    l->key_hashs = 0;
    return l->res;
}

#endif // LEXIN_FIRST_IMPLEMENTATION

#endif // LEXIN_IMPLEMENTATION

/*
 *  lexin - A simple lexer library in c
 *  Copyright (C) 2025 Menderes Sabaz <sabazmenders@proton.me>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
