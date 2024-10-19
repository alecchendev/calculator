# Physics Calculator

A simple program to calculate expressions with physical units.

This is a miniature version of some broader compiler/interpreter techniques,
for fun/to help me learn C better.

### Usage

![Demo of using the website](demo.gif)

See [docs](docs.md).

### Develop

(Everything from the project's root directory. Assumes clang is installed.)

Run:
- Build + run: `make`
- With debug logs: `make DEBUG=1`

Test:
- Build + test: `make test`
- With debug logs: `make test DEBUG=1`
- Specific test case: `make test test=3 case=4`
- Disable spawning separate processes for each test/case: `make test fork=0`

Build to wasm:
- Download and install [emscripten](https://emscripten.org/docs/getting_started/downloads.html)
- `make wasm`
- Serve files: `python3 -m http.server --directory website`
- Open http://localhost:8000

See the makefile for more info.

Contributors welcome!
