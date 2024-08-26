#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "arena.c"
#include "debug.c"

typedef enum UnitType UnitType;
enum UnitType {
    // Distance
    UNIT_CENTIMETER,
    UNIT_METER,
    UNIT_KILOMETER,
    UNIT_INCH,
    UNIT_FOOT,
    UNIT_MILE,
    // Time
    UNIT_SECOND,
    UNIT_MINUTE,
    UNIT_HOUR,
    // Mass
    UNIT_GRAM,
    UNIT_KILOGRAM,
    UNIT_POUND,
    UNIT_OUNCE,

    UNIT_COUNT,
    UNIT_NONE,
    UNIT_UNKNOWN,
};

#define MAX_UNIT_STRING 7

const char *unit_strings[] = {
    // Distance
    "cm",
    "m",
    "km",
    "in",
    "ft",
    "mi",
    // Time
    "s",
    "min",
    "h",
    // Mass
    "g",
    "kg",
    "lb",
    "oz",

    "",
    "none",
    "unknown",
};

typedef enum UnitCategory UnitCategory;
enum UnitCategory {
    UNIT_CATEGORY_DISTANCE,
    UNIT_CATEGORY_TIME,
    UNIT_CATEGORY_MASS,
    UNIT_CATEGORY_NONE,
};

UnitCategory unit_category(UnitType type) {
    switch (type) {
        case UNIT_CENTIMETER:
        case UNIT_METER:
        case UNIT_KILOMETER:
        case UNIT_INCH:
        case UNIT_FOOT:
        case UNIT_MILE:
            return UNIT_CATEGORY_DISTANCE;
        case UNIT_SECOND:
        case UNIT_MINUTE:
        case UNIT_HOUR:
            return UNIT_CATEGORY_TIME;
        case UNIT_GRAM:
        case UNIT_KILOGRAM:
        case UNIT_POUND:
        case UNIT_OUNCE:
            return UNIT_CATEGORY_MASS;
        case UNIT_COUNT:
        case UNIT_NONE:
        case UNIT_UNKNOWN:
            return UNIT_CATEGORY_NONE;
    }
}

// TODO: think more about precision
double unit_conversion[UNIT_COUNT][UNIT_COUNT] = {
    //     cm, m, km, in, ft, mi, s, min, h, g, kg, oz, lb
    /*cm*/ {1, 0.01, 0.00001, 1/2.54, 1/(2.54*12), 1/(2.54*12*5280), 0, 0, 0, 0, 0, 0, 0, },
    /*m */ {100, 1, 0.001, 39.3700787, 3.2808399, 3.2808399/5280, 0, 0, 0, 0, 0, 0, 0, },
    /*km*/ {100000, 1000, 1, 39370.0787, 3280.8399, 0.62137119, 0, 0, 0, 0, 0, 0, 0, },
    /*in*/ {2.54, 0.0254, 0.0000254, 1, 1.0/12, 1.0/(12*5280), 0, 0, 0, 0, 0, 0, 0, },
    /*ft*/ {30.48, 0.3048, 0.0003048, 12, 1, 1.0/5280, 0, 0, 0, 0, 0, 0, 0, },
    /*mi*/ {160934.4, 1609.344, 1.609344, 63360, 5280, 1, 0, 0, 0, 0, 0, 0, 0, },
    /*s*/  {0, 0, 0, 0, 0, 0, 1, 1.0/60, 1.0/3600, 0, 0, 0, 0, },
    /*min*/{0, 0, 0, 0, 0, 0, 60, 1, 1.0/60, 0, 0, 0, 0, },
    /*h*/  {0, 0, 0, 0, 0, 0, 3600, 60, 1, 0, 0, 0, 0, },
    /*g*/  {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0.001, 0.0352739619, 0.00220462262, },
    /*kg*/ {0, 0, 0, 0, 0, 0, 0, 0, 0, 1000, 1, 35.2739619, 2.20462262, },
    /*oz*/ {0, 0, 0, 0, 0, 0, 0, 0, 0, 28.3495231, 0.0283495231, 1, 1.0/16, },
    /*lb*/ {0, 0, 0, 0, 0, 0, 0, 0, 0, 453.59237, 0.45359237, 16, 1, },
};

typedef struct Unit Unit;
struct Unit {
    UnitType *types;
    int *degrees;
    size_t length;
};

bool is_unit_none(Unit unit) {
    return unit.length == 1 && unit.types[0] == UNIT_NONE;
}

bool is_unit_unknown(Unit unit) {
    return unit.length == 1 && unit.types[0] == UNIT_UNKNOWN;
}

Unit unit_new(UnitType types[], int degrees[], size_t length, Arena *arena) {
    UnitType *new_types = arena_alloc(arena, length * sizeof(UnitType));
    int *new_degrees = arena_alloc(arena, length * sizeof(int));
    memcpy(new_types, types, length * sizeof(UnitType));
    memcpy(new_degrees, degrees, length * sizeof(int));
    return (Unit) { .types = new_types, .degrees = new_degrees, .length = length };
}

Unit unit_new_single(UnitType type, int degree, Arena *arena) {
    return unit_new(&type, &degree, 1, arena);
}

Unit unit_new_none(Arena *arena) {
    return unit_new_single(UNIT_NONE, 0, arena);
}

Unit unit_new_unknown(Arena *arena) {
    return unit_new_single(UNIT_UNKNOWN, 0, arena);
}

Unit unit_combine(Unit a, Unit b, Arena *arena) {
    assert(!is_unit_unknown(a));
    assert(!is_unit_unknown(b));
    if (is_unit_none(a)) return b;
    if (is_unit_none(b)) return a;
    bool *a_leftover = arena_alloc(arena, a.length * sizeof(bool));
    bool *b_leftover = arena_alloc(arena, b.length * sizeof(bool));
    memset(a_leftover, true, a.length * sizeof(bool));
    memset(b_leftover, true, b.length * sizeof(bool));
    size_t length = a.length + b.length;
    for (size_t i = 0; i < b.length; i++) {
        for (size_t j = 0; j < a.length; j++) {
            if (a.types[j] == b.types[i] && a.degrees[j] == -b.degrees[i]) {
                a_leftover[j] = false;
                b_leftover[i] = false;
                length -= 2;
            } else if (a.types[j] == b.types[i]) {
                a.degrees[j] += b.degrees[i];
                b_leftover[i] = false;
                length -= 1;
            }
        }
    }
    UnitType *types = arena_alloc(arena, length * sizeof(UnitType));
    int *degrees = arena_alloc(arena, length * sizeof(int));
    size_t curr_length = 0;
    for (size_t i = 0; i < a.length; i++) {
        if (a_leftover[i]) {
            types[curr_length] = a.types[i];
            degrees[curr_length] = a.degrees[i];
            curr_length++;
        }
    }
    for (size_t i = 0; i < b.length; i++) {
        if (b_leftover[i]) {
            types[curr_length] = b.types[i];
            degrees[curr_length] = b.degrees[i];
            curr_length++;
        }
    }
    return (Unit) { .types = types, .degrees = degrees, .length = length };
}

bool units_equal(Unit a, Unit b) {
    if (a.length != b.length) {
        printf("Lengths not equal: Expected: %zu Got: %zu\n", b.length, a.length);
        return false;
    }
    for (size_t i = 0; i < a.length; i++) {
        if (a.types[i] != b.types[i]) {
            printf("Types not equal: Expected: %d Got: %d\n", b.types[i], a.types[i]);
            return false;
        }
        if (a.degrees[i] != b.degrees[i]) {
            printf("Degrees not equal: Expected: %d Got: %d\n", b.degrees[i], a.degrees[i]);
            return false;
        }
    }
    return true;
}

#define MAX_UNITS_DISPLAY 32
#define MAX_DEGREE_STRING 4
#define MAX_UNIT_WITH_DEGREE_STRING MAX_UNIT_STRING + MAX_DEGREE_STRING
#define MAX_COMPOSITE_UNIT_STRING MAX_UNITS_DISPLAY * MAX_UNIT_WITH_DEGREE_STRING

char *display_unit(Unit unit, Arena *arena) {
    if (unit.length == 0 || (unit.length == 1 && unit.types[0] == UNIT_NONE)) {
        return "none";
    }
    if (unit.length == 1 && unit.types[0] == UNIT_UNKNOWN) {
        return "unknown";
    }
    char *str = arena_alloc(arena, MAX_COMPOSITE_UNIT_STRING);
    strncpy(str, "", 1);
    for (size_t i = 0; i < unit.length; i++) {
        char unit_str[MAX_UNIT_WITH_DEGREE_STRING];
        if (unit.degrees[i] == 1) {
            snprintf(unit_str, sizeof(unit_str), "%s", unit_strings[unit.types[i]]);
        } else {
            snprintf(unit_str, sizeof(unit_str), "%s^%d", unit_strings[unit.types[i]], unit.degrees[i]);
        }
        strncat(str, unit_str, MAX_UNIT_WITH_DEGREE_STRING);
        if (i < unit.length - 1) {
            strncat(str, " ", 1);
        }
    }
    return str;
}
