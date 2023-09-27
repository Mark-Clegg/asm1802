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
| SUBROUTINE {relocatable} | Define a Subroutine |
| ENDSUB | End of Subroutine Definition |

## Operators

| Precedence | Operator | Meaning |
| :---: | :---: | --- |
| 1 | + - | Unary +/- |
| 2 | * / % | Multiply, Divide, Remainder |
| 3 | + - | Addition, Subtraction |
| 4 | << >> | Shift Left / Right |
| 5 | & | Bitwise AND |
| 6 | ^ | Bitwise Exclusive OR |
| 7 | \| | Bitwise OR |
