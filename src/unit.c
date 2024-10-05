#pragma once

#include <math.h>
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

    UNIT_NONE,
    UNIT_COUNT,
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
    "none",
    "",
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
        case UNIT_NONE:
        case UNIT_COUNT:
        case UNIT_UNKNOWN:
            return UNIT_CATEGORY_NONE;
    }
}

double to_meters(UnitType from) {
    switch (from) {
        case UNIT_CENTIMETER: return 0.01;
        case UNIT_METER: return 1;
        case UNIT_KILOMETER: return 1000;
        case UNIT_INCH: return 0.0254;
        case UNIT_FOOT: return 0.3048;
        case UNIT_MILE: return 1609.344;
        default:
            assert(false);
            return 0;
    }
}

double to_seconds(UnitType from) {
    switch (from) {
        case UNIT_SECOND: return 1;
        case UNIT_MINUTE: return 60;
        case UNIT_HOUR: return 3600;
        default:
            assert(false);
            return 0;
    }
}

double to_kilograms(UnitType from) {
    switch (from) {
        case UNIT_GRAM: return 0.001;
        case UNIT_KILOGRAM: return 1;
        case UNIT_OUNCE: return 0.0283495231;
        case UNIT_POUND: return 0.45359237;
        default:
            assert(false);
            return 0;
    }
}

// TODO: think more about precision, maybe rewrite some things
// as expressions so the compiler can work some magic
double unit_conversion(UnitType from, UnitType to) {
    UnitCategory cat_from = unit_category(from);
    UnitCategory cat_to = unit_category(to);
    if (cat_from != cat_to) return 0;
    switch (cat_from) {
        case UNIT_CATEGORY_DISTANCE:
            return to_meters(from) / to_meters(to);
        case UNIT_CATEGORY_MASS:
            return to_kilograms(from) / to_kilograms(to);
        case UNIT_CATEGORY_TIME:
            return to_seconds(from) / to_seconds(to);
        case UNIT_CATEGORY_NONE:
            return 0;
    }
}

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

#define MAX_UNITS_DISPLAY 32
#define MAX_DEGREE_STRING 4
#define MAX_UNIT_WITH_DEGREE_STRING MAX_UNIT_STRING + MAX_DEGREE_STRING
#define MAX_COMPOSITE_UNIT_STRING MAX_UNITS_DISPLAY * MAX_UNIT_WITH_DEGREE_STRING

char *display_unit(Unit unit, Arena *arena) {
    assert(unit.length > 0);
    if (is_unit_none(unit)) {
#ifdef DEBUG
        return "none";
#else
        return "";
#endif
    }
    if (is_unit_unknown(unit)) {
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

bool units_equal(Unit a, Unit b, Arena *arena) {
    if (a.length != b.length) {
        printf("Lengths not equal: Left: %s Right %s\n",
            display_unit(a, arena), display_unit(b, arena));
        return false;
    }
    // For every element in a, look for it in b.
    bool all_found = true;
    for (size_t i = 0; i < a.length; i++) {
        bool found = false;
        for (size_t j = 0; j < b.length; j++) {
            if (a.types[i] == b.types[j] && a.degrees[i] == b.degrees[j]) {
                found = true;
                break;
            }
        }
        all_found &= found;
    }
    if (!all_found) {
        printf("Units don't match: Left: %s Right %s\n",
            display_unit(a, arena), display_unit(b, arena));
    }
    return all_found;
}

double unit_convert_factor(Unit a, Unit b, Arena *arena) {
    // Should only be called if we are able to convert
    debug("a: %s b: %s a.length: %zu b.length: %zu\n",
          display_unit(a, arena), display_unit(b, arena), a.length, b.length);
    double all_factor = 1;
    for (size_t i = 0; i < a.length; i++) {
        double factor = 1;
        for (size_t j = 0; j < b.length; j++) {
            if (unit_category(a.types[i]) == unit_category(b.types[j])) {
                factor = pow(unit_conversion(a.types[i], b.types[j]), a.degrees[i]);
                debug("Found convertible: left: %s right: %s degree: %d -> factor: %lf\n", unit_strings[a.types[i]], unit_strings[b.types[j]], a.degrees[i], factor);
                break;
            }
        }
        all_factor *= factor;
    }
    return all_factor;
}

Unit unit_combine(Unit a, Unit b, bool reject_same_category, Arena *arena) {
    assert(!is_unit_unknown(a));
    assert(!is_unit_unknown(b));
    if (is_unit_none(a)) return b;
    if (is_unit_none(b)) return a;
    a = unit_new(a.types, a.degrees, a.length, arena); // Dup because we modify heap data
    bool *a_leftover = arena_alloc(arena, a.length * sizeof(bool));
    bool *b_leftover = arena_alloc(arena, b.length * sizeof(bool));
    memset(a_leftover, true, a.length * sizeof(bool));
    memset(b_leftover, true, b.length * sizeof(bool));
    size_t length = a.length + b.length;
    for (size_t i = 0; i < b.length; i++) {
        UnitType b_type = b.types[i];
        for (size_t j = 0; j < a.length; j++) {
            UnitType a_type = a.types[j];
            UnitCategory a_cat = unit_category(a_type);
            UnitCategory b_cat = unit_category(b_type);
            if (a_cat == b_cat && a_type != b_type && reject_same_category) {
                return unit_new_unknown(arena);
            } else if (a_cat == b_cat && a.degrees[j] == -b.degrees[i]) {
                a_leftover[j] = false;
                b_leftover[i] = false;
                length -= 2;
            } else if (a_cat == b_cat) {
                debug("combining convertible units' degrees: %d %d\n", a.degrees[j], b.degrees[i]);
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
    if (curr_length == 0) {
        return unit_new_none(arena);
    }
    return (Unit) { .types = types, .degrees = degrees, .length = length };
}
