#!/bin/python

import logging
import os
import re
import traceback
from lsprotocol import types as lsp
from pygls.server import LanguageServer
from typing import Optional, List, Union
from urllib.parse import urlparse, urlunparse

class OpCode:
    def __init__(self, Name, CPU, Description):
        self.Name = Name
        self.CPU = CPU
        self.Description = Description

CPU1802 = "CDP1802"
CPU1806 = "CDP1806"
CPU1806A = "CDP1806A"

PreProcessorRE = re.compile(r'^#(\w+)(\s+(.*))?$')
SourceLineRE = re.compile(r'^(((\w+):?\s*)|\s+)((\w+)(\s+(.*))?)?$')

OpCodeTable = {
    "adc":   OpCode("Add with Carry",CPU1802,"M(R(X))+D+DF -> DF, D"),
    "adci":  OpCode("Add with Carry Immediate",CPU1802,"M(R(P))+D+DF -> DF, D\nR(P)+1 -> R(P)"),
    "add":   OpCode("Add",CPU1802,"M(R(X))+D -> DF, D"),
    "adi":   OpCode("Add Immediate",CPU1802,"M(R(P))+D -> DF, D\nR(P)+1 -> R(P)"),
    "and":   OpCode("AND",CPU1802,"M(R(X)) AND D -> D"),
    "ani":   OpCode("AND Immediate",CPU1802,"M(R(P)) AND D -> D\nR(P)+1 -> R(P)"),
    "b1":    OpCode("Short Branch if EF1 = 1",CPU1802,"If EF1=1, M(R(P)) -> R(P).0\nelse R(P)+1 -> R(P)"),
    "b2":    OpCode("Short Branch if EF2 = 1",CPU1802,"If EF2=1, M(R(P)) -> R(P).0\nelse R(P)+1 -> R(P)"),
    "b3":    OpCode("Short Branch if EF3 = 1",CPU1802,"If EF3=1, M(R(P)) -> R(P).0\nelse R(P)+1 -> R(P)"),
    "b4":    OpCode("Short Branch if EF4 = 1",CPU1802,"If EF4=1, M(R(P)) -> R(P).0\nelse R(P)+1 -> R(P)"),
    "bci":   OpCode("Short Branch on Counter Interrupt",CPU1806,"If CI, M(R(P)) -> R(P).0\nelse R(P)+1 -> R(P)"),
    "bdf":   OpCode("Short Branch if DF = 1",CPU1802,"If DF=1, M(R(P)) -> R(P).0\nelse R(P)+1 -> R(P)"),
    "bge":   OpCode("Short Branch if Greater or Equal",CPU1802,"If DF=1, M(R(P)) -> R(P).0\nelse R(P)+1 -> R(P)"),
    "bl":    OpCode("Short Branch if Less",CPU1802,"If DF=0, M(R(P)) -> R(P).0\nelse R(P)+1 -> R(P)"),
    "bm":    OpCode("Short Branch if Minus",CPU1802,"If DF=0, M(R(P)) -> R(P).0\nelse R(P)+1 -> R(P)"),
    "bn1":   OpCode("Short Branch if EF1 = 0",CPU1802,"If EF1=0, M(R(P)) -> R(P).0\nelse R(P)+1 -> R(P)"),
    "bn2":   OpCode("Short Branch if EF2 = 0",CPU1802,"If EF2=0, M(R(P)) -> R(P).0\nelse R(P)+1 -> R(P)"),
    "bn3":   OpCode("Short Branch if EF3 = 0",CPU1802,"If EF3=0, M(R(P)) -> R(P).0\nelse R(P)+1 -> R(P)"),
    "bn4":   OpCode("Short Branch if EF4 = 0",CPU1802,"If EF4=0, M(R(P)) -> R(P).0\nelse R(P)+1 -> R(P)"),
    "bnf":   OpCode("Short Branch if DF = 0",CPU1802,"If DF=0, M(R(P)) -> R(P).0\nelse R(P)+1 -> R(P)"),
    "bnq":   OpCode("Short Branch if Q = 0",CPU1802,"If Q=0, M(R(P)) -> R(P).0\nelse R(P)+1 -> R(P)"),
    "bnz":   OpCode("Short Branch if D NOT 0",CPU1802,"If D NOT 0, M(R(P)) -> R(P).0\nelse R(P)+1 -> R(P)"),
    "bpz":   OpCode("Short Branch if Positive or Zero",CPU1802,"If DF=1, M(R(P)) -> R(P).0\nelse R(P)+1 -> R(P)"),
    "bq":    OpCode("Short Branch if Q = 1",CPU1802,"If Q=1, M(R(P)) -> R(P).0\nelse R(P)+1 -> R(P)"),
    "br":    OpCode("Short Branch",CPU1802,"M(R(P)) -> R(P).0"),
    "bxi":   OpCode("Short Branch on External Interrupt",CPU1806,"If XI, M(R(P)) -> R(P).0\nelse R(P)+1 -> R(P)"),
    "bz":    OpCode("Short Branch if D = 0",CPU1802,"If D=0, M(R(P)) -> R(P).0\nelse R(P)+1 -> R(P)"),
    "cid":   OpCode("Counter Interrupt Disable",CPU1806,"0 -> CIE"),
    "cie":   OpCode("Counter Interrupt Enable",CPU1806,"1 -> CIE"),
    "daci":  OpCode("Decimal Add with Carry Immediate",CPU1806A,"M(R(P))+D+DF -> DF, D\nR(P)+1 -> R(P)\nD Decimal Adjust -> DF, D"),
    "dadc":  OpCode("Decimal Add with Carry",CPU1806A,"M(R(X))+D+DF -> DF, D\nD Decimal Adjust -> DF, D"),
    "dadd":  OpCode("Decimal Add",CPU1806A,"M(R(X))+D -> DF, D\nD Decimal Adjust -> DF, D"),
    "dadi":  OpCode("Decimal Add Immediate",CPU1806A,"M(R(P))+D -> DF, D\nR(P)+1 -> R(P)\nD Decimal Adjust -> DF, D"),
    "dbnz":  OpCode("Decrement Register N and Long Branch if not 0",CPU1806A,"R(N)-1 -> R(N)\nIf R(N) not 0\nM(R(P)) -> R(P).1, M(R(P)+1) -> R(P).0\nelse\nR(P)+2 -> R(P)"),
    "dec":   OpCode("Decrement Register N",CPU1802,"R(N)-1 -> R(N)"),
    "dis":   OpCode("Disable",CPU1802,"M(R(X)) -> X, P\nR(X)+1 -> R(X)\n0 -> MIE"),
    "dsav":  OpCode("Save T, D, DF",CPU1806,"R(X)-1 -> R(X)\nT -> M(R(X))\nR(X)-1 -> R(X)\nD -> M(R(X))\nR(X)-1 -> R(X)\nShift D Right with Carry\nD -> M(R(X))"),
    "dsbi":  OpCode("Decimal Subtract Memory with Borrow Immediate",CPU1806A,"D-M(R(P))-(NOT DF) -> DF, D\nR(P)+1 -> R(P)\n D Decimal Adjust -> DF, D"),
    "dsm":   OpCode("Decimal Subtract Memory",CPU1802,"D-M(R(X)) -> DF, D\nD Decimal Adjust -> DF, D"),
    "dsmb":  OpCode("Decimal Subtract Memory with Borrow",CPU1806A,"D-M(R(X))-(NOT DF) -> DF, D\nD Decimal Adjust -> DF, D"),
    "dsmi":  OpCode("Decimal Subtract Memory Immediate",CPU1806A,"D-M(R(P)) -> DF, D\nR(P)+1 -> R(P)\nD Decimal Adjust -> DF, D"),
    "dtc":   OpCode("Decrement Timer/Counter",CPU1802,"Counter - 1 -> Counter"),
    "etq":   OpCode("Enable Toggle Q",CPU1806,"If Counter = 01\nNext Counter Clock Lo-Hi\n /Q -> Q"),
    "gec":   OpCode("Get Counter",CPU1806,"Counter -> D"),
    "ghi":   OpCode("Get High Register N",CPU1802,"R(N).1 -> D"),
    "glo":   OpCode("Get Low Register N",CPU1802,"R(N).0 -> D"),
    "idl":   OpCode("Idle",CPU1802,"Stop on TPB\nWait for DMA or Interrupt"),
    "inc":   OpCode("Increment Register N",CPU1802,"R(N)+1 -> R(N)"),
    "inp":   OpCode("Input",CPU1802,"BUS -> M(R(X))\nBUS -> D\nN -> N Lines"),
    "irx":   OpCode("Increment Register X",CPU1802,"R(X)+1 -> R(X)"),
    "lbdf":  OpCode("Long Brnach if DF = 1",CPU1802,"If DF=1, M(R(P)) -> R(P).1, M(R(P)+1) -> R(P).0\nelse R(P)+2 -> R(P)"),
    "lbnf":  OpCode("Long Brnach if DF = 0",CPU1802,"If DF=0, M(R(P)) -> R(P).1, M(R(P)+1) -> R(P).0\nelse R(P)+2 -> R(P)"),
    "lbnq":  OpCode("Long Brnach if Q = 0",CPU1802,"If Q=0, M(R(P)) -> R(P).1, M(R(P)+1) -> R(P).0\nelse R(P)+2 -> R(P)"),
    "lbnz":  OpCode("Long Brnach if D NOT 0",CPU1802,"If D NOT 0, M(R(P)) -> R(P).1, M(R(P)+1) -> R(P).0\nelse R(P)+2 -> R(P)"),
    "lbq":   OpCode("Long Brnach if Q = 1",CPU1802,"If Q=1, M(R(P)) -> R(P).1, M(R(P)+1) -> R(P).0\nelse R(P)+2 -> R(P)"),
    "lbr":   OpCode("Long Branch",CPU1802,"M(R(P)) -> R(P).1\nM(R(P)+1) -> R(P).0"),
    "lbz":   OpCode("Long Brnach if D = 0",CPU1802,"If D=0, M(R(P)) -> R(P).1, M(R(P)+1) -> R(P).0\nelse R(P)+2 -> R(P)"),
    "lda":   OpCode("Load Advance",CPU1802,"M(R(N)) -> D\nR(N)+1 -> R(N)"),
    "ldc":   OpCode("Load Counter",CPU1806,"If Counter Stopped\nD -> CH, D -> Counter\nelse\nD -> CH"),
    "ldi":   OpCode("Load Immediate",CPU1802,"M(R(P)) -> D\nR(P)+1 -> R(P)"),
    "ldn":   OpCode("Load via N",CPU1802,"M(R(N)) -> D\nFor N not 0"),
    "ldx":   OpCode("Load via X",CPU1802,"M(R(X)) -> D"),
    "ldxa":  OpCode("Load via X and Advance",CPU1802,"M(R(X)) -> D\nR(X)+1 -> R(X)"),
    "lsdf":  OpCode("Long Skip if DF = 1",CPU1802,"if DF=1, R(P)+2 -> R(P)\nelse Continue"),
    "lsie":  OpCode("Long Skip if MIE = 1",CPU1802,"if MIE=1, R(P)+2 -> R(P)\nelse Continue"),
    "lskp":  OpCode("Long Skip",CPU1802,"R(P)+2 -> R(P)"),
    "lsnf":  OpCode("Long Skip if DF = 0",CPU1802,"if DF=0, R(P)+2 -> R(P)\nelse Continue"),
    "lsnq":  OpCode("Long Skip if Q = 0",CPU1802,"if Q=0, R(P)+2 -> R(P)\nelse Continue"),
    "lsnz":  OpCode("Long Skip if D NOT 0",CPU1802,"if D NOT 0, R(P)+2 -> R(P)\nelse Continue"),
    "lsq":   OpCode("Long Skip if Q = 1",CPU1802,"if Q=1, R(P)+2 -> R(P)\nelse Continue"),
    "lsz":   OpCode("Long Skip if D = 0",CPU1802,"if D = 0, R(P)+2 -> R(P)\nelse Continue"),
    "mark":  OpCode("Push ",CPU1802,"(X, P) -> T\n(X, P) -> M(R(2))\nP -> X\nR(2)-1 -> R(2)"),
    "nbr":   OpCode("No Short Branch",CPU1802,"R(P)+1 -> R(P)"),
    "nlbr":  OpCode("No Long Branch",CPU1802,"R(P)+2 -> R(P)"),
    "nop":   OpCode("No Operation",CPU1802,"Continue"),
    "or":    OpCode("OR",CPU1802,"M(R(X)) OR D -> D"),
    "ori":   OpCode("OR Immediate",CPU1802,"M(R(P)) OR D -> D\nR(P)+1 -> R(P)"),
    "out":   OpCode("Output",CPU1802,"M(R(X)) -> BUS\nR(X)+1 -> R(X)\nN -> N Lines"),
    "phi":   OpCode("Put High Register N",CPU1802,"D -> R(N).1"),
    "plo":   OpCode("Put Low Register N",CPU1802,"D -> R(N).0"),
    "req":   OpCode("Reset Q",CPU1802,"0 -> Q"),
    "ret":   OpCode("Return",CPU1802,"M(R(X)) -> X, P\nR(X)+1 -> R(X)\n1 -> MIE"),
    "rldi":  OpCode("Register Load Immediate",CPU1806,"M(R(P)) -> R(N).1\nM(R(P)+1) -> R(N).0\nR(P)+2 -> R(P)"),
    "rlxa":  OpCode("Register Load via X and Advance",CPU1806,"M(R(X)) -> R(N).1\nM(R(X)+1) -> R(N).0\nR(X)+2 -> R(X)"),
    "rnx":   OpCode("Register N to Register X Copy",CPU1806,"R(N) -> R(X)"),
    "rshl":  OpCode("Ring Shift Left",CPU1802,"Shift D Left\nMSB(D) -> DF\nDF -> LSB(D)"),
    "rshr":  OpCode("Ring Shift Right",CPU1802,"Shift D Right\nLSB(D) -> DF\nDF -> MSB(D)"),
    "rsxd":  OpCode("Register Store via X and Decrement",CPU1806,"R(N).0 -> M(R(X))\nR(N).1 -> M(R(X)-1)\nR(X)-2 -> R(X)"),
    "sav":   OpCode("Save",CPU1802,"T -> M(R(X))"),
    "scal":  OpCode("Standard Call",CPU1806,"R(N).0 -> M(R(X))\nR(N).1 -> M(R(X)-1)\nR(X)-2 -> R(X)\nR(P) -> R(N)\nThen\nM(R(N)) -> R(P).1\nM(R(N)+1) -> R(P).0\nR(N)+2 -> R(N)"),
    "scm1":  OpCode("Set Counter Mode 1 and Start",CPU1806,"/EF1 -> Counter Clock"),
    "scm2":  OpCode("Set Counter Mode 2 and Start",CPU1806,"/EF2 -> Counter Clock"),
    "sd":    OpCode("Subtract D",CPU1802,"M(R(X))-D -> DF, D"),
    "sdb":   OpCode("Subtract D with Borrow",CPU1802,"M(R(X))-D-(NOT DF) -> DF, D"),
    "sdbi":  OpCode("Subtract D with Borrow Immediate",CPU1802,"M(R(P))-D-(NOT DF) -> DF, D\nR(P)+1 -> R(P)"),
    "sdi":   OpCode("Subtract D Immediate",CPU1802,"M(R(P))-D -> DF, D\nR(P)+1 -> R(P)"),
    "sep":   OpCode("Set P",CPU1802,"N -> P"),
    "seq":   OpCode("Set Q",CPU1802,"1 -> Q"),
    "sex":   OpCode("Set X",CPU1802,"N -> X"),
    "shl":   OpCode("Shift Left",CPU1802,"Shift D Left\nMSB(D) -> DF\n0 -> LSB(D)"),
    "shlc":  OpCode("Shift Left with Carry",CPU1802,"Shift D Left\nMSB(D) -> DF\nDF -> LSB(D)"),
    "shr":   OpCode("Shift Right",CPU1802,"Shift D Right\nLSB(D) -> DF\n0 -> MSB(D)"),
    "shrc":  OpCode("Shift Right with Carry",CPU1802,"Shift D Right\nLSB(D) -> DF\nDF -> MSB(D)"),
    "skp":   OpCode("Short Skip",CPU1802,"R(P)+1 -> R(P)"),
    "sm":    OpCode("Subtract Memory",CPU1802,"D-M(R(X) -> DF, D"),
    "smb":   OpCode("Subtract Memory with Borrow",CPU1802,"D-M(R(X))-(NOT DF) -> DF, D"),
    "smbi":  OpCode("Subtract Memory with Borrow Immediate",CPU1802,"D-M(R(P))-(NOT DF) -> DF, D\nR(P)+1 -> R(P)"),
    "smi":   OpCode("Subtract Memory Immediate",CPU1802,"D-M(R(P)) -> DF, D\nR(P)+1 -> R(P)"),
    "spm1":  OpCode("Set Pulse Width Mode 1 and Start",CPU1806,"TPA./EF1 -> Counter Clock\nEF1 Lo-Hi Stops Clock"),
    "spm2":  OpCode("Set Pulse Width Mode 2 and Start",CPU1806,"TPA./EF2 -> Counter Clock\nEF2 Lo-Hi Stops Clock"),
    "sret":  OpCode("Standard Return",CPU1806,"R(N) -> R(P)\nM(R(X)+1) -> R(N).1\nM(R(X)+2) -> R(N).0\nR(X)+2 -> R(X)"),
    "stm":   OpCode("Set Timer Mode and Start",CPU1806,"TPA / 32 -> Counter Clock"),
    "stpc":  OpCode("Stop Counter",CPU1806,"Stop Counter Clock\n0 -> /32 Prescaler"),
    "str":   OpCode("Store via N",CPU1802,"D -> M(R(N))"),
    "stxd":  OpCode("Store via X and Decrement",CPU1802,"D -> M(R(X))\nR(X)-1 -> R(X)"),
    "xid":   OpCode("External Interrupt Disable",CPU1806,"0 -> XIE"),
    "xie":   OpCode("External Interrupt Enable",CPU1806,"1 -> XIE"),
    "xor":   OpCode("Exclusive OR",CPU1802,"M(R(X)) XOR D -> D"),
    "xri":   OpCode("Exclusive OR Immediate",CPU1802,"M(R(P)) XOR D -> D\nR(P)+1 -> R(P)"),

    "db":         OpCode("DB value,...,value",CPU1802,"Inserts a sequence of comma separated bytes into the code stream. Parameters can either evaluate to single bytes, or be parsed as a double quoted string.\n\ne.g. DB 1,2,\"Hello World\""),
    "dw":         OpCode("DW value,...,value",CPU1802,"Inserts a sequence of comma separated words into the code stream. Each parameter is treated as a 16 bit number stored in big-endian format"),
    "dl":         OpCode("DL value,...,value",CPU1802,"Inserts a sequence of comma separated long words into the code stream. Each parameter is treated as a 32 bit number stored in big-endian format"),
    "dq":         OpCode("DQ value,...,value",CPU1802,"Inserts a sequence of comma separated quad words into the code stream. Each parameter is treated as a 64 bit number stored in big-endian format"),
    "rb":         OpCode("RB count",CPU1802,"Reserve count bytes of memory. No code iw written to the code stream, but the Program Counter is incremented accordingly"),
    "rw":         OpCode("RW count",CPU1802,"Reserve count words (2 bytes) of memory. No code iw written to the code stream, but the Program Counter is incremented accordingly"),
    "rl":         OpCode("RL count",CPU1802,"Reserve count longs (4 bytes) of memory. No code iw written to the code stream, but the Program Counter is incremented accordingly"),
    "rq":         OpCode("RQ count",CPU1802,"Reserve count quadwords (8 bytes) of memory. No code iw written to the code stream, but the Program Counter is incremented accordingly"),
    "assert":     OpCode("ASSERT expression",CPU1802,"Throws an error if the given expression evaluates to false"),
    "align":      OpCode("ALIGN expression {,PAD=byte}",CPU1802,"Increment the current address to the next 'expression' byte boundary.\nExpression must evaluate to a power of 2.\nOptionally pad skipped bytes with the 'byte' value given"),
    "macro":      OpCode("Label MACRO {parameters}",CPU1802,"Define a Macro. A label must be supplied, which names the macro. Any parameters listed can be used as tokens within the definition"),
    "endm":       OpCode("End Macro",CPU1802,"Marks the end of a Macro definition"),
    "endmacro":   OpCode("End Macro",CPU1802,"Marks the end of a Macro definition"),
    "subroutine": OpCode("Label SUBROUTINE {ALIGN=n|AUTO}, {PAD=padbyte}, {STATIC}",CPU1802,"Define a Subroutine. A label mus be supplied, which names the Subroutine. The following optional parameters can be supplied:\n\n"+
                                                       "ALIGN=<number>|AUTO\n: Align the subroutine to the given byte boundary, or Auto-Align to the nearest enclosing power of 2 sized block\n\n"+
                                                       "PAD=<padbyte>: When align is specified, fill missing bytes with padbyte.\n\n"+
                                                       "STATIC\n: Prevents the optimiser skipping assembly of the subroutine if it is not referenced elsewhere in the code."),
    "sub":        OpCode("Label SUBROUTINE {ALIGN=n|AUTO}, {STATIC}",CPU1802,"Define a Subroutine. A label mus be supplied, which names the Subroutine. The following optional parameters can be supplied:\n\n"+
                                                       "ALIGN=<number>|AUTO\n: Align the subroutine to the given byte boundary, or Auto-Align to the nearest enclosing power of 2 sized block\n\n"+
                                                       "PAD=<padbyte>: When align is specified, fill missing bytes with padbyte.\n\n"+
                                                       "STATIC\n: Prevents the optimiser skipping assembly of the subroutine if it is not referenced elsewhere in the code."),
    "endsub":     OpCode("ENDSUB {EntryPoint}",CPU1802,"Ends a Subroutine definition. ENDSUB can be followed by an optional Label, which sets the entry point for the subroutine."),
    "end":        OpCode("End of Source Code",CPU1802,"Marks the end of the source coe. No further lines are assembled. The optional parameter should evaluate to an address which is used as the entry point if the binary output format supports it."),
    "org":        OpCode("ORG {address}",CPU1802,"Set the current output address to the given expression"),
    "rorg":       OpCode("RORG {address}",CPU1802,"Relocate output. Calculate difference between current progam counter and givne address, and apply as an offset to binary output address for all subsequent code."),
    "rend":       OpCode("REND",CPU1802,"End Relocated code. (Equivalent to 'RORG .')"),
    "equ":        OpCode("Set Label",CPU1802,"Assign the value of the given expression to the supplied Label")
    }

logging.basicConfig(filename='/home/markc/pygls.log', filemode='w', level=logging.DEBUG)

server = LanguageServer('CDP1802 Assembler', 'v0.1')

@server.feature(lsp.TEXT_DOCUMENT_HOVER)
def hover(ls, params: lsp.TextDocumentPositionParams) -> lsp.Hover:
    try:
        uri = params.text_document.uri
        document = ls.workspace.get_text_document(uri)
        word = document.word_at_position(params.position).lower()

        # Exit if position is after end of trimmed line (i.e in a comment)
        if params.position.character >= len(Trim(document.lines[params.position.line])):
            return

        # Display OpCode operation
        if word in OpCodeTable:
            Data = OpCodeTable[word]
            return lsp.Hover(
                contents = lsp.MarkupContent(
                    kind = lsp.MarkupKind.Markdown,
                    value = "### " + word.upper() + " ("+Data.CPU+")\n\n**" + Data.Name + "**\n\n```\n" + Data.Description + "\n```\n"
                    )
                )

        Symbols = SymbolTable(ls, uri)

        # Show definition for EQUates
        lineNumber = params.position.line
        GotoURI, GotoLine = Symbols.LookupSymbol(ls, word, uri, lineNumber)
        if GotoURI:
            targetDoc = ls.workspace.get_text_document(GotoURI)
            line = targetDoc.lines[GotoLine]
            trimmedline = Trim(line)
            SourceLine = SourceLineRE.match(trimmedline)
            if SourceLine:
                Label, Mnemonic, Operands = SourceLine.group(3,5,7)
                if Mnemonic.lower() == "equ":
                    return lsp.Hover(
                        contents = lsp.MarkupContent(
                            kind = lsp.MarkupKind.Markdown,
                            value = "### " + word.upper() + " (EQUate)\n\n" + line
                            )
                        )

        # Display Macro definition
        uri,Lines,LineNumber = Symbols.MacroDefinition(ls, word)
        if Lines:
            uriparts = urlparse(uri)
            file = os.path.basename(uriparts.path)
            return lsp.Hover(
                contents = lsp.MarkupContent(
                    kind = lsp.MarkupKind.Markdown,
                   value = "### " + word.upper() + " (Macro)\n\nDefined in: "+file+" ("+str(LineNumber+1)+")\n\n```\n" + Lines + "\n```\n"
                   )
                )

        # Display Subroutine definition
        uri,Lines,LineNumber = Symbols.SubroutineDefinition(ls, word)
        if Lines:
            uriparts = urlparse(uri)
            file = os.path.basename(uriparts.path)
            return lsp.Hover(
                contents = lsp.MarkupContent(
                    kind = lsp.MarkupKind.Markdown,
                   value = "### " + word.upper() + " (Subroutine)\n\nDefined in: "+file+" ("+str(LineNumber+1)+")\n\n```\n" + Lines + "\n```\n"
                   )
                )

        # Display Label definition
        uri = params.text_document.uri
        document = ls.workspace.get_text_document(uri)
        line = params.position.line
        word = document.word_at_position(params.position).lower()
        LabelURI, LabelLine = Symbols.LookupSymbol(ls, word, uri, line)
        if LabelURI:
            urlparts = urlparse(LabelURI)
            file = os.path.basename(urlparts.path)
            return lsp.Hover(
                contents = lsp.MarkupContent(
                    kind = lsp.MarkupKind.Markdown,
                   value = "### " + word.upper() + " (Label)\n\nDefined in: " + file + " ("+str(LabelLine+1)+")\n"
                   )
                )

    except:
        ls.show_message("Exception Thrown\n" + traceback.format_exc())
    return None


def show_configuration_callback(ls):
    """Gets exampleConfiguration from the client settings using callback."""

    def _config_callback(config):
        try:
            #example_config = config[0].get("exampleConfiguration")

            ls.show_message(f"jsonServer.exampleConfiguration value: {config}")

        except Exception as e:
            ls.show_message_log(f"Error ocurred: {e}")

    cc = ls.client_capabilities
    if cc.workspace and cc.workspace.configuration:
        ls.show_message(f"Get Configuration {cc}")
        ls.get_configuration(
            lsp.WorkspaceConfigurationParams(
                items=[
                    lsp.ConfigurationItem(
                        scope_uri="", section="qtcreator" # ???????
                    )
                ]
            ),
            _config_callback
    )


@server.feature(lsp.TEXT_DOCUMENT_DEFINITION, lsp.DefinitionOptions())
def Definition(ls, params: lsp.DeclarationParams) -> Optional[Union[lsp.Location, List[lsp.Location], List[lsp.LocationLink]]]:
    try:

        show_configuration_callback(ls)

        uri = params.text_document.uri
        document = ls.workspace.get_text_document(uri)

        # Exit if position is after end of trimmed line (i.e in a comment)
        if params.position.character >= len(Trim(document.lines[params.position.line])):
            return

        line = params.position.line
        word = document.word_at_position(params.position).lower()

        if word in OpCodeTable:
            return None

        Symbols = SymbolTable(ls, params.text_document.uri)
        GotoURI, GotoLine = Symbols.LookupSymbol(ls, word, uri, line)
        location = lsp.Location(
            uri = GotoURI,
            range = lsp.Range(
                start = lsp.Position(line=GotoLine, character=0),
                end = lsp.Position(line=GotoLine, character=0)
                )
            )
        return location
    except:
        ls.show_message("Exception Thrown\n" + traceback.format_exc())

class Symbol:
    uri : str
    line : int
    def __init__(self, uri, line):
        self.uri = uri
        self.line = line

class Block:
    uri : str
    startLine : int
    endLine : int
    symbols : dict[str, Symbol]

    def __init__(self, uri, startLine):
        self.uri = uri
        self.startLine = startLine
        self.endLine = 0
        self.symbols = {}

    def __str__(self):
        return self.uri + " (" + str(self.startLine) + "," + str(self.endLine) + ")"

class SymbolTable:
    Globals : dict[str, Symbol] = {}
    Macro : dict[str, Block] = {}
    Subroutine : dict[str, Block] = {}

    CurrentMacro : str
    CurrentSubroutine : str = ""

    def __init__(self, ls, uri):
        self.ScanSource(ls, uri)

    def ScanSource(self, ls, uri):
        try:
            LineNumber = -1
            document = ls.workspace.get_text_document(uri)
            for line in document.lines:
                LineNumber += 1
                line = Trim(line)

                PreProcessor = PreProcessorRE.match(line)
                if PreProcessor:
                    match PreProcessor.group(1):
                        case "include":
                            IncludeRE = re.compile(R'^[<\"]([^>\"]+)[>\"]$')
                            IncludeFile = IncludeRE.match(PreProcessor.group(3))
                            if IncludeFile:
                                IncludeFile = IncludeFile.group(1)
                                uriparts = urlparse(uri)
                                if os.path.isabs(IncludeFile):
                                    newpath = IncludeFile
                                else:
                                    dir = os.path.dirname(uriparts.path)
                                    newpath = os.path.join(dir, IncludeFile)

                                self.ScanSource(ls, urlunparse(uriparts._replace(path=newpath)))
                    continue
                else:
                    SourceLine = SourceLineRE.match(line)
                    if SourceLine:
                        Label, Mnemonic, Operands = SourceLine.group(3,5,7)
                        if Label:
                            if Mnemonic:
                                match Mnemonic.upper():
                                    case 'SUBROUTINE' | 'SUB':
                                        self.RegisterLabel(Label, uri, LineNumber) # Add to Global Symbol Table
                                        self.CurrentSubroutine = Label.lower()
                                        self.Subroutine[Label.lower()] = Block(uri, LineNumber)
                                        self.RegisterLabel(Label, uri, LineNumber) # Add to Subroutine Symbol Table
                                    case 'MACRO':
                                        self.CurrentMacro = Label.lower()
                                        self.Macro[Label.lower()] = Block(uri, LineNumber)
                                    case _:
                                        self.RegisterLabel(Label, uri, LineNumber)
                            else:
                                self.RegisterLabel(Label, uri, LineNumber)
                        else:
                            if Mnemonic:
                                match Mnemonic.upper():
                                    case 'ENDSUB':
                                        self.Subroutine[self.CurrentSubroutine].endLine = LineNumber
                                        self.CurrentSubroutine=""
                                    case 'ENDM' | 'ENDMACRO':
                                        self.Macro[self.CurrentMacro].endLine = LineNumber
                                        self.CurrentMacro=""


        except:
            ls.show_message("Exception Thrown\n" + traceback.format_exc())

    def RegisterLabel(self, Label : str, uri : str, Line : int):
        """ Add the given symbol to the appropriate symbol table """
        if self.CurrentSubroutine:
            self.Subroutine[self.CurrentSubroutine].symbols[Label.lower()] = Symbol(uri, Line)
        else:
            self.Globals[Label.lower()] = Symbol(uri, Line)

    def LookupSymbol(self, ls, Label : str, uri : str, Line: int):
        if Label in self.Macro.keys():
            M = self.Macro[Label]
            return M.uri, M.startLine
        for S in self.Subroutine:
            Blk = self.Subroutine[S]
            if Blk.uri == uri and Blk.startLine <= Line and Blk.endLine >= Line and Label in Blk.symbols.keys():
                return Blk.symbols[Label].uri, Blk.symbols[Label].line
        if Label in self.Globals.keys():
            return self.Globals[Label].uri, self.Globals[Label].line
        return "",0

    def MacroDefinition(self, ls, Name : str) -> str:
        if Name in self.Macro.keys():
            M = self.Macro[Name]
            document = ls.workspace.get_text_document(M.uri)

            startLine = M.startLine
            while startLine > 0 and len(document.lines[startLine - 1]) > 1 and document.lines[startLine-1].startswith(";;"):
                startLine -= 1
            output = ""
            for Line in document.lines[startLine : M.endLine + 1]:
                if Line.rstrip(" \n\r\t"):
                    output += Line

            return M.uri, output, M.startLine
        return "","",0

    def SubroutineDefinition(self, ls, Name : str) -> str:
        if Name in self.Subroutine.keys():
            S = self.Subroutine[Name]
            document = ls.workspace.get_text_document(S.uri)

            startLine = S.startLine
            while startLine > 0 and len(document.lines[startLine - 1]) > 1 and document.lines[startLine-1].startswith(";;"):
                startLine -= 1

            output = ""
            for Line in document.lines[startLine : S.startLine + 1]:
                if Line.rstrip(" \n\r\t"):
                    output += Line

            return S.uri, output, S.startLine
        return "","",0

def Trim(input : str) -> str:
    """ Remove trailing white space and comments from string """
    inSingleQuote = False
    inDoubleQuote = False
    inEscape = False

    output = ""

    for ch in input:
        if not inSingleQuote and not inDoubleQuote and not inEscape:
            if ch == ';':
                break

            match ch:
                case '\'':
                    if not inDoubleQuote:
                        inSingleQuote = True
                        output += ch
                        continue
                case '\"':
                    if not inSingleQuote:
                        inDoubleQuote = True
                        output += ch
                        continue

        output += ch

        if (inDoubleQuote or inSingleQuote) and ch == '\\':
            inEscape = True
            continue
        if inSingleQuote and ch == '\'':
            inSingleQuote = False
            continue
        if inDoubleQuote and ch == '\"':
            inDoubleQuote = False
            continue
        if inEscape:
            inEscape = False
            continue

    return output.rstrip(" \n\r\t");

# def DumpSymbols(self, ls):
#     out : str = "SUBROUTINES\n"
#     for Sub in self.Subroutine:
#         Blk = self.Subroutine[Sub]
#         out += Sub + " : " + Blk.uri + " (" + str(Blk.startLine)+"-" + str(Blk.endLine)+")\n"
#         for Name in Blk.symbols:
#             Sym = Blk.symbols[Name]
#             out += "    " + Name + " : " + Sym.uri + " (" + str(Sym.line) + ")\n"
#     out += "GLOBALS\n"
#     for Name in self.Globals:
#         Sym = self.Globals[Name]
#         out += "    " + Name + " : " + Sym.uri + " (" + str(Sym.line) + ")\n"
#     out += "MACROS\n"
#     for Sub in self.Macro:
#         Blk = self.Macro[Sub]
#         out += Sub + " : " + Blk.uri + " (" + str(Blk.startLine)+"-" + str(Blk.endLine)+")\n"
#     ls.show_message(out)

server.start_io()
