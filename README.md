# asm1802
Assembler for the CDP1802 series microprocessor

## Command Line Options

asm1802 {options} file {options} file ... file

Files and Options are processed cumulatively from left to right. When assembling a file, all options to the left of the filename are considered.

| Short | Long | Meaning |
| :---: | --- | --- |
| -D name{=value} | --define name{=value} | Define pre-processor variable |
| -U name | --undefine name | Remove pre-processor variable |
| -L | --list | Create listing file (.lst) |
| -S | --symbols | Append Symbol Table to listing |
| | --noregisters | Do not predefine Register equates (R0-RF) |
| | --noports | Do not predefine Post equates (P1-P7) |

## Pseudo Operators

| Menmonic | Meaning |
| --- | --- |
| ORG arg | Set Address |
| SUBROUTINE {ALIGN = 2\|4\|8\|16\|32\|64\|128\|256\|AUTO } | Define a Subroutine, optionally aligned to boundary |
| ENDSUB | End of Subroutine Definition |
| ALIGN arg | align to boundary (arg = 2,4,8,16,32,64,128 or 256) |
| PROCESSOR model | Set Processor architecture, 1802\|1806\|1806A |
| EQU value | Assign value to label |
| DB value list | Define Bytes, each value can be numeric or "string" |
| DW value list | Define double bytes |

## Operators

| Precedence | Operator | Meaning |
| :---: | :---: | --- |
| 1 | + - ~ ! | Unary +, -, Bitwise NOT, Logical NOT |
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
