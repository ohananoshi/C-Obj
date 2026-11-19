#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c_string.h"

//A new string value with len chars from start
static cobj str_from(const char* start, size_t len){
    char* text = (char*)cobj_malloc(len + 1);
    if(text == NULL) return cobj_empty();

    if(len > 0) memcpy(text, start, len);
    text[len] = '\0';
    return cobj_box(&COBJ_STR, text);
}

//Moves the buffer's text into a string value
static cobj buf_value(cobj_buf* buf){
    if(buf->data == NULL) return str_from("", 0);
    return cobj_box(&COBJ_STR, buf->data);
}

cobj cobj_string_format(const char* fmt, ...){
    if(fmt == NULL) return cobj_empty();

    cobj_buf buf = COBJ_BUF_INIT;
    va_list args;
    va_start(args, fmt);
    cobj_status status = cobj_buf_vprintf(&buf, fmt, args);
    va_end(args);

    if(status != COBJ_OK){
        cobj_buf_free(&buf);
        return cobj_empty();
    }
    return buf_value(&buf);
}

cobj cobj_string_concat(const char* a, const char* b){
    if(a == NULL || b == NULL) return cobj_empty();

    cobj_buf buf = COBJ_BUF_INIT;
    if(cobj_buf_puts(&buf, a) != COBJ_OK || cobj_buf_puts(&buf, b) != COBJ_OK){
        cobj_buf_free(&buf);
        return cobj_empty();
    }
    return buf_value(&buf);
}

cobj cobj_string_repeat(const char* s, size_t times){
    if(s == NULL) return cobj_empty();

    size_t len = strlen(s);
    if(times > 0 && len > (SIZE_MAX - 1) / times) return cobj_empty();

    cobj_buf buf = COBJ_BUF_INIT;
    for(size_t i = 0; i < times; i++){
        if(cobj_buf_putn(&buf, s, len) != COBJ_OK){
            cobj_buf_free(&buf);
            return cobj_empty();
        }
    }
    return buf_value(&buf);
}

//A Python slice position: negative counts from the end, then clamped to [0, len]
static size_t clamp_position(ptrdiff_t position, size_t len){
    if(position < 0){
        size_t back = (size_t)(-(position + 1)) + 1;
        return back >= len ? 0 : len - back;
    }
    return (size_t)position > len ? len : (size_t)position;
}

cobj cobj_string_slice(const char* s, ptrdiff_t start, ptrdiff_t end){
    if(s == NULL) return cobj_empty();

    size_t len = strlen(s);
    size_t from = clamp_position(start, len);
    size_t to = clamp_position(end, len);
    return str_from(s + from, to > from ? to - from : 0);
}

//A copy of s with every char passed through convert
static cobj map_chars(const char* s, int (*convert)(int)){
    if(s == NULL) return cobj_empty();

    cobj value = str_from(s, strlen(s));
    for(char* c = (char*)value.as.p; c != NULL && *c != '\0'; c++) *c = (char)convert((unsigned char)*c);
    return value;
}

cobj cobj_string_upper(const char* s){
    return map_chars(s, toupper);
}

cobj cobj_string_lower(const char* s){
    return map_chars(s, tolower);
}

static bool strippable(char c, const char* chars){
    return chars == NULL ? isspace((unsigned char)c) != 0 : (c != '\0' && strchr(chars, c) != NULL);
}

cobj cobj_string_strip(const char* s, const char* chars){
    if(s == NULL) return cobj_empty();

    const char* start = s;
    const char* end = s + strlen(s);
    while(start < end && strippable(*start, chars)) start++;
    while(end > start && strippable(end[-1], chars)) end--;
    return str_from(start, (size_t)(end - start));
}

cobj cobj_string_replace(const char* s, const char* old_value, const char* new_value){
    if(s == NULL || old_value == NULL || new_value == NULL) return cobj_empty();

    size_t old_len = strlen(old_value);
    if(old_len == 0) return str_from(s, strlen(s));

    cobj_buf buf = COBJ_BUF_INIT;
    cobj_status status = COBJ_OK;
    const char* rest = s;

    for(const char* found = strstr(rest, old_value); found != NULL && status == COBJ_OK; found = strstr(rest, old_value)){
        status = cobj_buf_putn(&buf, rest, (size_t)(found - rest));
        if(status == COBJ_OK) status = cobj_buf_puts(&buf, new_value);
        rest = found + old_len;
    }
    if(status == COBJ_OK) status = cobj_buf_puts(&buf, rest);

    if(status != COBJ_OK){
        cobj_buf_free(&buf);
        return cobj_empty();
    }
    return buf_value(&buf);
}

ptrdiff_t cobj_string_find(const char* s, const char* sub){
    if(s == NULL || sub == NULL) return -1;

    const char* found = strstr(s, sub);
    return found != NULL ? found - s : -1;
}

size_t cobj_string_count(const char* s, const char* sub){
    if(s == NULL || sub == NULL || sub[0] == '\0') return 0;

    size_t count = 0;
    size_t sub_len = strlen(sub);
    for(const char* found = strstr(s, sub); found != NULL; found = strstr(found + sub_len, sub)) count++;
    return count;
}

bool cobj_string_starts_with(const char* s, const char* prefix){
    return s != NULL && prefix != NULL && strncmp(s, prefix, strlen(prefix)) == 0;
}

bool cobj_string_ends_with(const char* s, const char* suffix){
    if(s == NULL || suffix == NULL) return false;

    size_t len = strlen(s);
    size_t suffix_len = strlen(suffix);
    return suffix_len <= len && strcmp(s + len - suffix_len, suffix) == 0;
}

//Pushes a new string of len chars from start; frees the list when that fails
static bool push_piece(cobj_list* list, const char* start, size_t len){
    if(cobj_list_push(list, str_from(start, len)) == COBJ_OK) return true;

    cobj_list_free(list);
    return false;
}

static cobj_list* split_whitespace(const char* s, cobj_list* list){
    const char* c = s;

    for(;;){
        while(*c != '\0' && isspace((unsigned char)*c)) c++;
        if(*c == '\0') return list;

        const char* start = c;
        while(*c != '\0' && !isspace((unsigned char)*c)) c++;
        if(!push_piece(list, start, (size_t)(c - start))) return NULL;
    }
}

cobj_list* cobj_string_split(const char* s, const char* sep){
    if(s == NULL || (sep != NULL && sep[0] == '\0')) return NULL;

    cobj_list* list = cobj_list_new();
    if(list == NULL) return NULL;
    if(sep == NULL) return split_whitespace(s, list);

    size_t sep_len = strlen(sep);
    const char* start = s;
    for(const char* found = strstr(start, sep); found != NULL; found = strstr(start, sep)){
        if(!push_piece(list, start, (size_t)(found - start))) return NULL;
        start = found + sep_len;
    }

    if(!push_piece(list, start, strlen(start))) return NULL;
    return list;
}

cobj cobj_string_join(const cobj* iterable, const char* sep){
    if(!cobj_iterable(iterable) || sep == NULL) return cobj_empty();

    cobj_buf buf = COBJ_BUF_INIT;
    bool first = true;

    COBJ_FOREACH(item, iterable){
        const char* text = cobj_as_cstr(&item);
        if(text == NULL || (!first && cobj_buf_puts(&buf, sep) != COBJ_OK) || cobj_buf_puts(&buf, text) != COBJ_OK){
            cobj_buf_free(&buf);
            return cobj_empty();
        }
        first = false;
    }
    return buf_value(&buf);
}

cobj cobj_string_read_file(const char* path){
    if(path == NULL) return cobj_empty();

    FILE* file = fopen(path, "r");
    if(file == NULL) return cobj_empty();

    long size = -1;
    if(fseek(file, 0, SEEK_END) == 0) size = ftell(file);
    rewind(file);

    char* text = size >= 0 ? (char*)cobj_malloc((size_t)size + 1) : NULL;
    if(text == NULL){
        fclose(file);
        return cobj_empty();
    }

    size_t read = fread(text, 1, (size_t)size, file);
    text[read] = '\0';
    fclose(file);
    return cobj_box(&COBJ_STR, text);
}

cobj_status cobj_string_write_file(const char* path, const char* text, const char* mode){
    if(path == NULL || text == NULL || mode == NULL) return COBJ_EINVAL;

    FILE* file = fopen(path, mode);
    if(file == NULL) return COBJ_EIO;

    size_t len = strlen(text);
    bool written = fwrite(text, 1, len, file) == len;
    if(fclose(file) != 0) written = false;
    return written ? COBJ_OK : COBJ_EIO;
}
