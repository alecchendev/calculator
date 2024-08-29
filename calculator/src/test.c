
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "arena.c"
#include "evaluate.c"
#include "parse.c"
#include "tokenize.c"
#include "debug.c"

// Only global variable in this file. So
// we don't have to pipe an extra bool through
// our test case runners.
static bool run_with_fork = true;

bool run_test_case(size_t case_num, void (*test)(void *), void *c_opaque,
                    const char *pass_str, const char *fail_str) {
    if (!run_with_fork) {
        test(c_opaque);
        if (pass_str != NULL) printf(pass_str, case_num);
        return true;
    }

    pid_t pid = fork();
    assert(pid != -1);
    if (pid == 0) {
        test(c_opaque);
        exit(0);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            if (pass_str != NULL) printf(pass_str, case_num);
            return true;
        } else if (WIFSIGNALED(status)) {
            if (fail_str != NULL) printf(fail_str, case_num);
            return false;
        } else {
            printf("Child process exited abnormally\n");
            return false;
        }
    }
}

typedef struct {
    size_t start;
    size_t end;
} CaseBound;

CaseBound case_bound(ssize_t case_idx, size_t n_cases) {
    assert(case_idx == -1 || (case_idx >= 0 && case_idx < n_cases));
    size_t start = case_idx != -1 ? case_idx : 0;
    size_t end = case_idx != -1 ? case_idx + 1 : n_cases;
    return (CaseBound) { .start = start, .end = end };
}

typedef struct TokenCase TokenCase;
struct TokenCase {
    const char *input;
    const size_t length;
    const Token expected[MAX_INPUT];
};

bool eq_diff(double a, double b) {
    double diff = a - b;
    double abs_diff = diff < 0 ? -diff : diff;
    return abs_diff < 0.00001;
}

bool tokens_equal(Token a, Token b) {
    if (a.type != b.type) {
        printf("Expected type %d, got %d\n", b.type, a.type);
        return false;
    }
    if (a.type == TOK_WHITESPACE || a.type == TOK_INVALID || a.type == TOK_END || a.type == TOK_QUIT) {
        return true;
    }
    if (a.type == TOK_UNIT && a.unit_type != b.unit_type) {
        printf("Expected unit type %s, got %s\n", unit_strings[b.unit_type], unit_strings[a.unit_type]);
        return false;
    }
    if (a.type == TOK_NUM && !eq_diff(a.number, b.number)) {
        printf("Expected number %f, got %f\n", b.number, a.number);
        return false;
    }
    return true;
}

void test_tokenize_case(void *c_opaque) {
    TokenCase c = *(TokenCase *)c_opaque;
    Arena arena = arena_create();
    TokenString tokens = tokenize(c.input, &arena);
    for (size_t i = 0; i < tokens.length; i++) {
        token_display(tokens.tokens[i]);
    }
    assert_eq(tokens.length, c.length);
    for (size_t i = 0; i < tokens.length; i++) {
        assert(tokens_equal(tokens.tokens[i], c.expected[i]));
    }
    arena_free(&arena);
}

void test_tokenize(void *case_idx_opaque) {
    char max_len_input[MAX_INPUT + 1] = {0};
    memset(max_len_input, 'x', MAX_INPUT + 1);
    TokenCase cases[] = {
        {"", 1, {end_token}},
        {"1", 2, {token_new_num(1), end_token}},
        {"1 + 2", 4, {token_new_num(1), add_token, token_new_num(2), end_token}},
        {"\t 1\t+     2  ", 4, {token_new_num(1), add_token, token_new_num(2), end_token}},
        {"1 - 2 * 3 / 4", 8, {
            token_new_num(1), sub_token, token_new_num(2), mul_token,
            token_new_num(3), div_token, token_new_num(4), end_token
        }},
        {"45.874", 2, {token_new_num(45.874), end_token}},
        {"2e3", 2, {token_new_num(2000), end_token}},
        {"3.32e2", 2, {token_new_num(332), end_token}},
        {"3.32E2", 2, {token_new_num(332), end_token}},
        {"3.32e-2", 1, {invalid_token}}, // TODO: handle negatives generally + in scientific notation?
        {"asdf", 1, {invalid_token}},
        {"quit", 1, {quit_token}},
        {"exit", 1, {quit_token}},
        {"quitX", 1, {invalid_token}}, // TODO: this should be invalid
        {max_len_input, 1, {invalid_token}},
        // Units
        {"3 cm + 5.5min", 6, {
            token_new_num(3), token_new_unit(UNIT_CENTIMETER), add_token,
            token_new_num(5.5), token_new_unit(UNIT_MINUTE), end_token
        }},
        {"1 mi / h", 5, {
            token_new_num(1), token_new_unit(UNIT_MILE), div_token,
            token_new_unit(UNIT_HOUR), end_token
        }},
        {"->", 2, {convert_token, end_token}},
        {"4.5 - 3 km -> mi", 7, {token_new_num(4.5), sub_token, token_new_num(3),
            token_new_unit(UNIT_KILOMETER), convert_token, token_new_unit(UNIT_MILE),
            end_token}},
        {"50km^2", 5, {token_new_num(50), token_new_unit(UNIT_KILOMETER), caret_token, token_new_num(2), end_token}},
        {"50 km ^ 2", 5, {token_new_num(50), token_new_unit(UNIT_KILOMETER), caret_token, token_new_num(2), end_token}},
        {"- 50 kg^2km ^-2", 10, {
            sub_token, token_new_num(50), token_new_unit(UNIT_KILOGRAM), caret_token, token_new_num(2),
            token_new_unit(UNIT_KILOMETER), caret_token, sub_token, token_new_num(2), end_token
        }},
        // TODO: more comprehensive
    };
    const size_t num_cases = sizeof(cases) / sizeof(TokenCase);
    bool all_passed = true;
    ssize_t case_idx = *(ssize_t *)case_idx_opaque;
    CaseBound bound = case_bound(case_idx, num_cases);
    for (size_t i = bound.start; i < bound.end; i++) {
        all_passed &= run_test_case(i, test_tokenize_case, (void *)&cases[i],
                                     NULL, "Case %zu failed\n");
    }
    assert(all_passed);
}

typedef struct {
    const char *input;
    const Expression expected;
} ParseCase;

bool exprs_equal(Expression a, Expression b, Arena *arena) {
    if (a.type != b.type) {
        printf("Expected type %d, got %d\n", b.type, a.type);
        return false;
    }
    switch (a.type) {
        case EXPR_CONSTANT:
            if (a.expr.constant - b.expr.constant > 0.0000001) {
                printf("Expected number %f, got %f\n", b.expr.constant, a.expr.constant);
                return false;
            }
            return true;
        case EXPR_UNIT:
            if (a.type == EXPR_UNIT && a.expr.unit_type != b.expr.unit_type) {
                printf("Expected unit %s, got %s\n", unit_strings[b.expr.unit_type], unit_strings[a.expr.unit_type]);
                return false;
            }
            return true;
        case EXPR_NEG:
            return exprs_equal(*a.expr.unary_expr.right, *b.expr.unary_expr.right, arena);
        case EXPR_CONST_UNIT: case EXPR_COMP_UNIT:
        case EXPR_ADD: case EXPR_SUB: case EXPR_MUL: case EXPR_DIV:
        case EXPR_CONVERT: case EXPR_POW: case EXPR_DIV_UNIT:
            if (!exprs_equal(*a.expr.binary_expr.left, *b.expr.binary_expr.left, arena)) {
                return false;
            }
            if (!exprs_equal(*a.expr.binary_expr.right, *b.expr.binary_expr.right, arena)) {
                return false;
            }
        case EXPR_EMPTY: case EXPR_QUIT: case EXPR_HELP: case EXPR_INVALID:
            return true;
    }
}

void test_parse_case(void *c_opaque) {
    ParseCase *c = (ParseCase *)c_opaque;
    Arena arena = arena_create();
    TokenString tokens = tokenize(c->input, &arena);
    Expression expr = parse(tokens, &arena);
    debug("input: %s\n", c->input);
    debug("Expected:\n");
    display_expr(0, c->expected, &arena);
    debug("Got:\n");
    display_expr(0, expr, &arena);
    assert(exprs_equal(expr, c->expected, &arena));
    ErrorString err = err_empty();
    assert(check_valid_expr(expr, &err, &arena));
    arena_free(&arena);
}

void test_parse(void *case_idx_opaque) {
    Arena case_arena = arena_create();
    const ParseCase cases[] = {
        {"1 + 2 * 3", expr_new_bin(EXPR_ADD,
            expr_new_const(1),
            expr_new_bin(EXPR_MUL,
                expr_new_const(2),
                expr_new_const(3),
            &case_arena),
        &case_arena)},
        {"1 cm -2kg", expr_new_bin(EXPR_SUB,
            expr_new_const_unit(1, expr_new_unit(UNIT_CENTIMETER), &case_arena),
            expr_new_const_unit(2, expr_new_unit(UNIT_KILOGRAM), &case_arena),
        &case_arena)},
        {"5 mi / 4 h", expr_new_bin(EXPR_DIV,
            expr_new_const_unit(5, expr_new_unit(UNIT_MILE), &case_arena),
            expr_new_const_unit(4, expr_new_unit(UNIT_HOUR), &case_arena),
        &case_arena)},
        // Negative
        {"2 * - 3", expr_new_bin(EXPR_MUL, expr_new_const(2),
            expr_new_neg(expr_new_const(3), &case_arena), &case_arena)},
        {"-2 * 3", expr_new_bin(EXPR_MUL, expr_new_neg(expr_new_const(2), &case_arena),
            expr_new_const(3), &case_arena)},
        {"-2 cm * 3", expr_new_bin(EXPR_MUL,
            expr_new_neg(expr_new_const_unit(2, expr_new_unit(UNIT_CENTIMETER), &case_arena), &case_arena),
            expr_new_const(3), &case_arena)},
        {"- 2", expr_new_neg(expr_new_const(2), &case_arena)},
        {"1 - - 2", expr_new_bin(EXPR_SUB, expr_new_const(1),
            expr_new_neg(expr_new_const(2), &case_arena), &case_arena)},
        {"1 - - - 2", expr_new_bin(EXPR_SUB, expr_new_const(1),
            expr_new_neg(expr_new_neg(expr_new_const(2), &case_arena),
                &case_arena), &case_arena)},
        {"1 - - - - 2", expr_new_bin(EXPR_SUB, expr_new_const(1),
            expr_new_neg(expr_new_neg(expr_new_neg(expr_new_const(2),
                &case_arena), &case_arena), &case_arena), &case_arena)},

        // Units with degrees
        {"50 km^2", expr_new_const_unit(50, expr_new_unit_degree(UNIT_KILOMETER, expr_new_const(2), &case_arena), &case_arena)},
        {"50 km ^ 2", expr_new_const_unit(50, expr_new_unit_degree(UNIT_KILOMETER, expr_new_const(2), &case_arena), &case_arena)},
        {"50 km^-2", expr_new_const_unit(50, expr_new_unit_degree(UNIT_KILOMETER, expr_new_neg(expr_new_const(2), &case_arena), &case_arena), &case_arena)},
        {"50 km^ - 2", expr_new_const_unit(50, expr_new_unit_degree(UNIT_KILOMETER, expr_new_neg(expr_new_const(2), &case_arena), &case_arena), &case_arena)},
        {"- 50 km ^ -2", expr_new_neg(expr_new_const_unit(50, expr_new_unit_degree(UNIT_KILOMETER, expr_new_neg(expr_new_const(2), &case_arena), &case_arena), &case_arena), &case_arena)},
        {"- 50 km ^ -2 - 3", expr_new_bin(EXPR_SUB,
            expr_new_neg(expr_new_const_unit(
                50, expr_new_unit_degree(UNIT_KILOMETER, expr_new_neg(expr_new_const(2), &case_arena), &case_arena),
            &case_arena), &case_arena),
            expr_new_const(3), &case_arena)},

        // Composite units
        {"- 50 kg^2km ^-2", expr_new_neg(expr_new_const_unit(50,
            expr_new_unit_comp(
                expr_new_unit_degree(UNIT_KILOGRAM, expr_new_const(2), &case_arena),
                expr_new_unit_degree(UNIT_KILOMETER, expr_new_neg(expr_new_const(2), &case_arena), &case_arena),
            &case_arena),
        &case_arena), &case_arena)},
        {"- 50 kg^2km ^-2 - 3", expr_new_bin(EXPR_SUB, expr_new_neg(expr_new_const_unit(50,
            expr_new_unit_comp(
                expr_new_unit_degree(UNIT_KILOGRAM, expr_new_const(2), &case_arena),
                expr_new_unit_degree(UNIT_KILOMETER, expr_new_neg(expr_new_const(2), &case_arena), &case_arena),
            &case_arena),
        &case_arena), &case_arena), expr_new_const(3), &case_arena)},
        {"--1 s^-2 km ^3 cm^4 oz ^--5 * ---6lb---7oz^-8", expr_new_bin(EXPR_SUB,
            expr_new_bin(EXPR_MUL,
                expr_new_neg(expr_new_neg(expr_new_const_unit(1, 
                    expr_new_unit_comp(
                        expr_new_unit_comp(
                            expr_new_unit_comp(
                                expr_new_unit_degree(UNIT_SECOND, expr_new_neg(expr_new_const(2), &case_arena), &case_arena),
                                expr_new_unit_degree(UNIT_KILOMETER, expr_new_const(3), &case_arena),
                            &case_arena),
                            expr_new_unit_degree(UNIT_CENTIMETER, expr_new_const(4), &case_arena),
                        &case_arena),
                        expr_new_unit_degree(UNIT_OUNCE, expr_new_neg(expr_new_neg(expr_new_const(5), &case_arena), &case_arena), &case_arena),
                    &case_arena),
                &case_arena), &case_arena), &case_arena),
                expr_new_neg(expr_new_neg(expr_new_neg(expr_new_const_unit(6, expr_new_unit(UNIT_POUND), &case_arena), &case_arena), &case_arena), &case_arena),
            &case_arena),
            expr_new_neg(expr_new_neg(expr_new_const_unit(7, expr_new_unit_degree(UNIT_OUNCE, expr_new_neg(expr_new_const(8), &case_arena), &case_arena), &case_arena), &case_arena), &case_arena),
        &case_arena)}
        /*{"1 + 2km * 3 h / 2 km ^-2"}*/

        // "5 s^-2 km^3 cm^4 oz^5 lb * 2 s^2 / 3 km ^3"
    };
    const size_t num_cases = sizeof(cases) / sizeof(ParseCase);
    bool all_passed = true;
    ssize_t case_idx = *(ssize_t *)case_idx_opaque;
    CaseBound bound = case_bound(case_idx, num_cases);
    for (size_t i = bound.start; i < bound.end; i++) {
        all_passed &= run_test_case(i, test_parse_case, (void *)&cases[i],
                                     NULL, "Case %zu failed\n");
    }
    assert(all_passed);
    arena_free(&case_arena);
}

typedef struct {
    const char *input;
} InvalidExprCase;

void test_invalid_expr_case(void *c_opaque) {
    InvalidExprCase *c = (InvalidExprCase *)c_opaque;
    Arena arena = arena_create();
    TokenString tokens = tokenize(c->input, &arena);
    Expression expr = parse(tokens, &arena);
    ErrorString err = err_empty();
    assert(!check_valid_expr(expr, &err, &arena));
    arena_free(&arena);
}

void test_invalid_expr(void *case_idx_opaque) {
    Arena case_arena = arena_create();
    InvalidExprCase cases[] = {
        {"1 +"},
        {"1 -> "},
        {"- +"},
        {"* / ^"},
        {" -> km"},
        {"1 + asdf"},
        {"1 asdf 2"},
        {"1 km ^ asdf 2 km"},
        {"asdf"},
        {"asdf asdf"},
        {"asdf asdf asdf"},
        {"asdf asdf asdf asdf"},
    };
    const size_t num_cases = sizeof(cases) / sizeof(InvalidExprCase);
    bool all_passed = true;
    for (size_t i = 0; i < num_cases; i++) {
        all_passed &= run_test_case(i, test_invalid_expr_case, (void *)&cases[i],
                                     NULL, "Case %zu failed\n");
    }
    assert(all_passed);
    arena_free(&case_arena);
}


typedef struct {
    const char *input;
    const Unit expected;
} CheckUnitCase;

void test_check_unit_case(void *c_opaque) {
    CheckUnitCase *c = (CheckUnitCase *)c_opaque;
    Arena arena = arena_create();
    TokenString tokens = tokenize(c->input, &arena);
    Expression expr = parse(tokens, &arena);
    display_expr(0, expr, &arena);
    ErrorString err = err_empty();
    assert(check_valid_expr(expr, &err, &arena));
    Unit unit = check_unit(expr, &arena);
    assert(units_equal(unit, c->expected, &arena));
    arena_free(&arena);
}

void test_check_unit(void *case_idx_opaque) {
    Arena case_arena = arena_create();
    /*UnitT*/
    const CheckUnitCase cases[] = {
        {"79", unit_new_none(&case_arena)},
        {"1 + 2 * 3", unit_new_none(&case_arena)},
        {"1 km * 2 km * 3km", unit_new_single(UNIT_KILOMETER, 3, &case_arena)},
        {"1 mi + 1h", unit_new_unknown(&case_arena)},
        {"1 km * 2 oz * 3 h", unit_new((UnitType[]){UNIT_KILOMETER, UNIT_OUNCE, UNIT_HOUR}, (int[]){1, 1, 1}, 3, &case_arena)},
        {"1km*2mi*3h*4km*5mi*2s", unit_new((UnitType[]){UNIT_KILOMETER, UNIT_HOUR}, (int[]){4, 2}, 2, &case_arena)},
        {"1km*2mi*3h*4km*5mi*2s+3km", unit_new_unknown(&case_arena)},
        {"1km*2mi/4km", unit_new((UnitType[]){UNIT_KILOMETER}, (int[]){1}, 1, &case_arena)},
        {"50 km ^ -2 ^ 3", unit_new_single(UNIT_KILOMETER, -6, &case_arena)},
        {"50 km ^ -2 km", unit_new_single(UNIT_KILOMETER, -1, &case_arena)},
        {"1 km * 2 km ^-1", unit_new_none(&case_arena)},
        {"1 kg * 2 g ^-1", unit_new_none(&case_arena)},
        {"2 km m cm", unit_new_unknown(&case_arena)},
        {"50 km s^-1 + 50 s^-1 km", unit_new((UnitType[]){UNIT_KILOMETER, UNIT_SECOND},
            (int[]){1, -1}, 2, &case_arena)},
        {"50 s^-1 km + 50 km s^-1", unit_new((UnitType[]){UNIT_SECOND, UNIT_KILOMETER},
            (int[]){-1, 1}, 2, &case_arena)},
        {"1km -> mi", unit_new_single(UNIT_MILE, 1, &case_arena)},
        {"1km -> s", unit_new_unknown(&case_arena)},
        {"1kg -> h", unit_new_unknown(&case_arena)},
        {"1 lb -> in", unit_new_unknown(&case_arena)},
        {"2 m^2 -> cm^1", unit_new_unknown(&case_arena)},
        {"2 km s^-1 -> kg h^-1", unit_new_unknown(&case_arena)},
        {"1 kg * 2 kg ^ -3 km", unit_new((UnitType[]){UNIT_KILOGRAM, UNIT_KILOMETER}, (int[]){-2, 1}, 2, &case_arena)},
        {"1 km + 1", unit_new_unknown(&case_arena)},
        {"1 -> km", unit_new_unknown(&case_arena)},
        {"1 m / s", unit_new((UnitType[]){UNIT_METER, UNIT_SECOND}, (int[]){1, -1}, 2, &case_arena)},
        {"1 m^2 / s^2 kg^2", unit_new((UnitType[]){UNIT_METER, UNIT_SECOND, UNIT_KILOGRAM},
            (int[]){2, -2, -2}, 3, &case_arena)},
        {"1 m^2 s^3 / kg^2", unit_new((UnitType[]){UNIT_METER, UNIT_SECOND, UNIT_KILOGRAM},
            (int[]){2, 3, -2}, 3, &case_arena)}
    };
    const size_t num_cases = sizeof(cases) / sizeof(CheckUnitCase);
    bool all_passed = true;
    ssize_t case_idx = *(ssize_t *)case_idx_opaque;
    CaseBound bound = case_bound(case_idx, num_cases);
    for (size_t i = bound.start; i < bound.end; i++) {
        all_passed &= run_test_case(i, test_check_unit_case, (void *)&cases[i],
                                     NULL, "Case %zu failed\n");
    }
    assert(all_passed);
    arena_free(&case_arena);
}

typedef struct {
    const char *input;
    const double expected;
} EvaluateCase;

void test_evaluate_case(void *c_opaque) {
    EvaluateCase *c = (EvaluateCase *)c_opaque;
    Arena arena = arena_create();
    TokenString tokens = tokenize(c->input, &arena);
    Expression expr = parse(tokens, &arena);
    ErrorString err = err_empty();
    assert(check_valid_expr(expr, &err, &arena));
    assert(!is_unit_unknown(check_unit(expr, &arena)));
    double result = evaluate(expr, &arena);
    debug("Expected: %f, got: %f\n", c->expected, result);
    assert(eq_diff(result, c->expected));
    arena_free(&arena);
}

void test_evaluate(void *case_idx_opaque) {
    const EvaluateCase cases[] = {
        // Basic ops
        {"42", 42},
        {"9 + 10", 19},
        {"20 - 30", -10},
        {"21 * 2", 42},
        {"1 / 2", 0.5},
        // Order of operations
        {"1 + 2 * 3", 7},
        {"2 / 4 + 1", 1.5},
        {" 3000  - 600 * 20 / 2.5 +\t20", -1780},
        // Units
        {"2 cm * 3 + 1.5cm", 7.5},
        {"km ^ 2 h lb^-3", 0},
        {"1 m / s + 2 km / h", 1 + 2.0 * 1000 / 3600},
        // Conversions
        {"1 km * 3 -> in", 118110.236100},
        {"2 s + 3 h - 6 min -> min", (2.0 + 3 * 3600 - 6 * 60) / 60},
        {"2 s + 3 h^2 / 6 min -> min", (2 + 3.0 / (6.0 / 60) * 3600) / 60},
        {"6 min / 2 min * 3 s -> s", 6.0 / 2 * (3.0 / 60) * 60},
        {"6 min * 2 min * 3 s -> s^3", 6.0 * 2 * (3.0 / 60) * 60 * 60 * 60},
        // Compositve conversions
        {"2 m^2 -> cm^2", 20000},
        {"2 km s^-1 -> m h^-1", 7200000},
        {"2 min^2 km -> m h^2", 2.0*1000/60/60},
        // TODO: overflow tests
    };
    const size_t num_cases = sizeof(cases) / sizeof(EvaluateCase);
    bool all_passed = true;
    ssize_t case_idx = *(ssize_t *)case_idx_opaque;
    CaseBound bound = case_bound(case_idx, num_cases);
    for (size_t i = bound.start; i < bound.end; i++) {
        all_passed &= run_test_case(i, test_evaluate_case, (void *)&cases[i],
                                     NULL, "Case %zu failed\n");
    }
    assert(all_passed);
}

void test_unit_mirror(void *_) {
    for (UnitType unit_type1 = 0; unit_type1 < UNIT_COUNT; unit_type1++) {
        for (UnitType unit_type2 = unit_type1; unit_type2 < UNIT_COUNT; unit_type2++) {
            double one_to_two = unit_conversion[unit_type1][unit_type2];
            double two_to_one = unit_conversion[unit_type2][unit_type1];
            if (one_to_two == 0) {
                assert(two_to_one == 0);
            } else {
                debug("1: %s 2: %s 1 / one_to_two: %lf two_to_one: %lf\n", unit_strings[unit_type1], unit_strings[unit_type2], 1 / one_to_two, two_to_one);
                assert(eq_diff(1 / one_to_two, two_to_one));
            }
        }
    }
}

typedef struct {
    const Unit unit;
    const char *expected;
} DisplayUnitCase;

void test_display_unit(void *case_idx_opaque) {
    Arena arena = arena_create();
    DisplayUnitCase cases[] = {
        {unit_new_none(&arena), "none"},
        {unit_new_single(UNIT_MINUTE, 1, &arena), "min"},
        {unit_new_single(UNIT_CENTIMETER, 2, &arena), "cm^2"},
        {unit_new_single(UNIT_KILOGRAM, -2, &arena), "kg^-2"},
        {unit_new_unknown(&arena), "unknown"},
    };
    const size_t num_cases = sizeof(cases) / sizeof(DisplayUnitCase);
    ssize_t case_idx = *(ssize_t *)case_idx_opaque;
    CaseBound bound = case_bound(case_idx, num_cases);
    for (size_t i = bound.start; i < bound.end; i++) {
        char *displayed = display_unit(cases[i].unit, &arena);
        debug("Expected: %s got: %s\n", cases[i].expected, displayed);
        assert(strncmp(displayed, cases[i].expected, MAX_COMPOSITE_UNIT_STRING) == 0);
    }
    arena_free(&arena);
}

int main(int argc, char **argv) {
    ssize_t single_test_idx = -1;
    ssize_t single_case_idx = -1;
    if (argc > 1) {
        for (size_t i = 1; i < argc; i++) {
            int value = -1;
            if (sscanf(argv[i], "fork=%d", &value) == 1) {
                assert(run_with_fork == 1);
                run_with_fork = value == 1;
            } else if (sscanf(argv[i], "test=%d", &value) == 1) {
                assert(single_test_idx == -1);
                single_test_idx = value;
            } else if (sscanf(argv[i], "case=%d", &value) == 1) {
                assert(single_case_idx == -1);
                single_case_idx = value;
            }
        }
    }
    void (*tests[])(void *) = {
        test_tokenize,
        test_parse,
        test_invalid_expr,
        test_check_unit,
        test_evaluate,
        test_unit_mirror,
        test_display_unit,
    };
    const size_t n_tests = sizeof(tests) / sizeof(tests[0]);
    bool all_passed = true;
    CaseBound bound = case_bound(single_test_idx, n_tests);
    for (size_t i = bound.start; i < bound.end; i++) {
        all_passed &= run_test_case(i, tests[i], (void *)&single_case_idx, "Test %zu passed\n", "Test %zu failed\n");
    }
    if (all_passed) {
        printf("All tests passed\n");
    } else {
        printf("Some tests failed\n");
    }
}
