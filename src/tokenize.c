#pragma once

#include <math.h>
#include <stdbool.h>
#include <string.h>
#include "arena.c"
#include "unit.c"
#include "debug.c"
#include "string.c"

typedef enum TokenType TokenType;
enum TokenType {
    TOK_NUM,
    TOK_UNIT,
    TOK_VAR,
    TOK_EQUALS,
    TOK_CONVERT,
    TOK_ADD,
    TOK_SUB,
    TOK_MUL,
    TOK_DIV,
    TOK_CARET,
    TOK_WHITESPACE,
    TOK_EXAMPLES,
    TOK_SHOW_UNITS,
    TOK_MEMORY,
    TOK_HELP,
    TOK_QUIT,
    TOK_END,
    TOK_INVALID,
};

typedef struct Token Token;
struct Token {
    TokenType type;
    union {
        UnitType unit_type;
        double number;
        unsigned char *var_name;
    };
};

#define MAX_INPUT 256

bool char_in_set(char c, const unsigned char *set, size_t set_length) {
    for (size_t i = 0; i < set_length; i++) {
        if (c == set[i]) {
            return true;
        }
    }
    return false;
}

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

double char_to_digit(char c) {
    return c - '0';
}

bool is_letter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

const Token invalid_token = {TOK_INVALID};
const Token end_token = {TOK_END};
const Token quit_token = {TOK_QUIT};
const Token whitespace_token = {TOK_WHITESPACE};
const Token help_token = {TOK_HELP};
const Token memory_token = {TOK_MEMORY};
const Token show_units_token = {TOK_SHOW_UNITS};
const Token examples_token = {TOK_EXAMPLES};
const Token add_token = {TOK_ADD};
const Token sub_token = {TOK_SUB};
const Token mul_token = {TOK_MUL};
const Token div_token = {TOK_DIV};
const Token caret_token = {TOK_CARET};
const Token convert_token = {TOK_CONVERT};
const Token equals_token = {TOK_EQUALS};

Token token_new_num(double num) {
    return (Token){TOK_NUM, .number = num };
}

Token token_new_unit(UnitType unit) {
    return (Token){TOK_UNIT, .unit_type = unit};
}

Token token_new_variable(char string_token[MAX_INPUT], Arena *arena) {
    size_t name_len = strnlen(string_token, MAX_INPUT) + 1;
    Token token = { .type = TOK_VAR, .var_name = arena_alloc(arena, name_len) };
    memcpy(token.var_name, string_token, name_len);
    return token;
}

// TODO: make this more generic where I can simply define
// basically a table of strings and their corresponding tokens
Token next_token(const char *input, size_t *pos, size_t length, Arena *arena) {
    if (*pos >= length || input[*pos] == '\0') {
        if (*pos != length) {
            debug("Invalid end of input, next: %c\n", input[*pos+1]);
            return invalid_token;
        }
        debug("End of input, next: %c\n", input[*pos+1]);
        return end_token;
    }

    if (is_letter(input[*pos])) {
        debug("Letter: %c, next: %c\n", input[*pos], input[*pos+1]);
        char string_token[MAX_INPUT] = {0};
        for (size_t i = 0; is_letter(input[*pos]) || is_digit(input[*pos]) || input[*pos] == '_'; i++) {
            string_token[i] = input[*pos];
            (*pos)++;
        }
        if (strnlen(string_token, 5) == 4
            && (strncmp(string_token, "quit", 4) == 0
                || strncmp(string_token, "exit", 4) == 0)) {
            return quit_token;
        }
        if (strnlen(string_token, 5) == 4 && strncmp(string_token, "help", 4) == 0) {
            return help_token;
        }
        if (strnlen(string_token, 7) == 6 && strncmp(string_token, "memory", 6) == 0) {
            return memory_token;
        }
        if (strnlen(string_token, 6) == 5 && strncmp(string_token, "units", 5) == 0) {
            return show_units_token;
        }
        if (strnlen(string_token, 9) == 8 && strncmp(string_token, "examples", 8) == 0) {
            return examples_token;
        }
        UnitType unit = string_to_unit(string_token);
        if (unit != UNIT_UNKNOWN) {
            return token_new_unit(unit);
        }
        return token_new_variable(string_token, arena);
    }

    const unsigned char whitespace[3] = {' ', '\t', '\n'};
    if (char_in_set(input[*pos], whitespace, sizeof(whitespace))) {
        debug("Whitespace, next: %c\n", input[*pos+1]);
        while (char_in_set(input[*pos], whitespace, sizeof(whitespace))) {
            (*pos)++;
        }
        return whitespace_token;
    }

    const unsigned char operators[6] = {'+', '-', '*', '/', '^', '='};
    if (char_in_set(input[*pos], operators, sizeof(operators))) {
        debug("Operator: %c\n", input[*pos]);
        Token token = invalid_token;
        if (input[*pos] == '+') {
            token = add_token;
        } else if (input[*pos] == '-') {
            token = sub_token;
            (*pos)++;
            if (input[*pos] == '>') {
                token = convert_token;
            } else {
                (*pos)--;
            }
        } else if (input[*pos] == '*') {
            token = mul_token;
        } else if (input[*pos] == '/') {
            token = div_token;
        } else if (input[*pos] == '^'){
            token = caret_token;
        } else {
            token = equals_token;
        }
        (*pos)++;
        return token;
    }

    if (is_digit(input[*pos])) {
        debug("Number: %c\n", input[*pos]);
        double number = 0;
        while (is_digit(input[*pos])) {
            number = number * 10 + char_to_digit(input[*pos]);
            (*pos)++;
        }
        if (input[*pos] == '.') {
            (*pos)++;
            double decimal = 0.1;
            while (is_digit(input[*pos])) {
                number += char_to_digit(input[*pos]) * decimal;
                decimal /= 10;
                (*pos)++;
            }
        }
        if (input[*pos] == 'e' || input[*pos] == 'E') {
            (*pos)++;
            if (!is_digit(input[*pos])) {
                return invalid_token;
            }
            double power = 0;
            while (is_digit(input[*pos])) {
                power += power * 10 + char_to_digit(input[*pos]);
                (*pos)++;
            }
            // TODO: overflow check
            number *= pow(10, power);
        }
        return token_new_num(number);
    }

    return invalid_token;
}

typedef struct TokenString TokenString;
struct TokenString {
    Token *tokens;
    size_t length;
};

TokenString tokenize(const char *input, Arena *arena) {
    TokenString tokens;
    tokens.tokens = arena_alloc(arena, sizeof(Token) * MAX_INPUT);
    tokens.length = 0;
    bool done = false;
    size_t pos = 0;
    size_t input_length = strnlen(input, MAX_INPUT + 1);
    if (input_length > MAX_INPUT) {
        tokens.tokens[0] = invalid_token;
        tokens.length = 1;
        return tokens;
    }
    while (!done) {
        Token token = next_token(input, &pos, input_length, arena);
        if (token.type == TOK_INVALID || token.type == TOK_END) {
            done = true;
        } else if (token.type == TOK_WHITESPACE) {
            continue;
        }
        tokens.tokens[tokens.length] = token;
        tokens.length++;
    }
    if (tokens.length > 0 && tokens.tokens[tokens.length - 1].type == TOK_END) {
        tokens.length -= 1;
    }
    return tokens;
}

String token_string(Token token, Arena *arena) {
    switch (token.type) {
        case TOK_END:
            return string_new("end", arena);
        case TOK_INVALID:
            return string_new("invalid", arena);
        case TOK_QUIT:
            return string_new("quit/exit", arena);
        case TOK_HELP:
            return string_new("help", arena);
        case TOK_MEMORY:
            return string_new("memory", arena);
        case TOK_SHOW_UNITS:
            return string_new("units", arena);
        case TOK_EXAMPLES:
            return string_new("examples", arena);
        case TOK_NUM:
            return string_new_fmt(arena, "%f", token.number);
        case TOK_VAR:
            return string_new((char *)token.var_name, arena);
        case TOK_EQUALS:
            return string_new("=", arena);
        case TOK_ADD:
            return string_new("+", arena);
        case TOK_SUB:
            return string_new("-", arena);
        case TOK_MUL:
            return string_new("*", arena);
        case TOK_DIV:
            return string_new("/", arena);
        case TOK_WHITESPACE:
            return string_new("whitespace", arena);
        case TOK_UNIT:
            return string_new((char *)unit_strings[token.unit_type], arena);
        case TOK_CONVERT:
            return string_new("->", arena);
        case TOK_CARET:
            return string_new("^", arena);
    }
}

void token_display(Token token, Arena *arena) {
    debug("Token: %s\n", token_string(token, arena).s);
}
