# asm1802
Assembler for the CDP1802 series microprocessor. 

## Features

- Support for 1802, 1804/5/6 and 1804/5/6A microprocessors
- Subroutines with local labels
- Alignable subroutines to support pseudo-relocatable includes
- Intel HEX output
- Idiot/4 monitor output

## Subroutines

### SUBROUTINE {ALIGN=...}

... defines a subroutine.

#### Optional arguments

ALIGN=... Align the subroutine to the specified 
power of 2 boudary. (e.g. ALIGN=32). Specifying ALIGN=AUTO will align to the nearest greater
power of two boundary. This facilitates the creation of library modules that can be #included
anywhere in code ensuring that short branches remain in range wherever the code is included.
To prevent any following code from having out of range branches, it is recommended that
#included library code is placed at the end of the source.

Any labels defined within the subroutine are local to that subroutine, and
cannot be referenced elsewhere. The subroutine name itself appears in both the 
local and global symbol tables, with possibly different values.

### ENDSUB {expression}

Marks the end of a SUBROUTINE definition. If an optional Label is supplied, this must refer 
to a previously declared Label within the subroutine, and marks the entyr point of that 
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
| | --noports | Do not predefine Post equates (P1-P7) |

## Numerical Constants

| Format | Meaning |
| --- | --- |
| 1234 | Decimal |
| 0123 | Octal |
| $1234 | Hexadecimal |
| 0x1234 | Hexadecimal |
| . / $ | Current Address |

## Pseudo Operators

| Menmonic | Meaning |
| --- | --- |
| ALIGN arg | align to boundary (arg = 2,4,8,16,32,64,128 or 256) |
| ASSERT expression | Throw an error if expression evaluates to false (0) |
| DB value list | Define Bytes, each value can be numeric or "string" |
| DW value list | Define double bytes |
| EQU value | Assign value to label |
| ISDEF/ISDEFINED label | Return true if label is defined |
| ISUNDEF/ISUNDEFINED label | Return true if lable is not defined |
| ORG arg | Set Address |
| PROCESSOR model | Set Processor architecture, 1802\|1806\|1806A |
| SUBROUTINE {ALIGN = 2\|4\|8\|16\|32\|64\|128\|256\|AUTO } | Define a Subroutine, optionally aligned to boundary |
| ENDSUB | End of Subroutine Definition |
| MACRO parameters | Define a Macro |
| ENDM | End of Macro Definition |

## Operators

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

## Functions
 | Function | Parameters | Meaning |
 | --- | --- | --- |
 | HIGH(expression) | value | High order 8 bits of value |
 | LOW(expression) | value | LOW order 8 bits of value |

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
