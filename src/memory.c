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
    HashMap units; // string -> int
};

Memory memory_new(Arena *arena) {
    return (Memory) {
        .vars = hash_map_new(sizeof(Expression), arena),
        .units = hash_map_new(sizeof(int), arena),
    };
}

bool memory_contains_unit(Memory mem, unsigned char *unit_name) {
    debug("Checking for unit: %s\n", unit_name);
    for (size_t i = 0; i < mem.units.capacity; i++) {
        if (mem.units.exists[i]) {
            debug("Key: %s\n", mem.units.items[i].key);
        }
    }
    bool result = hash_map_contains(mem.units, unit_name);
    debug("found unit: %d\n", result);
    return result;
}

void memory_add_unit(Memory *mem, unsigned char *unit_name, Arena *arena) {
    assert(!memory_contains_unit(*mem, unit_name));
    int unit_type = unit_type_user_min() + mem->units.size;
    hash_map_insert(&mem->units, unit_name, (void *)&unit_type, arena);
}

const UnitBasic memory_get_unit(Memory mem, unsigned char *unit_name) {
    assert(hash_map_contains(mem.units, (unsigned char *)unit_name));
    int unit_type = *(int *)hash_map_get(mem.units, (unsigned char *)unit_name);
    return (UnitBasic) { .type = unit_type, .name = (char *)unit_name };
}

void memory_add_var(Memory *mem, unsigned char *var_name, Expression value, Arena *arena) {
    hash_map_insert(&mem->vars, var_name, (void *)&value, arena);
}

bool memory_contains_var(Memory mem, unsigned char *var_name) {
    debug("Checking for var: %s\n", var_name);
    for (size_t i = 0; i < mem.vars.capacity; i++) {
        if (mem.vars.exists[i]) {
            debug("Key: %s\n", mem.vars.items[i].key);
        }
    }
    bool result = hash_map_contains(mem.vars, (unsigned char *)var_name);
    debug("found var: %d\n", result);
    return result;
}

const Expression memory_get_var(Memory mem, unsigned char *var_name) {
    assert(hash_map_contains(mem.vars, var_name));
    return *(Expression *)hash_map_get(mem.vars, var_name);
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

// Show all the variables in memory
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

