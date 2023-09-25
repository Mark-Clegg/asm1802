#ifndef OPCODE_H
#define OPCODE_H

#include <map>
#include <string>

enum OpCodeEnum
{
    IDL  = 0x00,
    LDN  = 0x00,
    INC  = 0x10,
    DEC  = 0x20,
    BR   = 0x30,
    BQ   = 0x31,
    BZ   = 0x32,
    BDF  = 0x33,
    BPZ  = 0x33,
    BGE  = 0x33,
    B1   = 0x34,
    B2   = 0x35,
    B3   = 0x36,
    B4   = 0x37,
    NBR  = 0x38,
    SKP  = 0x38,
    BNQ  = 0x39,
    BNZ  = 0x3A,
    BNF  = 0x3B,
    BM   = 0x3B,
    BL   = 0x3B,
    BN1  = 0x3C,
    BN2  = 0x3D,
    BN3  = 0x3E,
    BN4  = 0x3F,
    LDA  = 0x40,
    STR  = 0x50,
    IRX  = 0x60,
    OUT  = 0x60,
    INP  = 0x68,
    RET  = 0x70,
    DIS  = 0x71,
    LDXA = 0x72,
    STXD = 0x73,
    ADC  = 0x74,
    SDB  = 0x75,
    SHRC = 0x76,
    RSHR = 0x76,
    SMB  = 0x77,
    SAV  = 0x78,
    MARK = 0x79,
    REQ  = 0x7A,
    SEQ  = 0x7B,
    ADCI = 0x7C,
    SDBI = 0x7D,
    SHLC = 0x7E,
    RSHL = 0x7E,
    SMBI = 0x7F,
    GLO  = 0x80,
    GHI  = 0x90,
    PLO  = 0xA0,
    PHI  = 0xB0,
    LBR  = 0xC0,
    LBQ  = 0xC1,
    LBZ  = 0xC2,
    LBDF = 0xC3,
    NOP  = 0xC4,
    LSNQ = 0xC5,
    LSNZ = 0xC6,
    LSNF = 0xC7,
    LSKP = 0xC8,
    NLBR = 0xC9,
    LBNQ = 0xC9,
    LBNZ = 0xCA,
    LBNF = 0xCB,
    LSIE = 0xCC,
    LSQ  = 0xCD,
    LSZ  = 0xCE,
    LSDF = 0xCF,
    SEP  = 0xD0,
    SEX  = 0xE0,
    LDX  = 0xF0,
    OR   = 0xF1,
    AND  = 0xF2,
    XOR  = 0xF3,
    ADD  = 0xF4,
    SD   = 0xF5,
    SHR  = 0xF6,
    SM   = 0xF7,
    LDI  = 0xF8,
    ORI  = 0xF9,
    ANI  = 0xFA,
    XRI  = 0xFB,
    ADI  = 0xFC,
    SDI  = 0xFD,
    SHL  = 0xFE,
    SMI  = 0xFF,

    // 1806 Additions

    STPC = 0x6800,
    DTC  = 0x6801,
    SPM2 = 0x6802,
    SCM2 = 0x6803,
    SPM1 = 0x6804,
    SCM1 = 0x6805,
    LDC  = 0x6806,
    STM  = 0x6807,
    GEC  = 0x6808,
    ETQ  = 0x6809,
    XIE  = 0x680A,
    XID  = 0x680B,
    CIE  = 0x680C,
    CID  = 0x680D,
    BCI  = 0x683E,
    BXI  = 0x683F,
    RLXA = 0x6860,
    SCAL = 0x6880,
    SRET = 0x6890,
    RSXD = 0x68A0,
    RNX  = 0x68B0,
    RLDI = 0x68C0,

    // 1806A Additions

    DBNZ = 0x6820,
    DADC = 0x6874,
    DSAV = 0x6876,
    DSMB = 0x6877,
    DACI = 0x687C,
    DSBI = 0x687F,
    DADD = 0x68F4,
    DSM  = 0x68F7,
    DADI = 0x68FC,
    DSMI = 0x68FF,

    // Pseudo OpCodes

    SUB    = 0xFF02,
    ENDSUB = 0xFF03,
    ORG    = 0xFF04
};

enum OpCodeTypeEnum
{
    BASIC,                          // OPCODE
    REGISTER,                       // OPCODE Rn (n=0-F)
    IMMEDIATE,                      // OPCODE 0xnn
    SHORT_BRANCH,                   // OPCODE 0xnn (must be in same page)
    LONG_BRANCH,                    // OPCODE 0xnnnn
    INPUT_OUTPUT,                   // OPCODE Pn (n=1-7)
    EXTENDED,                       // OPCODE
    EXTENDED_REGISTER,              // OPCODE Rn (n=0-F)
    EXTENDED_IMMEDIATE,             // OPCODE 0xnn
    EXTENDED_SHORT_BRANCH,          // OPCODE 0xnn (must be in same page)
    EXTENDED_REGISTER_IMMEDIATE16,  // OPCODE Rn, 0xnnnn
    PSEUDO_OP                       // Pseudo Operation
};

enum CPUTypeEnum
{
    CPU_1802,
    CPU_1806,
    CPU_1806A
};

class OpCodeSpec
{
public:
    OpCodeEnum OpCode;
    OpCodeTypeEnum OpCodeType;
    CPUTypeEnum CPUType;
};

class OpCodeTable
{
public:
    static const std::map<std::string, OpCodeSpec> OpCode;
    static const std::map<OpCodeTypeEnum, int> OpCodeBytes;
};

#endif // OPCODE_H
