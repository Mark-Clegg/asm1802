# asm1802

Assembler for the CDP1802 series microprocessor. 

asm1802 is a three pass assembler, scanning the source file(s) 3 times:

- Pass 1: Define and expand Macros, and calculate the size of any subroutines.
- Pass 2: Expand Macros and assign values to Labels
- Pass 3: Expand Macros, generate output and listing file.

## Features

- Support for 1802, 1804/5/6 and 1804/5/6A microprocessors
- Subroutines with local labels
- Parameterised Macros
- C style '#' pre-processor directives
- Intel HEX compatible output
- Idiot/4 monitor compatible output

## Building asm1802

asm1802 written in C++, is built using cmake, and uses the fmt library: https://fmt.dev/latest/index.html

From the project folder, execute:
```
$ cmake -DCMAKE_BUILD_TYPE=Release -B Release
$ cd Release
$ make
```

## Command Line Options

asm1802 {options} file {options} file ... file

All Options are processed first, before assembling any files.

| Short | Long | Meaning |
| :---: | --- | --- |
| -D name{=value} | --define name{=value} | Define pre-processor variable |
| -U name | --undefine name | Remove pre-processor variable |
| -L | --list | Create listing file (.lst) |
| -S | --symbols | Append Symbol Table to listing |
| | --noregisters | Do not predefine Register equates (R0-RF) |
| | --noports | Do not predefine Port equates (P1-P7) |

## Pre-Processor

asm1802 supports a C-like pre-processor allowing for conditional assembly.
Pre-processor directives are denoted be a line with '#' in column 1.

The following Pre-processor directives are supported

- #processor "{cputype}"

Specify the CPU model. 1802, 1804/5/6, 1804/5/6A

- #define variable{=value}

Define a preprocessor variable and optionally assign a value

- #undefine variable

Remove the definition of a variable

- #if {condition}

Conditional assembly if \<condition\> evaluates to true (non-zero). In addition to arithmetic, the following functions are available:

    - HIGH(expression)
    Returns the high byte of expression
    - LOW(expression)
    Returns the low byte of expression
    - PROCESSOR("designation")
    Returns true(1) if designation is the currently selected processor model

- #ifdef variable

Conditional assembly if variable is defined

- #ifndef variable

Conditional assembly if variable is nto defined

- #elif {condition}

Alternative IF clause as part of an #if sequence

- #else

Alternate branch for #if... directives

- #endif

End of conditional assembly block

- #include \<filename\> | "filename"

Include the contents of the specified file into the input stream.

### Predefined Pre-processor variables

| Variable | Value |
| --- | --- |
| \_\_DATE\_\_ | Date in "MMM DD YYYY" format |
| \_\_TIME\_\_ | Time in 24h "hh:mm:ss" format |
| \_\_TIMESTAMP\_\_ | Date Time in "ddd MMM dd  hh:mm:ss" format |
| \_\_FILE\_\_ | Current source filename |
| \_\_LINE\_\_ | Current source line number |

Note that Pre-processor variables are distinct from labels specified 
during assembly. During pre-processing, any reference to a pre-processor
variable in the source code, is replaced by it's correspinding value.

## Source code syntax

Each source line should be formatted as follows:

```
; Comment
#pre-proccessor direcive
{Label}   {Mnemonic    {Operands}}    ; Comment
```

Labels *MUST* begin in column 1, start with 'A'-'Z' or '\_' and can 
contain only letters, numbers and underscore. ([A-Z][0-9]\_). Labels 
can optionally be followed by a ':'.

Mnemonics must be preceeded by whitespace. Any required operands for the 
mnemonic should follow, with each operand separated by a ','.

Any text following a ';' is treated as a comment.

### Pre-Defined Labels

The following labels are pre-defined

| Label | Value |
| --- | --- |
| R0 - R15 | 0 - 15 |
| R0 - RF | 0 - 15 |
| P1 - P7 | 1 -7 |
| ON | TRUE | 1 |
| OFF | FALSE | 0 |


## Operands

Operands are expressions that must evaluate to a value that must be in 
range for the mnemonic's specific parameter, e.g. Registers must be in 
the range 0-F(15), Ports, 1-7, Bytes 0-FF(255), Words 0-FFFF(65535).

The 'DB' pseudo op also allows a list of bytes to be specified as a string. 
e.g. "ABC" evaluates to 41,42,43

### Numerical Constants

Numerical constants can be specified in decimal, octal or hex.

| Format | Meaning |
| --- | --- |
| 1234 | Decimal |
| 0123 | Octal |
| $1234 | Hexadecimal |
| 0x1234 | Hexadecimal |
| . / $ | Current Address |

### Operator Precedence

The following operators are supported, and are evaluated according to the
precedence shown.

| Precedence | Operator | Meaning |
| :---: | :---: | --- |
| 1 | + - ~ ! | Unary +, -, Bitwise NOT, Logical NOT |
| 2 | . | High Low selector e.g. (label.1) (label.0) |
| 2 | * / % | Multiply, Divide, Remainder |
| 3 | + - | Addition, Subtraction |
| 4 | << >> | Shift Left / Right |
| 5 | < <= >= > | Less, Less or Equal, Greater or Euqal, Greater |
| 6 | = == != | Equality, Non-Equality (= and == treated the same |
| 7 | & | Bitwise AND |
| 8 | ^ | Bitwise Exclusive OR |
| 9 | \| | Bitwise OR |
| 10 | && | Logical AND |
| 11 | \|\| | Logical OR |

For Logical operators, 0 = false, 1 = true.

### Functions

 | Function | Parameters | Meaning |
 | --- | --- | --- |
 | HIGH(expression) | value | HIGH order 8 bits of value |
 | LOW(expression) | value | LOW order 8 bits of value |

## Pseudo Operators

| Menmonic | Meaning |
| --- | --- |
| ALIGN arg | align to boundary (arg = 2,4,8,16,32,64,128 or 256) |
| ASSERT expression | Throw an error if expression evaluates to false (0) |
| DB value list | Define Bytes, each value can be numeric or "string" |
| BYTE value list | Pseudonym for DB |
| DW value list | Define double bytes |
| WORD value list | Pseudonym for DW |
| EQU value | Assign value to label |
| ISDEF/ISDEFINED label | Return true if label is defined |
| ISUNDEF/ISUNDEFINED label | Return true if lable is not defined |
| ORG arg | Set Address |
| SUBROUTINE {ALIGN = 2\|4\|8\|16\|32\|64\|128\|256\|AUTO } | Define a Subroutine, optionally aligned to boundary |
| ENDSUB | End of Subroutine Definition |
| MACRO parameters | Define a Macro |
| ENDM | End of Macro Definition |
| LIST value | Turn Listing output on (1) or off (0) |
| SYMBOLS value | Turn on symbol table dump (1) or off (0) |

## Subroutines

### SUBROUTINE {ALIGN=...}

... defines a subroutine.

#### Optional arguments

ALIGN=... Align the subroutine to the specified 
power of 2 boudary. (e.g. ALIGN=32). Specifying ALIGN=AUTO will align to the nearest greater
power of two boundary. This allows the creation of library modules that can be #included
anywhere in code ensuring that short branches remain in range wherever the code is included.
To prevent any following code from having out of range branches, it is recommended that
#included library code is placed at the end of the source.

Any labels defined within the subroutine are local to that subroutine, and
cannot be referenced elsewhere. The subroutine name itself appears in both the 
local and global symbol tables (with possibly different values. see below)

### ENDSUB {expression}

Marks the end of a SUBROUTINE definition. If an optional Label is supplied, this must refer 
to a previously declared Label within the subroutine, and marks the entry point of that 
subroutine. This modifies the value of the SUBROUTINE's label in the Master symbol table to
point to the referenced local label so that any reference to the SUBROUTINE name in main code
references the local label instead of the first byte of the SUBROUTINE itself.

e.g.
```
0000 F8 10                  LDI     HIGH(FlashQ)
0002 B6                     PHI     R6
0003 F8 01                  LDI     LOW(FlashQ)
0005 A6                     PLO     R6
0006 D6                     SEP     R6  

                            ...
                            
                    FlashQ  SUBROUTINE
1000 D3                     SEP     R3
1001 7B             START   SEQ
1002 7A                     REQ
1003 30 00                  BR      FlashQ
                            ENDSUB  START
```

Main code loads R6 with the address of the local START label (1001). However, within the
subroutine, FlashQ will have it's initial address of 1000.

## Macros

```
; Definition
NAME    MACRO   Param1, Param2... , ParamN
        Assembly code
        ENDMACRO
; usage        
        NAME    1, "Hello", 'C'
```
... defines a Macro named LABEL. Macros can be used anywhere in code, by using 
LABEL as the OpCode, followed by a matching list of arguments.
During assembly, Param1..N are substituted from the call.

### Notes

- Macros defined inside a subroutine, are local to that subroutine.

- Macros cannot contain labels.
