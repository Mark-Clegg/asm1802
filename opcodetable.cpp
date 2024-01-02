#include "opcodetable.h"

const std::map<std::string, OpCodeSpec> OpCodeTable::OpCode =
{
    { "IDL",  { IDL,  OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "LDN",  { LDN,  OpCodeTypeEnum::REGISTER,                      CPUTypeEnum::CPU_1802  }},
    { "INC",  { INC,  OpCodeTypeEnum::REGISTER,                      CPUTypeEnum::CPU_1802  }},
    { "DEC",  { DEC,  OpCodeTypeEnum::REGISTER,                      CPUTypeEnum::CPU_1802  }},
    { "BR",   { BR,   OpCodeTypeEnum::SHORT_BRANCH,                  CPUTypeEnum::CPU_1802  }},
    { "BQ",   { BQ,   OpCodeTypeEnum::SHORT_BRANCH,                  CPUTypeEnum::CPU_1802  }},
    { "BZ",   { BZ,   OpCodeTypeEnum::SHORT_BRANCH,                  CPUTypeEnum::CPU_1802  }},
    { "BDF",  { BDF,  OpCodeTypeEnum::SHORT_BRANCH,                  CPUTypeEnum::CPU_1802  }},
    { "BPZ",  { BPZ,  OpCodeTypeEnum::SHORT_BRANCH,                  CPUTypeEnum::CPU_1802  }},
    { "BGE",  { BGE,  OpCodeTypeEnum::SHORT_BRANCH,                  CPUTypeEnum::CPU_1802  }},
    { "B1",   { B1,   OpCodeTypeEnum::SHORT_BRANCH,                  CPUTypeEnum::CPU_1802  }},
    { "B2",   { B2,   OpCodeTypeEnum::SHORT_BRANCH,                  CPUTypeEnum::CPU_1802  }},
    { "B3",   { B3,   OpCodeTypeEnum::SHORT_BRANCH,                  CPUTypeEnum::CPU_1802  }},
    { "B4",   { B4,   OpCodeTypeEnum::SHORT_BRANCH,                  CPUTypeEnum::CPU_1802  }},
    { "NBR",  { NBR,  OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "SKP",  { SKP,  OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "BNQ",  { BNQ,  OpCodeTypeEnum::SHORT_BRANCH,                  CPUTypeEnum::CPU_1802  }},
    { "BNZ",  { BNZ,  OpCodeTypeEnum::SHORT_BRANCH,                  CPUTypeEnum::CPU_1802  }},
    { "BNF",  { BNF,  OpCodeTypeEnum::SHORT_BRANCH,                  CPUTypeEnum::CPU_1802  }},
    { "BM",   { BM,   OpCodeTypeEnum::SHORT_BRANCH,                  CPUTypeEnum::CPU_1802  }},
    { "BL",   { BL,   OpCodeTypeEnum::SHORT_BRANCH,                  CPUTypeEnum::CPU_1802  }},
    { "BN1",  { BN1,  OpCodeTypeEnum::SHORT_BRANCH,                  CPUTypeEnum::CPU_1802  }},
    { "BN2",  { BN2,  OpCodeTypeEnum::SHORT_BRANCH,                  CPUTypeEnum::CPU_1802  }},
    { "BN3",  { BN3,  OpCodeTypeEnum::SHORT_BRANCH,                  CPUTypeEnum::CPU_1802  }},
    { "BN4",  { BN4,  OpCodeTypeEnum::SHORT_BRANCH,                  CPUTypeEnum::CPU_1802  }},
    { "LDA",  { LDA,  OpCodeTypeEnum::REGISTER,                      CPUTypeEnum::CPU_1802  }},
    { "STR",  { STR,  OpCodeTypeEnum::REGISTER,                      CPUTypeEnum::CPU_1802  }},
    { "IRX",  { IRX,  OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "OUT",  { OUT,  OpCodeTypeEnum::INPUT_OUTPUT,                  CPUTypeEnum::CPU_1802  }},
    { "INP",  { INP,  OpCodeTypeEnum::INPUT_OUTPUT,                  CPUTypeEnum::CPU_1802  }},
    { "RET",  { RET,  OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "DIS",  { DIS,  OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "LDXA", { LDXA, OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "STXD", { STXD, OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "ADC",  { ADC,  OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "SDB",  { SDB,  OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "SHRC", { SHRC, OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "RSHR", { RSHR, OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "SMB",  { SMB,  OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "SAV",  { SAV,  OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "MARK", { MARK, OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "REQ",  { REQ,  OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "SEQ",  { SEQ,  OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "ADCI", { ADCI, OpCodeTypeEnum::IMMEDIATE,                     CPUTypeEnum::CPU_1802  }},
    { "SDBI", { SDBI, OpCodeTypeEnum::IMMEDIATE,                     CPUTypeEnum::CPU_1802  }},
    { "SHLC", { SHLC, OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "RSHL", { RSHL, OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "SMBI", { SMBI, OpCodeTypeEnum::IMMEDIATE,                     CPUTypeEnum::CPU_1802  }},
    { "GLO",  { GLO,  OpCodeTypeEnum::REGISTER,                      CPUTypeEnum::CPU_1802  }},
    { "GHI",  { GHI,  OpCodeTypeEnum::REGISTER,                      CPUTypeEnum::CPU_1802  }},
    { "PLO",  { PLO,  OpCodeTypeEnum::REGISTER,                      CPUTypeEnum::CPU_1802  }},
    { "PHI",  { PHI,  OpCodeTypeEnum::REGISTER,                      CPUTypeEnum::CPU_1802  }},
    { "LBR",  { LBR,  OpCodeTypeEnum::LONG_BRANCH,                   CPUTypeEnum::CPU_1802  }},
    { "LBQ",  { LBQ,  OpCodeTypeEnum::LONG_BRANCH,                   CPUTypeEnum::CPU_1802  }},
    { "LBZ",  { LBZ,  OpCodeTypeEnum::LONG_BRANCH,                   CPUTypeEnum::CPU_1802  }},
    { "LBDF", { LBDF, OpCodeTypeEnum::LONG_BRANCH,                   CPUTypeEnum::CPU_1802  }},
    { "NOP",  { NOP,  OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "LSNQ", { LSNQ, OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "LSNZ", { LSNZ, OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "LSNF", { LSNF, OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "LSKP", { LSKP, OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "NLBR", { NLBR, OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "LBNQ", { LBNQ, OpCodeTypeEnum::LONG_BRANCH,                   CPUTypeEnum::CPU_1802  }},
    { "LBNZ", { LBNZ, OpCodeTypeEnum::LONG_BRANCH,                   CPUTypeEnum::CPU_1802  }},
    { "LBNF", { LBNF, OpCodeTypeEnum::LONG_BRANCH,                   CPUTypeEnum::CPU_1802  }},
    { "LSIE", { LSIE, OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "LSQ",  { LSQ,  OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "LSZ",  { LSZ,  OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "LSDF", { LSDF, OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "SEP",  { SEP,  OpCodeTypeEnum::REGISTER,                      CPUTypeEnum::CPU_1802  }},
    { "SEX",  { SEX,  OpCodeTypeEnum::REGISTER,                      CPUTypeEnum::CPU_1802  }},
    { "LDX",  { LDX,  OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "OR",   { OR,   OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "AND",  { AND,  OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "XOR",  { XOR,  OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "ADD",  { ADD,  OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "SD",   { SD,   OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "SHR",  { SHR,  OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "SM",   { SM,   OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "LDI",  { LDI,  OpCodeTypeEnum::IMMEDIATE,                     CPUTypeEnum::CPU_1802  }},
    { "ORI",  { ORI,  OpCodeTypeEnum::IMMEDIATE,                     CPUTypeEnum::CPU_1802  }},
    { "ANI",  { ANI,  OpCodeTypeEnum::IMMEDIATE,                     CPUTypeEnum::CPU_1802  }},
    { "XRI",  { XRI,  OpCodeTypeEnum::IMMEDIATE,                     CPUTypeEnum::CPU_1802  }},
    { "ADI",  { ADI,  OpCodeTypeEnum::IMMEDIATE,                     CPUTypeEnum::CPU_1802  }},
    { "SDI",  { SDI,  OpCodeTypeEnum::IMMEDIATE,                     CPUTypeEnum::CPU_1802  }},
    { "SHL",  { SHL,  OpCodeTypeEnum::BASIC,                         CPUTypeEnum::CPU_1802  }},
    { "SMI",  { SMI,  OpCodeTypeEnum::IMMEDIATE,                     CPUTypeEnum::CPU_1802  }},

    // 1806 Additions

    { "STPC", { STPC, OpCodeTypeEnum::EXTENDED,                      CPUTypeEnum::CPU_1806  }},
    { "DTC",  { DTC,  OpCodeTypeEnum::EXTENDED,                      CPUTypeEnum::CPU_1806  }},
    { "SPM2", { SPM2, OpCodeTypeEnum::EXTENDED,                      CPUTypeEnum::CPU_1806  }},
    { "SCM2", { SCM2, OpCodeTypeEnum::EXTENDED,                      CPUTypeEnum::CPU_1806  }},
    { "SPM1", { SPM1, OpCodeTypeEnum::EXTENDED,                      CPUTypeEnum::CPU_1806  }},
    { "SCM1", { SCM1, OpCodeTypeEnum::EXTENDED,                      CPUTypeEnum::CPU_1806  }},
    { "LDC",  { LDC,  OpCodeTypeEnum::EXTENDED,                      CPUTypeEnum::CPU_1806  }},
    { "STM",  { STM,  OpCodeTypeEnum::EXTENDED,                      CPUTypeEnum::CPU_1806  }},
    { "GEX",  { GEC,  OpCodeTypeEnum::EXTENDED,                      CPUTypeEnum::CPU_1806  }},
    { "ETQ",  { ETQ,  OpCodeTypeEnum::EXTENDED,                      CPUTypeEnum::CPU_1806  }},
    { "XIE",  { XIE,  OpCodeTypeEnum::EXTENDED,                      CPUTypeEnum::CPU_1806  }},
    { "XID",  { XID,  OpCodeTypeEnum::EXTENDED,                      CPUTypeEnum::CPU_1806  }},
    { "CIE",  { CIE,  OpCodeTypeEnum::EXTENDED,                      CPUTypeEnum::CPU_1806  }},
    { "CID",  { CID,  OpCodeTypeEnum::EXTENDED,                      CPUTypeEnum::CPU_1806  }},
    { "BCI",  { BCI,  OpCodeTypeEnum::EXTENDED_SHORT_BRANCH,         CPUTypeEnum::CPU_1806  }},
    { "BXI",  { BXI,  OpCodeTypeEnum::EXTENDED_SHORT_BRANCH,         CPUTypeEnum::CPU_1806  }},
    { "RLXA", { RLXA, OpCodeTypeEnum::EXTENDED_REGISTER,             CPUTypeEnum::CPU_1806  }},
    { "SCAL", { SCAL, OpCodeTypeEnum::EXTENDED_REGISTER_IMMEDIATE16, CPUTypeEnum::CPU_1806  }},
    { "SRET", { SRET, OpCodeTypeEnum::EXTENDED_REGISTER,             CPUTypeEnum::CPU_1806  }},
    { "RSXD", { RSXD, OpCodeTypeEnum::EXTENDED_REGISTER,             CPUTypeEnum::CPU_1806  }},
    { "RNX",  { RNX,  OpCodeTypeEnum::EXTENDED_REGISTER,             CPUTypeEnum::CPU_1806  }},
    { "RLDI", { RLDI, OpCodeTypeEnum::EXTENDED_REGISTER_IMMEDIATE16, CPUTypeEnum::CPU_1806  }},

    // 1806A Additions

    { "DBNZ", { DBNZ, OpCodeTypeEnum::EXTENDED_REGISTER_IMMEDIATE16, CPUTypeEnum::CPU_1806A }},
    { "DADC", { DADC, OpCodeTypeEnum::EXTENDED,                      CPUTypeEnum::CPU_1806A }},
    { "DSAV", { DSAV, OpCodeTypeEnum::EXTENDED,                      CPUTypeEnum::CPU_1806A }},
    { "DSMB", { DSMB, OpCodeTypeEnum::EXTENDED,                      CPUTypeEnum::CPU_1806A }},
    { "DACI", { DACI, OpCodeTypeEnum::EXTENDED_IMMEDIATE,            CPUTypeEnum::CPU_1806A }},
    { "DSBI", { DSBI, OpCodeTypeEnum::EXTENDED_IMMEDIATE,            CPUTypeEnum::CPU_1806A }},
    { "DADD", { DADD, OpCodeTypeEnum::EXTENDED,                      CPUTypeEnum::CPU_1806A }},
    { "DSM",  { DSM,  OpCodeTypeEnum::EXTENDED,                      CPUTypeEnum::CPU_1806A }},
    { "DADI", { DADI, OpCodeTypeEnum::EXTENDED_IMMEDIATE,            CPUTypeEnum::CPU_1806A }},
    { "DSMI", { DSMI, OpCodeTypeEnum::EXTENDED_IMMEDIATE,            CPUTypeEnum::CPU_1806A }},

    // Pseudo Operations

    { "EQU",        { EQU,       OpCodeTypeEnum::PSEUDO_OP,             CPUTypeEnum::CPU_1802  }},
    { "SUB",        { SUB,       OpCodeTypeEnum::PSEUDO_OP,             CPUTypeEnum::CPU_1802  }},
    { "SUBROUTINE", { SUB,       OpCodeTypeEnum::PSEUDO_OP,             CPUTypeEnum::CPU_1802  }},
    { "ENDSUB",     { ENDSUB,    OpCodeTypeEnum::PSEUDO_OP,             CPUTypeEnum::CPU_1802  }},
    { "ORG",        { ORG,       OpCodeTypeEnum::PSEUDO_OP,             CPUTypeEnum::CPU_1802  }},
    { "RORG",       { RORG,      OpCodeTypeEnum::PSEUDO_OP,             CPUTypeEnum::CPU_1802  }},
    { "REND",       { REND,      OpCodeTypeEnum::PSEUDO_OP,             CPUTypeEnum::CPU_1802  }},
    { "DB",         { DB,        OpCodeTypeEnum::PSEUDO_OP,             CPUTypeEnum::CPU_1802  }},
    { "DW",         { DW,        OpCodeTypeEnum::PSEUDO_OP,             CPUTypeEnum::CPU_1802  }},
    { "DL",         { DL,        OpCodeTypeEnum::PSEUDO_OP,             CPUTypeEnum::CPU_1802  }},
    { "ALIGN",      { ALIGN,     OpCodeTypeEnum::PSEUDO_OP,             CPUTypeEnum::CPU_1802  }},
    { "ASSERT",     { ASSERT,    OpCodeTypeEnum::PSEUDO_OP,             CPUTypeEnum::CPU_1802  }},
    { "MACRO",      { MACRO,     OpCodeTypeEnum::PSEUDO_OP,             CPUTypeEnum::CPU_1802  }},
    { "ENDMACRO",   { ENDMACRO,  OpCodeTypeEnum::PSEUDO_OP,             CPUTypeEnum::CPU_1802  }},
    { "ENDM",       { ENDMACRO,  OpCodeTypeEnum::PSEUDO_OP,             CPUTypeEnum::CPU_1802  }},
    { "END",        { END,       OpCodeTypeEnum::PSEUDO_OP,             CPUTypeEnum::CPU_1802  }}
};

const std::map<OpCodeTypeEnum, int> OpCodeTable::OpCodeBytes =
{
    { OpCodeTypeEnum::BASIC,                          1 },
    { OpCodeTypeEnum::REGISTER,                       1 },
    { OpCodeTypeEnum::IMMEDIATE,                      2 },
    { OpCodeTypeEnum::SHORT_BRANCH,                   2 },
    { OpCodeTypeEnum::LONG_BRANCH,                    3 },
    { OpCodeTypeEnum::INPUT_OUTPUT,                   1 },
    { OpCodeTypeEnum::EXTENDED,                       2 },
    { OpCodeTypeEnum::EXTENDED_REGISTER,              2 },
    { OpCodeTypeEnum::EXTENDED_IMMEDIATE,             3 },
    { OpCodeTypeEnum::EXTENDED_SHORT_BRANCH,          3 },
    { OpCodeTypeEnum::EXTENDED_REGISTER_IMMEDIATE16,  4 }
};

const std::map<std::string, CPUTypeEnum> OpCodeTable::CPUTable =
{
    { "1802",     CPUTypeEnum::CPU_1802  },
    { "1804",     CPUTypeEnum::CPU_1806  },
    { "1805",     CPUTypeEnum::CPU_1806  },
    { "1806",     CPUTypeEnum::CPU_1806  },
    { "1804A",    CPUTypeEnum::CPU_1806A },
    { "1805A",    CPUTypeEnum::CPU_1806A },
    { "1806A",    CPUTypeEnum::CPU_1806A },
    { "CDP1802",  CPUTypeEnum::CPU_1802  },
    { "CDP1804",  CPUTypeEnum::CPU_1806  },
    { "CDP1805",  CPUTypeEnum::CPU_1806  },
    { "CDP1806",  CPUTypeEnum::CPU_1806  },
    { "CDP1804A", CPUTypeEnum::CPU_1806A },
    { "CDP1805A", CPUTypeEnum::CPU_1806A },
    { "CDP1806A", CPUTypeEnum::CPU_1806A }
};
