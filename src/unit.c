#pragma once

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "arena.c"
#include "debug.c"
#include "string.c"

// A number representing the unit type.
// User defined units will be numbers greater than
// UNIT_UNKNOWN.
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
    // Temperature
    UNIT_KELVIN,
    UNIT_CELSIUS,
    UNIT_FAHRENHEIT,

    UNIT_COUNT,
    UNIT_NONE,
    UNIT_UNKNOWN,
};

// The minimum value for a user defined unit type.
int unit_type_user_min() {
    return UNIT_UNKNOWN + 1;
}

#define MAX_UNIT_STRING 7

const char *builtin_unit_strings[] = {
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
    // Temperature
    "K",
    "C",
    "F",

    "",
    "none",
    "unknown",
};

typedef struct UnitBasic UnitBasic;
struct UnitBasic {
    UnitType type;
    char *name;
};

UnitBasic unit_basic(UnitType type, char *name) {
    return (UnitBasic) { .type = type, .name = name };
}

UnitBasic unit_basic_builtin(UnitType type) {
    assert(type < unit_type_user_min());
    return unit_basic(type, (char *)builtin_unit_strings[type]);
}

bool string_in_set(char *s, char *set[], size_t set_len) {
    size_t len = strnlen(s, 32);
    for (size_t i = 0; i < set_len; i++) {
        char *curr = set[i];
        size_t curr_len = strnlen(curr, 32);
        if (curr_len != len) continue;
        if (strncmp(s, curr, len) == 0) return true;
    }
    return false;
}

UnitType string_to_unit(char *s) {
    char *cms[] = {"cm", "centimeter", "centimeters"};
    char *ms[] = {"m", "meter", "meters"};
    char *kms[] = {"km", "kilometer", "kilometers"};
    char *ins[] = {"in", "inch", "inches"};
    char *fts[] = {"ft", "foot", "feet"};
    char *mis[] = {"mi", "mile", "miles"};
    char *secs[] = {"s", "sec", "secs", "second", "seconds"};
    char *mins[] = {"min", "mins", "minute", "minutes"};
    char *hrs[] = {"h", "hr", "hrs", "hour", "hours"};
    char *gs[] = {"g", "gram", "grams"};
    char *kgs[] = {"kg", "kilogram", "kilograms"};
    char *lbs[] = {"lb", "lbs", "pound", "pounds"};
    char *ozs[] = {"oz", "ozs", "ounce", "ounces"};
    char *ks[] = {"K", "k", "kelvin"};
    char *cs[] = {"C", "c", "celsius"};
    char *fs[] = {"F", "f", "fahrenheit"};
    if (string_in_set(s, cms, 3)) return UNIT_CENTIMETER;
    if (string_in_set(s, ms, 3)) return UNIT_METER;
    if (string_in_set(s, kms, 3)) return UNIT_KILOMETER;
    if (string_in_set(s, ins, 3)) return UNIT_INCH;
    if (string_in_set(s, fts, 3)) return UNIT_FOOT;
    if (string_in_set(s, mis, 3)) return UNIT_MILE;
    if (string_in_set(s, secs, 5)) return UNIT_SECOND;
    if (string_in_set(s, mins, 4)) return UNIT_MINUTE;
    if (string_in_set(s, hrs, 5)) return UNIT_HOUR;
    if (string_in_set(s, gs, 3)) return UNIT_GRAM;
    if (string_in_set(s, kgs, 3)) return UNIT_KILOGRAM;
    if (string_in_set(s, lbs, 4)) return UNIT_POUND;
    if (string_in_set(s, ozs, 4)) return UNIT_OUNCE;
    if (string_in_set(s, ks, 3)) return UNIT_KELVIN;
    if (string_in_set(s, cs, 3)) return UNIT_CELSIUS;
    if (string_in_set(s, fs, 3)) return UNIT_FAHRENHEIT;
    return UNIT_UNKNOWN;
}

typedef enum UnitCategory UnitCategory;
enum UnitCategory {
    UNIT_CATEGORY_DISTANCE,
    UNIT_CATEGORY_TIME,
    UNIT_CATEGORY_MASS,
    UNIT_CATEGORY_TEMPERATURE,
    UNIT_CATEGORY_NONE,
};

const char *unit_category_strings[] = {
    "Distance",
    "Time",
    "Mass",
    "Temperature",
    "none",
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
        case UNIT_KELVIN:
        case UNIT_CELSIUS:
        case UNIT_FAHRENHEIT:
            return UNIT_CATEGORY_TEMPERATURE;
        case UNIT_NONE:
        case UNIT_COUNT:
        case UNIT_UNKNOWN:
            return UNIT_CATEGORY_NONE;
        default:
            // User defined units simply have their own category for now
            // TODO: allow user defined units to share categories
            // when we add user defined conversions
            return (UnitCategory)type;
    }
}

String show_all_builtin_units(Arena *arena) {
    String s = string_empty(arena);
    for (UnitCategory cat = 0; cat < UNIT_CATEGORY_NONE; cat++) {
        char *cat_str = (char *)unit_category_strings[cat];
        s = string_concat_static(s, cat_str, arena);
        s = string_concat_static(s, ": ", arena);
        bool first = true;
        for (UnitType typ = 0; typ < UNIT_COUNT; typ++) {
            if (unit_category(typ) == cat) {
                if (first) {
                    first = false;
                } else {
                    s = string_concat_static(s, ", ", arena);
                }
                char *unit_str = (char *)builtin_unit_strings[typ];
                s = string_concat_static(s, unit_str, arena);
            }
        }
        if (cat < UNIT_CATEGORY_NONE - 1) {
            s = string_concat_static(s, "\n", arena);
        }
    }
    return s;
}

// y = mx + b
// x = (y - b) / m
typedef struct SlopeIntercept SlopeIntercept;
struct SlopeIntercept {
    double m;
    double b;
};

SlopeIntercept mb_new(double m, double b) {
    return (SlopeIntercept) { .m = m, .b = b };
}

double solve_y(SlopeIntercept mb, double x) {
    return mb.m * x + mb.b;
}

double solve_x(SlopeIntercept mb, double y) {
    return (y - mb.b) / mb.m;
}

SlopeIntercept to_meters2(UnitType from) {
    switch (from) {
        case UNIT_CENTIMETER: return mb_new(0.01, 0);
        case UNIT_METER: return mb_new(1, 0);
        case UNIT_KILOMETER: return mb_new(1000, 0);
        case UNIT_INCH: return mb_new(0.0254, 0);
        case UNIT_FOOT: return mb_new(0.3048, 0);
        case UNIT_MILE: return mb_new(1609.344, 0);
        default:
            assert(false);
            return mb_new(0, 0);
    }
}

SlopeIntercept to_seconds2(UnitType from) {
    switch (from) {
        case UNIT_SECOND: return mb_new(1, 0);
        case UNIT_MINUTE: return mb_new(60, 0);
        case UNIT_HOUR: return mb_new(3600, 0);
        default:
            assert(false);
            return mb_new(0, 0);
    }
}

SlopeIntercept to_kilograms2(UnitType from) {
    switch (from) {
        case UNIT_GRAM: return mb_new(0.001, 0);
        case UNIT_KILOGRAM: return mb_new(1, 0);
        case UNIT_OUNCE: return mb_new(0.0283495231, 0);
        case UNIT_POUND: return mb_new(0.45359237, 0);
        default:
            assert(false);
            return mb_new(0, 0);
    }
}

SlopeIntercept to_kelvin(UnitType from) {
    switch (from) {
        case UNIT_KELVIN: return mb_new(1, 0);
        case UNIT_CELSIUS: return mb_new(1, 273.15);
        case UNIT_FAHRENHEIT:
            return mb_new(5.0 / 9.0, 459.67 * 5.0 / 9.0);
        default:
            assert(false);
            return mb_new(0, 0);
    }
}

// TODO: think more about precision, maybe rewrite some things
// as expressions so the compiler can work some magic
double unit_conversion(double value, UnitType from, UnitType to) {
    UnitCategory cat_from = unit_category(from);
    UnitCategory cat_to = unit_category(to);
    if (cat_from != cat_to) return 0;
    double meters, kilograms, seconds, kelvin;
    switch (cat_from) {
        case UNIT_CATEGORY_DISTANCE:
            meters = solve_y(to_meters2(from), value);
            return solve_x(to_meters2(to), meters);
        case UNIT_CATEGORY_MASS:
            kilograms = solve_y(to_kilograms2(from), value);
            return solve_x(to_kilograms2(to), kilograms);
        case UNIT_CATEGORY_TIME:
            seconds = solve_y(to_seconds2(from), value);
            return solve_x(to_seconds2(to), seconds);
        case UNIT_CATEGORY_TEMPERATURE:
            kelvin = solve_y(to_kelvin(from), value);
            return solve_x(to_kelvin(to), kelvin);
        case UNIT_CATEGORY_NONE:
            return value;
        default:
            // Assume user defined units are their own category.
            return value;
    }
}

typedef struct Unit Unit;
struct Unit {
    UnitBasic *types;
    int *degrees;
    size_t length;
};

bool is_unit_none(Unit unit) {
    return unit.length == 1 && unit.types[0].type == UNIT_NONE;
}

bool is_unit_unknown(Unit unit) {
    return unit.length == 1 && unit.types[0].type == UNIT_UNKNOWN;
}

Unit unit_new_builtin(UnitType types[], int degrees[], size_t length, Arena *arena) {
    UnitBasic *new_types = arena_alloc(arena, length * sizeof(UnitBasic));
    int *new_degrees = arena_alloc(arena, length * sizeof(int));
    for (size_t i = 0; i < length; i++) {
        new_types[i] = unit_basic_builtin(types[i]);
    }
    memcpy(new_degrees, degrees, length * sizeof(int));
    return (Unit) { .types = new_types, .degrees = new_degrees, .length = length };
}

Unit unit_new(UnitBasic types[], int degrees[], size_t length, Arena *arena) {
    UnitBasic *new_types = arena_alloc(arena, length * sizeof(UnitBasic));
    int *new_degrees = arena_alloc(arena, length * sizeof(int));
    memcpy(new_types, types, length * sizeof(UnitBasic));
    memcpy(new_degrees, degrees, length * sizeof(int));
    return (Unit) { .types = new_types, .degrees = new_degrees, .length = length };
}

Unit unit_new_single(UnitBasic basic, int degree, Arena *arena) {
    return unit_new(&basic, &degree, 1, arena);
}

Unit unit_new_single_builtin(UnitType type, int degree, Arena *arena) {
    return unit_new_builtin(&type, &degree, 1, arena);
}

Unit unit_new_none(Arena *arena) {
    return unit_new_single_builtin(UNIT_NONE, 1, arena);
}

Unit unit_new_unknown(Arena *arena) {
    return unit_new_single_builtin(UNIT_UNKNOWN, 0, arena);
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
            snprintf(unit_str, sizeof(unit_str), "%s", unit.types[i].name);
        } else {
            snprintf(unit_str, sizeof(unit_str), "%s^%d", unit.types[i].name, unit.degrees[i]);
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
            if (a.types[i].type == b.types[j].type && a.degrees[i] == b.degrees[j]) {
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

double unit_convert(double value, Unit a, Unit b, Arena *arena) {
    // Should only be called if we are able to convert
    debug("a: %s b: %s a.length: %zu b.length: %zu\n",
          display_unit(a, arena), display_unit(b, arena), a.length, b.length);
    for (size_t i = 0; i < a.length; i++) {
        for (size_t j = 0; j < b.length; j++) {
            if (unit_category(a.types[i].type) == unit_category(b.types[j].type)) {
                double new_value = value;
                double value_degree_1 = pow(value, 1.0 / a.degrees[i]);
                double converted_degree_1 = unit_conversion(value_degree_1, a.types[i].type, b.types[j].type);
                new_value = pow(converted_degree_1, a.degrees[i]);
                debug("Found convertible: left: %s right: %s degree: %d pre-value: %lf post-value: %lf\n", a.types[i].name, b.types[j].name, a.degrees[i], value, new_value);
                value = new_value;
                break;
            }
        }
    }
    return value;
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
        UnitType b_type = b.types[i].type;
        for (size_t j = 0; j < a.length; j++) {
            UnitType a_type = a.types[j].type;
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
    UnitBasic *types = arena_alloc(arena, length * sizeof(UnitBasic));
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
