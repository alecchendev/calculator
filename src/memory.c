#pragma once

#include <stdio.h>
#include "debug.c"
#include "expression.c"
#include "hash_map.c"
#include "string.c"
#include "unit.c"

// Structures for tracking user defined things
// we want to track between different executions.

typedef struct Memory Memory;
struct Memory {
    HashMap vars; // string -> Expression
};

Memory memory_new(Arena *arena) {
    return (Memory) { .vars = hash_map_new(sizeof(Expression), arena) };
}

void memory_add_var(Memory *mem, unsigned char *var_name, Expression value, Arena *arena) {
    hash_map_insert(&mem->vars, (unsigned char *)var_name, (void *)&value, arena);
}

bool memory_contains_var(Memory mem, unsigned char *var_name) {
    debug("Checking for var: %s\n", var_name);
    for (size_t i = 0; i < mem.vars.capacity; i++) {
        if (mem.vars.exists[i]) {
            debug("Key: %s\n", mem.vars.items[i].key);
        }
    }
    bool result = hash_map_contains(mem.vars, (unsigned char *)var_name);
    debug("found: %d\n", result);
    return result;
}

const Expression memory_get_var(Memory mem, unsigned char *var_name) {
    assert(hash_map_contains(mem.vars, (unsigned char *)var_name));
    return *(Expression *)hash_map_get(mem.vars, (unsigned char *)var_name);
}

String display_var(const unsigned char *var_name, const Expression value, bool newline, Arena *arena) {
    if (value.type == EXPR_CONST_UNIT) {
        double constant = value.expr.binary_expr.left->expr.constant;
        Unit unit = value.expr.binary_expr.right->expr.unit;
        return string_new_fmt(arena, "%s = %g%s%s%s", var_name, constant, is_unit_none(unit) ? "" : " ", display_unit(unit, arena), newline ? "\n" : "");
    } else if (value.type == EXPR_UNIT) {
        Unit unit = value.expr.unit;
        return string_new_fmt(arena, "%s = %s%s", var_name, display_unit(unit, arena), newline ? "\n" : "");
    }
    assert(false);
    return string_empty(arena);
}

String memory_show(Memory mem, Arena *arena) {
    String s = string_empty(arena);
    size_t seen = 0;
    for (size_t i = 0; i < mem.vars.capacity; i++) {
        if (mem.vars.exists[i]) {
            KeyValue item = mem.vars.items[i];
            String line = display_var(item.key, *(Expression *)item.value, seen < mem.vars.size - 1, arena);
            debug("Memory show: %s\n", line.s);
            s = string_concat(s, line, arena);
            seen += 1;
        }
    }
    return s;
}

