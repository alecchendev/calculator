#pragma once

#include <stdarg.h>
#include <stdio.h>
#include "arena.c"
#include "debug.c"

typedef struct String String;
struct String {
    char *s;
    size_t len;
    Arena *arena;
};

String string_empty(Arena *arena) {
    return (String) { .s = "", .len = 0, .arena = arena };
}

String string_new(char *s, size_t len, Arena *arena) {
    char *s_new = arena_alloc(arena, len);
    memcpy(s_new, s, len);
    return (String) { .s = s_new, .len = len, .arena = arena };
}

// Basically printf for a string!
String string_new_fmt(Arena *arena, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    // Determine required size
    va_list ap_dup;
    va_copy(ap_dup, ap);
    int size = vsnprintf(NULL, 0, fmt, ap_dup);
    va_end(ap_dup);

    if (size < 0) {
        va_end(ap);
        return string_empty(arena);
    }

    // Allocate and write string
    String str = string_empty(arena);
    str.len = size + 1;
    str.s = arena_alloc(arena, str.len);

    int ret = vsnprintf(str.s, str.len, fmt, ap);
    va_end(ap);
    assert(ret != -1);

    return str;
}

String string_concat(String s1, String s2, Arena *arena) {
    size_t s1_len = s1.len > 0 ? s1.len - 1 : 0; // Drop null terminator
    size_t s2_len = s2.len;
    size_t len_new = s1_len + s2_len;
    char *s_new = arena_alloc(arena, len_new);
    memcpy(s_new, s1.s, s1_len);
    memcpy(&s_new[s1_len], s2.s, s2_len);
    return (String) { .s = s_new, .len = len_new, .arena = arena };
}
