# Pascal Compiler

A compiler for **Pascallite**, a small subset of Pascal, written in C++. It reads a Pascallite program and produces two things: a **listing file** (numbered source lines plus any error message) and **x86 assembly** in NASM syntax.

Final project for Algorithms and Compilers (CS 4301). This repo contains **Stage 1** of the compiler.

## What it does

The compiler runs in a single pass: a hand-written scanner feeds a recursive-descent parser, which fills a symbol table and generates assembly as it goes.

**Supported language**

- `program` header, `const` section, and `var` section
- `integer` and `boolean` types
- Assignment, `read(...)`, and `write(...)`
- Arithmetic: `+` `-` `*` `div` `mod`, plus unary `+` and `-`
- Boolean logic: `and` `or` `not`, with `true` / `false`
- Relational operators: `=` `<>` `<` `<=` `>` `>=`
- Correct operator precedence, left-to-right associativity, and parentheses
- `{ comments }`

**Not implemented yet**

Control flow (`if`, `while`, `repeat`: the keywords are reserved but not parsed), real numbers, and procedures.

## How it works

- **Scanner** (`nextToken`, `nextChar`): splits the source into tokens, skips comments, and writes each source line to the listing as it reads.
- **Parser**: one function per grammar production (`prog`, `constStmts`, `varStmts`, `assignStmt`, `express`, `term`, `factor`, `part`, ...). Operator precedence comes from the grammar levels: relational, then `+ - or`, then `* div mod and`, then unary operators.
- **Symbol table**: tracks each name's type, mode (constant or variable), value, and the internal label used in the generated assembly.
- **Code generator**: emits `eax`-based x86. It tracks what the A register currently holds so it can skip redundant loads, and it saves a temporary result to memory only when the register is needed for something else.
- **Error handling**: compilation stops at the first error and the listing shows the line number and a description.

## Example

Pascallite source:

```pascal
program smoke;
  const
    two = 2;
  var
    x, y, z: integer;
  begin
    read(x, y);
    z := x + y * two;
    write(x, y, z);
  end.
```

Generated assembly (the part that does the work):

```nasm
_start:
        call    ReadInt                 ; read int; value placed in eax
        mov     [I1],eax                ; store eax at x
        call    ReadInt                 ; read int; value placed in eax
        mov     [I2],eax                ; store eax at y
        imul    eax,[I0]                ; AReg = y * two
        add     eax,[I1]                ; AReg = x + T0
        mov     [I3],eax                ; z = AReg
        ...
```

With inputs `10` and `20`, the program prints `10`, `20`, and `50`.

A compile error shows up in the listing like this:

```
    1|program e; const k = 3; begin k := 4; 

Error: Line 1: symbol on left-hand side of assignment must have a storage mode of VARIABLE

COMPILATION TERMINATED      1 ERROR ENCOUNTERED
```

## Build and run

```bash
# build the compiler
g++ -I. stage11.cpp stage1main.C -o compiler

# compile a Pascallite program: source, listing, object (assembly)
./compiler program.pas program.lst program.asm
```

To assemble and run the output you need [NASM](https://www.nasm.us/) and the course's `Along32.inc` and `Macros_Along.inc` (I/O routines and macros). Those files belong to the course and are not included in this repo. For example, on 32-bit Linux:

```bash
nasm -f elf32 program.asm -o program.o
ld -m elf_i386 -o program program.o
./program
```

## Files

| File | Description |
| --- | --- |
| `stage11.cpp` | The compiler: scanner, parser, symbol table, and code generator. The class required a single submission file, so everything lives in one `.cpp`. |
| `stage1.h` | Class declarations (provided by the professor). |
| `stage1main.C` | Driver that runs the compiler (provided by the professor). |

## Credits

Written by Luis Arreola and Htoo Myat for CS 4301. `stage1.h` and `stage1main.C` were provided by the professor.
