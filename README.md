# asm1802
Assembler for the CDP1802 series microprocessor

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
