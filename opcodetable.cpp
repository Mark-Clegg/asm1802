#include "opcodetable.h"

const std::map<std::string, OpCodeSpec> OpCodeTable::OpCode = {
    { "IDL",  { IDL,  BASIC,                         CPU_1802  }},
    { "LDN",  { LDN,  REGISTER,                      CPU_1802  }},
    { "INC",  { INC,  REGISTER,                      CPU_1802  }},
    { "DEC",  { DEC,  REGISTER,                      CPU_1802  }},
    { "BR",   { BR,   SHORT_BRANCH,                  CPU_1802  }},
    { "BQ",   { BQ,   SHORT_BRANCH,                  CPU_1802  }},
    { "BZ",   { BZ,   SHORT_BRANCH,                  CPU_1802  }},
    { "BDF",  { BDF,  SHORT_BRANCH,                  CPU_1802  }},
    { "BPZ",  { BPZ,  SHORT_BRANCH,                  CPU_1802  }},
    { "BGE",  { BGE,  SHORT_BRANCH,                  CPU_1802  }},
    { "B1",   { B1,   SHORT_BRANCH,                  CPU_1802  }},
    { "B2",   { B2,   SHORT_BRANCH,                  CPU_1802  }},
    { "B3",   { B3,   SHORT_BRANCH,                  CPU_1802  }},
    { "B4",   { B4,   SHORT_BRANCH,                  CPU_1802  }},
    { "NBR",  { NBR,  BASIC,                         CPU_1802  }},
    { "SKP",  { SKP,  BASIC,                         CPU_1802  }},
    { "BNQ",  { BNQ,  SHORT_BRANCH,                  CPU_1802  }},
    { "BNZ",  { BNZ,  SHORT_BRANCH,                  CPU_1802  }},
    { "BNF",  { BNF,  SHORT_BRANCH,                  CPU_1802  }},
    { "BM",   { BM,   SHORT_BRANCH,                  CPU_1802  }},
    { "BL",   { BL,   SHORT_BRANCH,                  CPU_1802  }},
    { "BN1",  { BN1,  SHORT_BRANCH,                  CPU_1802  }},
    { "BN2",  { BN2,  SHORT_BRANCH,                  CPU_1802  }},
    { "BN3",  { BN3,  SHORT_BRANCH,                  CPU_1802  }},
    { "BN4",  { BN4,  SHORT_BRANCH,                  CPU_1802  }},
    { "LDA",  { LDA,  REGISTER,                      CPU_1802  }},
    { "STR",  { STR,  REGISTER,                      CPU_1802  }},
    { "IRX",  { IRX,  BASIC,                         CPU_1802  }},
    { "OUT",  { OUT,  INPUT_OUTPUT,                  CPU_1802  }},
    { "INP",  { INP,  INPUT_OUTPUT,                  CPU_1802  }},
    { "RET",  { RET,  BASIC,                         CPU_1802  }},
    { "DIS",  { DIS,  BASIC,                         CPU_1802  }},
    { "LDXA", { LDXA, BASIC,                         CPU_1802  }},
    { "STXD", { STXD, BASIC,                         CPU_1802  }},
    { "ADC",  { ADC,  BASIC,                         CPU_1802  }},
    { "SDB",  { SDB,  BASIC,                         CPU_1802  }},
    { "SHRC", { SHRC, BASIC,                         CPU_1802  }},
    { "RSHR", { RSHR, BASIC,                         CPU_1802  }},
    { "SMB",  { SMB,  BASIC,                         CPU_1802  }},
    { "SAV",  { SAV,  BASIC,                         CPU_1802  }},
    { "MARK", { MARK, BASIC,                         CPU_1802  }},
    { "REQ",  { REQ,  BASIC,                         CPU_1802  }},
    { "SEQ",  { SEQ,  BASIC,                         CPU_1802  }},
    { "ADCI", { ADCI, IMMEDIATE,                     CPU_1802  }},
    { "SDBI", { SDBI, IMMEDIATE,                     CPU_1802  }},
    { "SHLC", { SHLC, BASIC,                         CPU_1802  }},
    { "RSHL", { RSHL, BASIC,                         CPU_1802  }},
    { "SMBI", { SMBI, IMMEDIATE,                     CPU_1802  }},
    { "GLO",  { GLO,  REGISTER,                      CPU_1802  }},
    { "GHI",  { GHI,  REGISTER,                      CPU_1802  }},
    { "PLO",  { PLO,  REGISTER,                      CPU_1802  }},
    { "PHI",  { PHI,  REGISTER,                      CPU_1802  }},
    { "LBR",  { LBR,  LONG_BRANCH,                   CPU_1802  }},
    { "LBQ",  { LBQ,  LONG_BRANCH,                   CPU_1802  }},
    { "LBZ",  { LBZ,  LONG_BRANCH,                   CPU_1802  }},
    { "LBDF", { LBDF, LONG_BRANCH,                   CPU_1802  }},
    { "NOP",  { NOP,  BASIC,                         CPU_1802  }},
    { "LSNQ", { LSNQ, BASIC,                         CPU_1802  }},
    { "LSNZ", { LSNZ, BASIC,                         CPU_1802  }},
    { "LSNF", { LSNF, BASIC,                         CPU_1802  }},
    { "LSKP", { LSKP, BASIC,                         CPU_1802  }},
    { "NLBR", { NLBR, BASIC,                         CPU_1802  }},
    { "LBNQ", { LBNQ, LONG_BRANCH,                   CPU_1802  }},
    { "LBNZ", { LBNZ, LONG_BRANCH,                   CPU_1802  }},
    { "LBNF", { LBNF, LONG_BRANCH,                   CPU_1802  }},
    { "LSIE", { LSIE, BASIC,                         CPU_1802  }},
    { "LSQ",  { LSQ,  BASIC,                         CPU_1802  }},
    { "LSZ",  { LSZ,  BASIC,                         CPU_1802  }},
    { "LSDF", { LSDF, BASIC,                         CPU_1802  }},
    { "SEP",  { SEP,  REGISTER,                      CPU_1802  }},
    { "SEX",  { SEX,  REGISTER,                      CPU_1802  }},
    { "LDX",  { LDX,  BASIC,                         CPU_1802  }},
    { "OR",   { OR,   BASIC,                         CPU_1802  }},
    { "AND",  { AND,  BASIC,                         CPU_1802  }},
    { "XOR",  { XOR,  BASIC,                         CPU_1802  }},
    { "ADD",  { ADD,  BASIC,                         CPU_1802  }},
    { "SD",   { SD,   BASIC,                         CPU_1802  }},
    { "SHR",  { SHR,  BASIC,                         CPU_1802  }},
    { "SM",   { SM,   BASIC,                         CPU_1802  }},
    { "LDI",  { LDI,  IMMEDIATE,                     CPU_1802  }},
    { "ORI",  { ORI,  IMMEDIATE,                     CPU_1802  }},
    { "ANI",  { ANI,  IMMEDIATE,                     CPU_1802  }},
    { "XRI",  { XRI,  IMMEDIATE,                     CPU_1802  }},
    { "ADI",  { ADI,  IMMEDIATE,                     CPU_1802  }},
    { "SDI",  { SDI,  IMMEDIATE,                     CPU_1802  }},
    { "SHL",  { SHL,  BASIC,                         CPU_1802  }},
    { "SMI",  { SMI,  IMMEDIATE,                     CPU_1802  }},

    // 1806 Additions

    { "STPC", { STPC, EXTENDED,                      CPU_1806  }},
    { "DTC",  { DTC,  EXTENDED,                      CPU_1806  }},
    { "SPM2", { SPM2, EXTENDED,                      CPU_1806  }},
    { "SCM2", { SCM2, EXTENDED,                      CPU_1806  }},
    { "SPM1", { SPM1, EXTENDED,                      CPU_1806  }},
    { "SCM1", { SCM1, EXTENDED,                      CPU_1806  }},
    { "LDC",  { LDC,  EXTENDED,                      CPU_1806  }},
    { "STM",  { STM,  EXTENDED,                      CPU_1806  }},
    { "GEX",  { GEC,  EXTENDED,                      CPU_1806  }},
    { "ETQ",  { ETQ,  EXTENDED,                      CPU_1806  }},
    { "XIE",  { XIE,  EXTENDED,                      CPU_1806  }},
    { "XID",  { XID,  EXTENDED,                      CPU_1806  }},
    { "CIE",  { CIE,  EXTENDED,                      CPU_1806  }},
    { "CID",  { CID,  EXTENDED,                      CPU_1806  }},
    { "BCI",  { BCI,  EXTENDED_SHORT_BRANCH,         CPU_1806  }},
    { "BXI",  { BXI,  EXTENDED_SHORT_BRANCH,         CPU_1806  }},
    { "RLXA", { RLXA, EXTENDED_REGISTER,             CPU_1806  }},
    { "SCAL", { SCAL, EXTENDED_REGISTER_IMMEDIATE16, CPU_1806  }},
    { "SRET", { SRET, EXTENDED_REGISTER,             CPU_1806  }},
    { "RSXD", { RSXD, EXTENDED_REGISTER,             CPU_1806  }},
    { "RNX",  { RNX,  EXTENDED_REGISTER,             CPU_1806  }},
    { "RLDI", { RLDI, EXTENDED_REGISTER_IMMEDIATE16, CPU_1806  }},

    // 1806A Additions

    { "DBNZ", { DBNZ, EXTENDED_REGISTER_IMMEDIATE16, CPU_1806A }},
    { "DADC", { DADC, EXTENDED,                      CPU_1806A }},
    { "DSAV", { DSAV, EXTENDED,                      CPU_1806A }},
    { "DSMB", { DSMB, EXTENDED,                      CPU_1806A }},
    { "DACI", { DACI, EXTENDED_IMMEDIATE,            CPU_1806A }},
    { "DSBI", { DSBI, EXTENDED_IMMEDIATE,            CPU_1806A }},
    { "DADD", { DADD, EXTENDED,                      CPU_1806A }},
    { "DSM",  { DSM,  EXTENDED,                      CPU_1806A }},
    { "DADI", { DADI, EXTENDED_IMMEDIATE,            CPU_1806A }},
    { "DSMI", { DSMI, EXTENDED_IMMEDIATE,            CPU_1806A }},

    // Pseudo Operations

    { "EQU",        { EQU,       PSEUDO_OP,             CPU_1802  }},
    { "SUB",        { SUB,       PSEUDO_OP,             CPU_1802  }},
    { "SUBROUTINE", { SUB,       PSEUDO_OP,             CPU_1802  }},
    { "ENDSUB",     { ENDSUB,    PSEUDO_OP,             CPU_1802  }},
    { "ORG",        { ORG,       PSEUDO_OP,             CPU_1802  }},
    { "DB",         { DB,        PSEUDO_OP,             CPU_1802  }},
    { "DW",         { DW,        PSEUDO_OP,             CPU_1802  }},
    { "CPU",        { PROCESSOR, PSEUDO_OP,             CPU_1802  }},
    { "PROCESSOR",  { PROCESSOR, PSEUDO_OP,             CPU_1802  }},
    { "ALIGN",      { ALIGN,     PSEUDO_OP,             CPU_1802  }},
    { "ASSERT",     { ASSERT,    PSEUDO_OP,             CPU_1802  }},
    { "MACRO",      { MACRO,     PSEUDO_OP,             CPU_1802  }},
    { "ENDMACRO",   { ENDMACRO,  PSEUDO_OP,             CPU_1802  }},
    { "ENDM",       { ENDMACRO,  PSEUDO_OP,             CPU_1802  }},
    { "END",        { END,       PSEUDO_OP,             CPU_1802  }}
};

const std::map<OpCodeTypeEnum, int> OpCodeTable::OpCodeBytes = {
    { BASIC,                          1 },
    { REGISTER,                       1 },
    { IMMEDIATE,                      2 },
    { SHORT_BRANCH,                   2 },
    { LONG_BRANCH,                    3 },
    { INPUT_OUTPUT,                   1 },
    { EXTENDED,                       2 },
    { EXTENDED_REGISTER,              2 },
    { EXTENDED_IMMEDIATE,             3 },
    { EXTENDED_SHORT_BRANCH,          3 },
    { EXTENDED_REGISTER_IMMEDIATE16,  4 }
};

const std::map<std::string, CPUTypeEnum> OpCodeTable::CPUTable = {
    { "1802",     CPU_1802  },
    { "1804",     CPU_1806  },
    { "1805",     CPU_1806  },
    { "1806",     CPU_1806  },
    { "1804A",    CPU_1806A },
    { "1805A",    CPU_1806A },
    { "1806A",    CPU_1806A },
    { "CDP1802",  CPU_1802  },
    { "CDP1804",  CPU_1806  },
    { "CDP1805",  CPU_1806  },
    { "CDP1806",  CPU_1806  },
    { "CDP1804A", CPU_1806A },
    { "CDP1805A", CPU_1806A },
    { "CDP1806A", CPU_1806A }
};

//OpCodeSpec::OpCodeSpec()
//{
//}

//OpCodeSpec::OpCodeSpec(OpCodeEnum OpCode, OpCodeTypeEnum OpCodeType, CPUTypeEnum CPUType)
//{
//    this->OpCode = OpCode;
//    this->OpCodeType = OpCodeType;
//    this->CPUType = CPUType;
//}

//OpCodeSpec::OpCodeSpec(OpCodeEnum OpCode, std::string& Mnemonic, OpCodeTypeEnum OpCodeType, CPUTypeEnum CPUType)
//{
//    this->OpCode = OpCode;
//    this->Mnemonic = Mnemonic;
//    this->OpCodeType = OpCodeType;
//    this->CPUType = CPUType;
//}
