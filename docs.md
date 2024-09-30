# Docs

All the things you can do with this tool. If there's a difference
between what's said here and how the actual program behaves, please report
it as a bug!

### General

Every number is treated as a signed 64-bit floating point number.

### Basic arithmetic

Addition: 1 + 2

Subtraction: 1 - 2

Multiplication: 1 * 2

Division: 1 / 2

Negation: -3

Scientific notation: 4.5e6 (negative degrees don't work, e.g. 4.5e-6)

Order of operations should be maintained. There's no ability to
do parentheses (but I'd like to add it!).

### Numbers with units

You can add units to numbers by just typing a unit after it.

Basic: 5 km

Composite: 5 km kg

Degrees: 5 m kg^2 s^-3

Division: 5 m / kg^2 s (= m kg^-2 s^-1)

You can't do two units of the same type: 5 km ft

Right now, you can combine units only by typing them after each other, not by multiplying with `*`.

Degrees are casted to integers: km ^ 4.567 (= km^4)

### Unit conversions

You can convert a number of one unit to another unit, as long as they
make sense to convert.

Convert: 1 km/s^2 -> mi/h^2

### Arithmetic on numbers with units

You can combine these!

Same unit: 5km + 2km

Same unit category (automatically converts to left-most unit): 5km + 2mi

Cannot add/subtract different unit categories: 5km + 2lb

Multiply: 5kg * 5m

Divide: 3 mi / 4 h

We will always give the answer in the most simplified form, using absolute units.

Numbers without units will not add/subtract with numbers with units, but they
will multiply/divide.

### Variables

You can store values in variables and then use them in expressions.
Variable expressions will always be evaluated and then stored.

Example:
- x = 4
- x + 6
- 10 km^x

### Unit aliases

Variables work for units as well.

Example:
- x = km
- 5 x
- y = s
- z = 10
- z x / y ^ z

Right now, answers will always display in absolute units, so if you made an
alias like `n = kg m / s^2`, the answer would never print using `n`.

You cannot set a bulitin unit as a variable/unit alias: km = lb

### All currently supported units

Distance: cm, m, km, in, ft, mi

Time: s, min, h

Mass: g, kg, lb, oz
