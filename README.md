# asm1802

asm1802 is a multi-pass assembler for the CDP1802 series microprocessor.

- Pre-Processor: Processes # directives, producing a single intermediate '.pp' file

The '.pp' file is then read several times by the main assembler:

- Pass 1: Define and expand MACROs, calculate the size of any SUBROUTINEs.
- Pass 2: Expand MACROs and assign values to Labels
- Pass 3: Expand MACROs, generate output and listing file.

After Pass 3, any unreferenced non-STATIC SUBROUTINE's are flagged for removal, and 
assembly restarts on Pass 2 until no unreferenced SUBs are found.

## Features

- C style '#' pre-processor directives
- Support for 1802, 1804/5/6 and 1804/5/6A microprocessors
- Subroutines with local labels
- Parameterised Macros
- Intel HEX compatible output
- Idiot/4 monitor compatible output
- Raw Binary output

## Building asm1802

asm1802 written in C++, is built using cmake, and uses the fmt library: https://fmt.dev/latest/index.html

From the project folder, execute:
```
$ cmake -DCMAKE_BUILD_TYPE=Release -B Release
$ cd Release
$ make
```

## Command Line Options

asm1802 {options} file.asm

All Options are processed first, before assembling any files.

| Short | Long | Meaning |
| --- | --- | --- |
| -C type | --cpu type | Set initial processor type |
| -D name{=value} | --define name{=value} | Define pre-processor variable |
| -U name | --undefine name | Remove pre-processor variable |
| -L | --list | Create listing file (.lst) |
| -S | --symbols | Append Symbol Tables to listing |
| -k | --keep-preprocessor | Do not delete intermediate pre-processor output (saved as file.pp) |
| -o format | --output format | Binary output format. "none" (default), "intel-hex", "idiot4" or "bin" |
| | --noregisters | Do not predefine Register equates (R0-RF) |
| | --noports | Do not predefine Port equates (P1-P7) |
| -v | --version | Display version number |
| -? | --help | display usage information |

## Pre-Processor

asm1802 supports a C-like pre-processor allowing for conditional assembly.
Pre-processor directives are denoted be a line with '#' in column 1.

The following Pre-processor directives are supported

- #processor {cputype}

Specify the CPU model. 1802, 1804/5/6, 1804/5/6A

- #define variable{=value}

Define a preprocessor variable and optionally assign a value

- #undefine variable

Remove the definition of a variable

- #if {condition}

Conditional assembly if \<condition\> evaluates to true (non-zero). In addition to arithmetic, the following functions are available:

| Function | Meaning |
| --- | --- |
| HIGH(expression) | Returns the high byte of expression (bits 8-15) |
| LOW(expression) | Returns the low byte of expression (bits 0-7) |
| PROCESSOR(designation) | Returns true(1) if designation is the currently selected processor model |
| CPU(designation) | Pseudonym for PROCESSOR(designation) |

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

- #list on|off

Turn on/off listing output

- #symbols on|off

Append symbol table to end of listing file

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
variable in the source code, is replaced by it's corresponding value.

## Source code syntax

Each source line should be formatted as follows:

```
; Comment
#pre-proccessor direcive
{Label}   {Mnemonic    {Operands}    ; Comment
```

Labels *MUST* begin in column 1, start with 'A'-'Z' or '\_' and can 
contain only letters, numbers and underscore. ([A-Z][0-9]\_). Labels 
can optionally be followed by a ':'.

Mnemonics must be preceeded by whitespace. Any required operands for the 
mnemonic should follow, with each operand separated by a ','.

Any text following a ';' is treated as a comment.

### Pre-Defined Labels

The following labels are pre-defined (see --noregisters/--noports command line options)

| Label | Value |
| --- | --- |
| R0 - R15 | 0 - 15 |
| R0 - RF | 0 - 15 |
| P1 - P7 | 1 -7 |

## Operands

All operands are expressions that evaluate to a value that is appropriate
for the mnemonic's specific parameter, e.g. Registers must be in 
the range 0-F, Ports, 1-7, Bytes 0-FF (0-255 or -128-127), Words 0-FFFF (0-65535 or -32768-32767).

### Numerical Constants

Numerical constants can be specified in decimal, octal or hex.

| Format | Meaning |
| --- | --- |
| 1234 | Decimal |
| 0123 | Octal (0 prefix)|
| $1234 | Hexadecimal |
| 0x1234 | Hexadecimal |
| . or $ | Current Address |

### Operator Precedence

The following operators are supported, and are evaluated according to the
precedence shown. Parentheses can be used to group sub-expressions.

| Precedence | Operator | Meaning |
| :---: | :---: | --- |
| 1 | + - ~ ! | Unary +, -, Bitwise NOT, Logical NOT |
| 2 | . | Byte selector e.g. (label.7) ... (label.1) (label.0) |
| 2 | * / % | Multiply, Divide, Remainder |
| 3 | + - | Addition, Subtraction |
| 4 | << >> | Shift Left / Right |
| 5 | < <= >= > | Less, Less or Equal, Greater or Equal, Greater |
| 6 | = == != | Equality, Non-Equality (= and == treated the same |
| 7 | & | Bitwise AND |
| 8 | ^ | Bitwise Exclusive OR |
| 9 | \| | Bitwise OR |
| 10 | && | Logical AND |
| 11 | \|\| | Logical OR |

For Logical operators, 0 = false, 1 = true.

### Functions

 | Function | Meaning |
 | --- | --- |
 | HIGH(expression) | HIGH order 8 bits of 16 bit value. (same as value.1) |
 | LOW(expression) | LOW order 8 bits of 16 bit value. (Same as value.0) |
 | ISDEF(label) | True if label is defined |
 | ISNDEF(label) | True if label is not defined |
 | PROCESSOR(designation) | True if designated processor is suppported |
 | CPU(designation) | Pseudonym for PROCESSOR(designation) |

## Pseudo Operators

| Menmonic | Meaning |
| --- | --- |
| ALIGN arg {, PAD=padbyte} | Increment Program Counter to align with a power of 2 boundary (arg = 2,4,8,16,32,64,128 or 256), Optionally filling space with PadByte |
| ASSERT expression | Throw an error if expression evaluates to false (0) |
| DB value list | Define Bytes (see below) |
| DW value list | Define Word (2 bytes) |
| DL value list | Define Long (4 bytes) |
| DQ value list | Define QuadWord (8 bytes) |
| RB count | Reserve count Bytes |
| RW count | Reserve count Words (2 bytes) |
| RL count | Reserve count Longs (4 bytes) |
| RQ count | Reserve count QuadWorda (8 bytes) |
| EQU value | Assign value to label |
| ORG arg | Set Address |
| SUBROUTINE {ALIGN = 2\|4\|8\|16\|32\|64\|128\|256\|AUTO}, {PAD=padbyte}, {STATIC} | Define a Subroutine, optionally aligned to boundary, optionally padding with padbyte, and optionally prevent removal if unreferenced |
| ENDSUB {expression}| End of Subroutine Definition. Optionally set the entypoint to expression |
| MACRO parameters | Define a Macro |
| ENDM | End of Macro Definition |
| END expression | End of source code. Expression sets the start address |

### Notes

- In addition to a list of byte values, the DB pseudo-op also accepts the following alternate operands:
    - ```DB "ABC"``` is equivalent to ```DB $41, $42, $43```
    - ```DB @"filename"``` inserts the contents of the specified file into the output stream

    All formats can be combined as required, e.g. ```DB $41, @"filename", "string", 0```

- The RB,RW,RL,RQ pseudo-ops reserve space for the given number of items, without writing anything
to the output stream.

## Subroutines

### SUBROUTINE {ALIGN=x}, {PAD=byte}, {STATIC}

... defines a subroutine.

#### Optional arguments

- ALIGN=...: Align the subroutine to the specified 
power of 2 boudary. (e.g. ALIGN=32). 

- ALIGN=AUTO: If the entire SUBROUTINE will not fit in the current page, then align to the
nearest power of 2 boundary greater or equal to the SUBROUTINE size (as determined during 
pass 1). This allows the inclusion of library modules ensuring that short branches remain 
in range wherever the code is included.

- PAD { = byte}: When ALIGN is specified, fill any skipped bytes with the optionally given value, instead of leaving an 
unintialised gap (default: 0).

- STATIC: By default, SUBROUTINES that are not referenced, are removed from the assembly.
Flagging a SUBROUTINE as STATIC forces assembly of the SUBROUTINE regardless of whether
or not it is used.

Any labels defined within the subroutine are local to that subroutine, and
cannot be referenced elsewhere. The subroutine name is added to the symbol table with the current assemly
address, or as set in the matching ENDSUB, if specified.

### ENDSUB {expression}

Marks the end of a SUBROUTINE definition. If an optional expression is supplied, this is evaluated
and marks the entry point of that subroutine. This modifies the value of the SUBROUTINE's label 
in the master symbol table.

e.g.
```
                    ; Main Code
0000 F8 10                  LDI     HIGH(FlashQ)
0002 B6                     PHI     R6
0003 F8 01                  LDI     LOW(FlashQ)
0005 A6                     PLO     R6
0006 D6                     SEP     R6  

                            ...
                    ; Subroutine        
                    FlashQ  SUBROUTINE
1000 D3                     SEP     R3
1001 7B             START   SEQ
1002 7A                     REQ
1003 30 00                  BR      FlashQ
                            ENDSUB  START
```

Main code loads R6 with the entry point address of the Flashq Subroutine, as specified by the ENDSUB
statement. This evaluates to the subroutines local START label: 1001. 

Within the subroutine, FlashQ retains it's initial address 1000.

## Macros

```
; Definition
MAC     MACRO    Param1, Param2... , ParamN
        LDI      Param1
        DB       Param2
        ...
        ADCI     ParamN
        ENDMACRO
; usage        
        MAC      1, "Hello", 'C'
```
... defines a Macro named MAC. Macros can be used anywhere in code, by using 
the macro name as the OpCode, followed by a matching list of arguments.
During assembly, the name is replaced by its content, and Param1..N are substituted where used.

### Notes

- Macros defined inside a subroutine, are local to that subroutine.

- Macros cannot contain labels.

# Output Formats

The "-o format" command line option sets the desired assembly output format:

## -o none

Do not generate an output file. (This is the default).

## -o intel_hex

Generates intel hex compatible output. File: filename.hex

## -o idiot4

Generates a sequence of commands suitable for pasting into the idiot4 monitor. File: filename.idiot

## -o bin

Generates a raw memory image for the assembled code. File: filename.bin, 

Output starts with the lowest addressed assembled byte, and is padded with zeros where gaps
would result from non PADded ALIGNs and ORGs

# Syntax Highlighting

Linux users using editors based on KatePart can benefit from syntax highlighting using
the installed /usr/local/share/org.kde.syntax-highlighting/syntax/asm1802.xml highlighting 
definitions.  This should happen automatically in Kate, providing the source file is 
recognised as 1802 assembler. (You can check/force this by clicking on the file type
on the right had side of kate's status bar, and selecting "CDP1802 Assembler (asm1802)"

# Language Server Protocol

If your editor supports the Language Server Protocol, a rudimentry server is installed
as /usr/local/bin/cdp1802-languageserver.py. Configure your editor to use this server 
using JSON/RCP over STDIO

With the Language Server in use, hovering over opcode mnemonics will display the instructions
operation, (e.g. ```LDI n, Load Immediate, M(R(P)) -> D, R(P)+1 -> R(P)``` )

For SUBROUTINEs and MACROs the location of the definition is shown along with any supporting 
documentation comment.

Documentation commets are "double commented" comments immediately preceeding the definition. (e.g. ```;; Comment```)

Right-Click-GoTo-Definition is also supported on Labels, MACROs and SUBROUTINEs.

## Kate

Within Kate, select "Settings", "Configure Kate" then select the "LSP Client" node and 
"User Server Settings" tab. Enter/add the following:

```
{
    "servers": {
        "asm1802": {
            "command": ["/usr/local/bin/cdp1802-languageserver.py", "start"],
            "root": ""
        }
    }
}
```

## QTCreator

Within QTCreator, select "Edit", "Preferences", then select the "Language Client" and "Add" 
a new "Generic Stdio Language Server". Set the MIME type to text/x-asm, file mask to *.asm 
and path to executable to /usr/local/bin/cdp1802-languageserver.py
