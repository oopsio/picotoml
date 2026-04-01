#ifndef PICOTOML_H
#define PICOTOML_H

/**
 * picotoml.h - A single-header, small, C99-compliant TOML parser.
 * Following the TOML v1.0.0 specification.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    PICOTOML_TYPE_STRING,
    PICOTOML_TYPE_INTEGER,
    PICOTOML_TYPE_FLOAT,
    PICOTOML_TYPE_BOOLEAN,
    PICOTOML_TYPE_DATETIME,
    PICOTOML_TYPE_ARRAY,
    PICOTOML_TYPE_TABLE,
    PICOTOML_TYPE_NULL
} picotoml_type;

typedef struct picotoml_datetime {
    int year, month, day;
    int hour, minute, second;
    int millisecond;
    bool has_date;
    bool has_time;
    bool has_offset;
    int offset_hour;
    int offset_minute;
} picotoml_datetime;

typedef struct picotoml_node {
    picotoml_type type;
    char* key;
    bool is_inline;
    union {
        char* string;
        int64_t integer;
        double floating;
        bool boolean;
        picotoml_datetime datetime;
        struct picotoml_node* array_head;
        struct picotoml_node* table_head;
    } value;
    struct picotoml_node* next;
} picotoml_node;

picotoml_node* picotoml_parse(const char* input, char** error_out);
void picotoml_free(picotoml_node* node);
picotoml_node* picotoml_get(picotoml_node* table, const char* key);
picotoml_node* picotoml_at(picotoml_node* array, size_t index);
size_t picotoml_size(picotoml_node* node);

#ifdef __cplusplus
}
#endif

#endif

#ifdef PICOTOML_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

static void* pt_malloc(size_t s) { return malloc(s); }
static void* pt_calloc(size_t n, size_t s) { return calloc(n, s); }
static char* pt_strdup(const char* s) {
    if (!s) return NULL;
    size_t l = strlen(s);
    char* d = (char*)pt_malloc(l + 1);
    memcpy(d, s, l + 1);
    return d;
}

static void pt_error(char** error_out, const char* fmt, ...) {
    if (!error_out) return;
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if (*error_out) free(*error_out);
    *error_out = pt_strdup(buffer);
}

static picotoml_node* pt_node_new(picotoml_type type) {
    picotoml_node* n = (picotoml_node*)pt_calloc(1, sizeof(picotoml_node));
    if (n) n->type = type;
    return n;
}

void picotoml_free(picotoml_node* node) {
    while (node) {
        picotoml_node* next = node->next;
        if (node->key) free(node->key);
        if (node->type == PICOTOML_TYPE_STRING) free(node->value.string);
        else if (node->type == PICOTOML_TYPE_ARRAY) picotoml_free(node->value.array_head);
        else if (node->type == PICOTOML_TYPE_TABLE) picotoml_free(node->value.table_head);
        free(node);
        node = next;
    }
}

picotoml_node* picotoml_get(picotoml_node* table, const char* key) {
    if (!table || table->type != PICOTOML_TYPE_TABLE || !key) return NULL;
    for (picotoml_node* curr = table->value.table_head; curr; curr = curr->next)
        if (curr->key && strcmp(curr->key, key) == 0) return curr;
    return NULL;
}

picotoml_node* picotoml_at(picotoml_node* array, size_t index) {
    if (!array || array->type != PICOTOML_TYPE_ARRAY) return NULL;
    picotoml_node* curr = array->value.array_head;
    while (curr && index--) curr = curr->next;
    return curr;
}

static void pt_append(picotoml_node** head, picotoml_node* entry) {
    while (*head) head = &(*head)->next;
    *head = entry;
}

typedef enum {
    PT_TOK_EOF, PT_TOK_EQUALS, PT_TOK_DOT, PT_TOK_COMMA,
    PT_TOK_LBRACKET, PT_TOK_RBRACKET, PT_TOK_LBRACE, PT_TOK_RBRACE,
    PT_TOK_DBRACKET_OPEN, PT_TOK_DBRACKET_CLOSE,
    PT_TOK_BARE, PT_TOK_STRING, PT_TOK_ERROR
} pt_token_type;

typedef struct {
    const char* p;
    int line;
    pt_token_type type;
    char* s_val;
} pt_state;

static void pt_skip_ws(pt_state* s) {
    while (*s->p) {
        if (isspace((unsigned char)*s->p)) { if (*s->p == '\n') s->line++; s->p++; }
        else if (*s->p == '#') { while (*s->p && *s->p != '\n') s->p++; }
        else break;
    }
}

static char* pt_scan_string(pt_state* s, bool multi, bool literal) {
    size_t len = 0, cap = 32;
    char* res = (char*)pt_malloc(cap);
    char q = literal ? '\'' : '\"';
    s->p += (multi ? 3 : 1);
    if (multi && *s->p == '\n') { s->p++; s->line++; }
    while (*s->p) {
        if (multi && s->p[0] == q && s->p[1] == q && s->p[2] == q) { s->p += 3; res[len] = 0; return res; }
        if (!multi && *s->p == q) { s->p++; res[len] = 0; return res; }
        if (!multi && *s->p == '\n') break;
        if (!literal && *s->p == '\\') {
            s->p++;
            if (multi && (*s->p == '\n' || (*s->p == '\r' && s->p[1] == '\n'))) {
                if (*s->p == '\r') s->p++; s->p++; s->line++;
                while (*s->p && isspace((unsigned char)*s->p)) { if (*s->p == '\n') s->line++; s->p++; }
                continue;
            }
            char c = *s->p++;
            switch(c) {
                case 'b': c = '\b'; break; case 't': c = '\t'; break; case 'n': c = '\n'; break;
                case 'f': c = '\f'; break; case 'r': c = '\r'; break; case 'e': c = 27; break;
                case 'x': { char h[3]={s->p[0],s->p[1],0}; c=(char)strtoul(h,0,16); s->p+=2; break; }
                case 'u': case 'U': { int n=(c=='u'?4:8); char h[9]={0}; for(int i=0;i<n;i++) h[i]=s->p[i]; c=(char)strtoul(h,0,16); s->p+=n; break; }
            }
            if (len+1 >= cap) res = (char*)realloc(res, cap *= 2);
            res[len++] = c;
        } else {
            if (len+1 >= cap) res = (char*)realloc(res, cap *= 2);
            if (*s->p == '\n') s->line++;
            res[len++] = *s->p++;
        }
    }
    free(res); return NULL;
}

static void pt_next(pt_state* s) {
    if (s->s_val) { free(s->s_val); s->s_val = NULL; }
    pt_skip_ws(s);
    if (!*s->p) { s->type = PT_TOK_EOF; return; }
    char c = *s->p;
    if (c == '=') { s->type = PT_TOK_EQUALS; s->p++; return; }
    if (c == '.') { s->type = PT_TOK_DOT; s->p++; return; }
    if (c == ',') { s->type = PT_TOK_COMMA; s->p++; return; }
    if (c == '[') { if (s->p[1] == '[') { s->type = PT_TOK_DBRACKET_OPEN; s->p += 2; } else { s->type = PT_TOK_LBRACKET; s->p++; } return; }
    if (c == ']') { if (s->p[1] == ']') { s->type = PT_TOK_DBRACKET_CLOSE; s->p += 2; } else { s->type = PT_TOK_RBRACKET; s->p++; } return; }
    if (c == '{') { s->type = PT_TOK_LBRACE; s->p++; return; }
    if (c == '}') { s->type = PT_TOK_RBRACE; s->p++; return; }
    if (c == '\"' || c == '\'') {
        bool m = (s->p[0]==c && s->p[1]==c && s->p[2]==c);
        s->s_val = pt_scan_string(s, m, c=='\'');
        s->type = s->s_val ? PT_TOK_STRING : PT_TOK_ERROR; return;
    }
    const char* start = s->p;
    while (*s->p && (isalnum((unsigned char)*s->p) || strchr("-_:+.T", *s->p))) s->p++;
    if (s->p > start) {
        s->type = PT_TOK_BARE;
        s->s_val = (char*)pt_malloc(s->p - start + 1);
        memcpy(s->s_val, start, s->p - start); s->s_val[s->p - start] = 0;
        return;
    }
    s->type = PT_TOK_ERROR;
}

static picotoml_node* pt_parse_value(pt_state* s, char** err);

static picotoml_node* pt_parse_literal(pt_state* s) {
    char* v = s->s_val;
    if (strcmp(v, "true") == 0) { picotoml_node* n = pt_node_new(PICOTOML_TYPE_BOOLEAN); n->value.boolean = true; return n; }
    if (strcmp(v, "false") == 0) { picotoml_node* n = pt_node_new(PICOTOML_TYPE_BOOLEAN); n->value.boolean = false; return n; }
    if (strcmp(v, "inf") == 0 || strcmp(v, "+inf") == 0) { picotoml_node* n = pt_node_new(PICOTOML_TYPE_FLOAT); n->value.floating = INFINITY; return n; }
    if (strcmp(v, "-inf") == 0) { picotoml_node* n = pt_node_new(PICOTOML_TYPE_FLOAT); n->value.floating = -INFINITY; return n; }
    if (strcmp(v, "nan") == 0 || strcmp(v, "+nan") == 0 || strcmp(v, "-nan") == 0) { picotoml_node* n = pt_node_new(PICOTOML_TYPE_FLOAT); n->value.floating = NAN; return n; }
    
    if (strlen(v) >= 10 && v[4] == '-' && v[7] == '-') {
        picotoml_node* n = pt_node_new(PICOTOML_TYPE_DATETIME); n->value.datetime.has_date = true;
        sscanf(v, "%d-%d-%d", &n->value.datetime.year, &n->value.datetime.month, &n->value.datetime.day);
        char* t = strpbrk(v, "T ");
        if (t) { n->value.datetime.has_time = true; sscanf(t+1, "%d:%d:%d", &n->value.datetime.hour, &n->value.datetime.minute, &n->value.datetime.second);
            char* o = strpbrk(t+1, "Z+-"); if (o) { n->value.datetime.has_offset = true; if (*o != 'Z') sscanf(o+1, "%d:%d", &n->value.datetime.offset_hour, &n->value.datetime.offset_minute); }
        }
        return n;
    }

    char* clean = (char*)pt_malloc(strlen(v)+1), *end, *p = v, *q = clean;
    while (*p) { if (*p != '_') *q++ = *p; p++; } *q = 0;
    picotoml_node* n = NULL;
    if (strpbrk(clean, ".eE") && clean[1] != 'x' && clean[1] != 'b' && clean[1] != 'o') { n = pt_node_new(PICOTOML_TYPE_FLOAT); n->value.floating = strtod(clean, &end); }
    else {
        n = pt_node_new(PICOTOML_TYPE_INTEGER);
        if (clean[0] == '0' && clean[1] == 'x') n->value.integer = (int64_t)strtoll(clean + 2, &end, 16);
        else if (clean[0] == '0' && clean[1] == 'o') n->value.integer = (int64_t)strtoll(clean + 2, &end, 8);
        else if (clean[0] == '0' && clean[1] == 'b') n->value.integer = (int64_t)strtoll(clean + 2, &end, 2);
        else n->value.integer = (int64_t)strtoll(clean, &end, 10);
    }
    free(clean); return n;
}

static picotoml_node* pt_parse_value(pt_state* s, char** err) {
    picotoml_node* n = NULL;
    if (s->type == PT_TOK_STRING) { n = pt_node_new(PICOTOML_TYPE_STRING); n->value.string = pt_strdup(s->s_val); pt_next(s); }
    else if (s->type == PT_TOK_BARE) { n = pt_parse_literal(s); pt_next(s); }
    else if (s->type == PT_TOK_LBRACKET) {
        n = pt_node_new(PICOTOML_TYPE_ARRAY); pt_next(s);
        while (s->type != PT_TOK_RBRACKET && s->type != PT_TOK_EOF) {
            if (s->type == PT_TOK_COMMA) { pt_next(s); continue; }
            picotoml_node* v = pt_parse_value(s, err); if (!v) return NULL;
            pt_append(&n->value.array_head, v); if (s->type == PT_TOK_COMMA) pt_next(s); else if (s->type != PT_TOK_RBRACKET) break;
        }
        if (s->type == PT_TOK_RBRACKET) pt_next(s);
    } else if (s->type == PT_TOK_LBRACE) {
        n = pt_node_new(PICOTOML_TYPE_TABLE); n->is_inline = true; pt_next(s);
        while (s->type != PT_TOK_RBRACE && s->type != PT_TOK_EOF) {
            if (s->type == PT_TOK_COMMA) { pt_next(s); continue; }
            char* k = pt_strdup(s->s_val); pt_next(s); pt_next(s);
            picotoml_node* v = pt_parse_value(s, err); v->key = k; pt_append(&n->value.table_head, v);
            if (s->type == PT_TOK_COMMA) pt_next(s);
        }
        if (s->type == PT_TOK_RBRACE) pt_next(s);
    }
    return n;
}

picotoml_node* picotoml_parse(const char* input, char** error_out) {
    pt_state s = {input, 1, PT_TOK_EOF, NULL}; pt_next(&s);
    picotoml_node* root = pt_node_new(PICOTOML_TYPE_TABLE), *curr = root;
    while (s.type != PT_TOK_EOF) {
        if (s.type == PT_TOK_LBRACKET || s.type == PT_TOK_DBRACKET_OPEN) {
            bool dbl = (s.type == PT_TOK_DBRACKET_OPEN); pt_next(&s); curr = root;
            while (true) {
                char* k = pt_strdup(s.s_val); pt_next(&s);
                if (s.type == PT_TOK_DOT) {
                    picotoml_node* existing = picotoml_get(curr, k);
                    if (!existing) { existing = pt_node_new(PICOTOML_TYPE_TABLE); existing->key = pt_strdup(k); pt_append(&curr->value.table_head, existing); }
                    curr = existing; free(k); pt_next(&s);
                } else {
                    if (dbl) {
                        picotoml_node* arr = picotoml_get(curr, k);
                        if (!arr) { arr = pt_node_new(PICOTOML_TYPE_ARRAY); arr->key = pt_strdup(k); pt_append(&curr->value.table_head, arr); }
                        picotoml_node* tbl = pt_node_new(PICOTOML_TYPE_TABLE); pt_append(&arr->value.array_head, tbl); curr = tbl;
                    } else {
                        picotoml_node* existing = picotoml_get(curr, k);
                        if (!existing) { existing = pt_node_new(PICOTOML_TYPE_TABLE); existing->key = pt_strdup(k); pt_append(&curr->value.table_head, existing); }
                        curr = existing;
                    }
                    free(k); if (s.type == PT_TOK_RBRACKET || s.type == PT_TOK_DBRACKET_CLOSE) pt_next(&s); break;
                }
            }
        } else if (s.type == PT_TOK_BARE || s.type == PT_TOK_STRING) {
            char* k = pt_strdup(s.s_val); pt_next(&s); picotoml_node* target = curr;
            while (s.type == PT_TOK_DOT) {
                picotoml_node* existing = picotoml_get(target, k);
                if (!existing) { existing = pt_node_new(PICOTOML_TYPE_TABLE); existing->key = pt_strdup(k); pt_append(&target->value.table_head, existing); }
                target = existing; free(k); pt_next(&s); k = pt_strdup(s.s_val); pt_next(&s);
            }
            if (s.type == PT_TOK_EQUALS) { pt_next(&s); picotoml_node* v = pt_parse_value(&s, error_out); if (v) { v->key = k; pt_append(&target->value.table_head, v); } }
            else free(k);
        } else pt_next(&s);
    }
    return root;
}
#endif
