//
//  mix.h
//  mix-simulator
//
//  Created by Chris on 12/26/15.
//  Copyright © 2015 Chris. All rights reserved.
//

#ifndef mix_h
#define mix_h

#include <cstdlib>
#include <string>

struct bad_opcode { std::string what;
    bad_opcode(std::string s) : what(s) {}
};
class halted_exception {};
class address_violation {};
class memory_access_violation : address_violation {};

constexpr int NUMBASE=64;
constexpr auto ADDR_CAP = 4000;

typedef unsigned char MIXByte;
typedef signed char SignedMIXByte;

struct TrueMIXWord {
    unsigned sign : 1;
    unsigned b4 : 6;
    unsigned b3 : 6;
    unsigned b2 : 6;
    unsigned b1 : 6;
    unsigned b0 : 6;
};

struct InstrMIXWord {
    unsigned sign  : 1;
    unsigned addr  : 12;
    unsigned field : 6;
    unsigned index : 6;
    unsigned opcode: 6;
};


struct MIXWord;
struct MIXAddr {
    SignedMIXByte sign;
    MIXByte byte[2];
    
    explicit MIXAddr(int i) :sign(1) {
        if (i<0) {
            sign = -1;
            i = std::abs(i);
        }
        byte[0] = i/NUMBASE;
        byte[1] = i%NUMBASE;
    }
    MIXAddr(const MIXAddr& other) = default; // bitwise copy works
    MIXAddr() :sign(1), byte{0,0} {}
    explicit MIXAddr(const MIXWord& other) ;
    int decode() const {
        return sign*NUMBASE*byte[0]+byte[1]; }
};


enum Opcode : unsigned char { // opcodes
    NOP = 0,
    ADD = 1,
    SUB = 2,
    MUL = 3,
    DIV = 4,
    NUM = 5,
    CHAR = NUM,
    HLT = NUM,
    SLA,
    SRA = SLA,
    SLAX = SLA,
    SRAX = SLA,
    SLC = SLA,
    SRC = SLA,
    MOVE=7,
    LDA=8, LD1, LD2, LD3, LD4, LD5, LD6, LDX,
    LDAN=16, LD1N, LD2N, LD3N, LD4N, LD5N, LD6N, LDXN,
    STA=24, ST1, ST2, ST3, ST4, ST5, ST6, STX,
    STJ=32,
    STZ,
    JBUS,
    IOC,
    IN,
    OUT,
    JRED,
    JMP,
    JSJ = JMP,
    JOV = JMP,
    JNOV = JMP,
    JL = JMP,
    JE = JMP,
    JG = JMP,
    JGE = JMP,
    JNE = JMP,
    JLE = JMP,
    JAN,
    JAZ = JAN,
    JAP = JAN,
    JANN = JAN,
    JANZ = JAN,
    JANP = JAN,
    
    J1N,
    J1Z = J1N,
    J1P = J1N,
    J1NN = J1N,
    J1NZ = J1N,
    J1NP = J1N,
    
    J2N,
    J2Z = J2N,
    J2P = J2N,
    J2NN = J2N,
    J2NZ = J2N,
    J2NP = J2N,
    
    
    J3N,
    J3Z = J3N,
    J3P = J3N,
    J3NN = J3N,
    J3NZ = J3N,
    J3NP = J3N,
    
    
    J4N,
    J4Z = J4N,
    J4P = J4N,
    J4NN = J4N,
    J4NZ = J4N,
    J4NP = J4N,
    
    
    J5N,
    J5Z = J5N,
    J5P = J5N,
    J5NN = J5N,
    J5NZ = J5N,
    J5NP = J5N,
    
    
    J6N,
    J6Z = J6N,
    J6P = J6N,
    J6NN = J6N,
    J6NZ = J6N,
    J6NP = J6N,
    
    
    JXN,
    JXZ = JXN,
    JXP = JXN,
    JXNN = JXN,
    JXNZ = JXN,
    JXNP = JXN,
    
    INCA,
    DECA=INCA,
    ENTA=INCA,
    ENNA=INCA,
    
    INC1,
    DEC1=INC1,
    ENT1=INC1,
    ENN1=INC1,
    
    INC2,
    DEC2=INC2,
    ENT2=INC2,
    ENN2=INC2,
    
    
    INC3,
    DEC3=INC3,
    ENT3=INC3,
    ENN3=INC3,
    
    
    INC4,
    DEC4=INC4,
    ENT4=INC4,
    ENN4=INC4,
    
    
    INC5,
    DEC5=INC5,
    ENT5=INC5,
    ENN5=INC5,
    
    
    INC6,
    DEC6=INC6,
    ENT6=INC6,
    ENN6=INC6,
    
    
    INCX,
    DECX=INCX,
    ENTX=INCX,
    ENNX=INCX,
    
    CMPA,
    CMP1,
    CMP2,
    CMP3,
    CMP4,
    CMP5,
    CMP6,
    CMPX
};


struct MIXWord {
    SignedMIXByte sign;
    MIXByte byte[5];
    
    MIXWord():sign(1) { byte[0] = byte[1] = byte[2] = byte[3] = byte[4] = 0;}
    ~MIXWord() = default;
    MIXWord(MIXAddr addr, MIXByte index, MIXByte field, Opcode oc) :sign(addr.sign)
    { byte[0] = addr.byte[0]; byte[1] = addr.byte[1];
        byte[2] = index; byte[3]= field; byte[4]=oc;}
    MIXWord(const MIXWord&)= default;
    explicit MIXWord(int i);
    int decode(int lo=0,int hi=5) const;
    MIXWord& operator=(const MIXWord &other) = default;
};

inline MIXAddr::MIXAddr(const MIXWord& other)
:sign(other.sign)
{
    byte[0]=other.byte[0]; byte[1]=other.byte[1];
}

typedef void (*MIXOp)(MIXAddr addr, MIXByte index, MIXByte field, Opcode oc);//, MIXByte index, MIXByte field, Opcode op);

extern MIXOp opTable[64];

extern MIXWord AReg; //= {0};
extern MIXAddr IReg[7]; //= {0};
extern MIXWord XReg;// = {0};
extern MIXAddr JReg;// = {0};
extern const MIXAddr ZReg;// = {0};

extern MIXWord Memory[4000];

extern signed char compIndicator;
extern bool overflowToggle;

extern int programCounter;

std::ostream &operator <<(std::ostream &os, MIXWord& w);

#endif /* mix_h */
