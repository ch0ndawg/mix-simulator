//
//  mixop-table.cpp
//  mix-simulator
//
//  Created by Chris on 12/26/15.
//  Copyright © 2015 Chris. All rights reserved.
//
#include "mix.h"
#include "mixop-table.hpp"
#include <cstdlib>
#include <cassert>
#include <sstream>
#include <iostream>

typedef long int LongInt;
constexpr LongInt WORDBASE = 1073741824L;

// arrays of void pointers (because the registers are different) for indexing into
// for load/store operations. Load operations use them as mutable registers, while
// store operations can use immutable registers.

const void *registers[] = {&AReg, &IReg[1],&IReg[2],&IReg[3],
    &IReg[4], &IReg[5], &IReg[6], &XReg, &JReg, &ZReg} ;

void *mutable_registers[] = {&AReg, &IReg[1],&IReg[2],&IReg[3],
    &IReg[4], &IReg[5], &IReg[6], &XReg} ;


// the no op ignores everything
void nop(MIXAddr addr, MIXByte index, MIXByte field, Opcode oc)
{ }

// sentinel function for instructions not yet implemented
void nullfunc(MIXAddr addr, MIXByte index, MIXByte field, Opcode oc)
{
    std::ostringstream s;
    // output the program counter and (decoded) instruction fields
    s << "Opcode " << static_cast<int>(oc) << " not yet implemented.\n"
    "This occurred at memory location " << programCounter << "\n"
    "The address field of the instruction was " << addr.decode() << ".\n"
    "And the index and field: " << static_cast<int>(index) << ' '
    << static_cast<int>(field) << "\n";
    throw bad_opcode(s.str());
}

// ADD instruction
void add(MIXAddr addr, MIXByte index, MIXByte field, Opcode oc)
{
    // field specifications
    int lo = field/8;
    int hi = field%8;
    
    // calculate address plus index register
    int newAddr = addr.decode()+ IReg[index].decode();
    
    // fetch the contents
    int val = Memory[newAddr].decode(lo,hi);
    
    // fetch the A register
    int aReg = AReg.decode();
    
    aReg += val; // increment the accumulator
    
    // check that it does not overflow (set the toggle if so)
    if (abs(aReg) >= WORDBASE) overflowToggle = true;
    else overflowToggle = false;
    AReg = MIXWord(aReg); // store result in the A register.
}

// implement it as addition of the negative
void sub(MIXAddr addr, MIXByte index, MIXByte field, Opcode oc)
{
    AReg.sign = -AReg.sign; // negate
    add(addr,index,field,oc); // add
    AReg.sign = -AReg.sign; // negate again.
}

void mul(MIXAddr addr, MIXByte index, MIXByte field, Opcode oc)
{
    // NOTE: may need to adjust if long ints are not large enough
    // other method is to use the Cauchy product.
    int lo = field/8;
    int hi = field%8;
    int newAddr = addr.decode() + IReg[index].decode();
    LongInt val = std::abs(Memory[newAddr].decode(lo,hi)); // full decoding and promotion
    signed char valsgn =  (lo > 0 ? 1 : Memory[newAddr].sign); // if field includes sign or not
    LongInt aReg = std::abs(AReg.decode());
    signed char sgn = AReg.sign;
    
    LongInt result = val*aReg;
    
    if (result >= WORDBASE*WORDBASE) {
        overflowToggle = true; // trip the overflow
    } else {
        overflowToggle = false;
    }
    // 10-byte product in rAX (A gets hi word, X gets lo word)
    int hiWord = static_cast<int>(result/WORDBASE);
    int loWord = result % WORDBASE;
    
    AReg = MIXWord(hiWord);
    XReg = MIXWord(loWord);
    XReg.sign = AReg.sign = sgn*valsgn;
    
}

// integer division. register rA and rX combine to form a 10-byte product.
// divide out by the (field of the) addressed value
void div(MIXAddr addr, MIXByte index, MIXByte field, Opcode oc)
{
    int lo = field/8;
    int hi = field%8;

    int newAddr = addr.decode()+ IReg[index].decode();
    int val =std::abs(Memory[newAddr].decode(lo,hi)); // full decoding and promotion
    signed char valsgn = (lo > 0 ? 1 : Memory[newAddr].sign);
    LongInt aReg = std::abs(AReg.decode());
    LongInt xReg = std::abs(XReg.decode());
    LongInt dividend = WORDBASE*aReg +xReg;
    
    if (val!=0) {
        if (aReg >= val) {
            overflowToggle = true;
        } else {
            overflowToggle = false;
        }
        signed char oldsgn = AReg.sign;
        aReg = dividend/val;
        xReg = dividend% val;
        
        AReg = MIXWord(static_cast<int>(aReg));
        AReg.sign = valsgn*oldsgn;
        XReg = MIXWord(static_cast<int>(xReg));
        XReg.sign = oldsgn;
        
    } else {
        overflowToggle = true;
    }
}

// special instruction: conversions and halt
void numChar(MIXAddr addr, MIXByte index, MIXByte field, Opcode oc)
{
    if (field==2) throw halted_exception();
     nullfunc(addr, index,field,oc); // not yet implemented
}


void shift(MIXAddr addr, MIXByte index, MIXByte field, Opcode oc)
{
    nullfunc(addr,index,field,oc); // call nullfunc; not implemented yet
}

// Load instructions:
// This function covers all register cases, possibly with sign change.
// It uses indirection and some flags to avoid code duplication
// In other words, this handles about 16 different instructions
void load(MIXAddr addr, MIXByte index, MIXByte field, Opcode oc)
{
    int lo = field/8;
    int hi = field%8;
    bool loadneg=false;
    
    int newAddr = addr.decode()+ IReg[index].decode();
    int val = Memory[newAddr].decode(lo,hi); // full decoding and promotion
    
    // index into the array of registers (LDA is at the base)
    int whichReg = static_cast<int>(oc) - static_cast<int>(LDA);
    if (whichReg >=8) {
        whichReg -= 8; // the case of loading with negative: set a flag
        loadneg = true;
    }
    
    // if it is an index register, i.e., only two bytes
    if (whichReg > 0 && whichReg < 7) {
        MIXAddr *r = static_cast<MIXAddr*>(mutable_registers[whichReg]);
        // managed environment: throw an address violation.
        if (val >= NUMBASE*NUMBASE) throw address_violation(); // two bytes
        // set the register
        *r = MIXAddr(val);
        if (loadneg) r->sign = -r->sign; // swap sign if it is warranted
        
    } else { // A or X register: a full word
        MIXWord *r = static_cast<MIXWord*>(mutable_registers[whichReg]);
        // set the register
        *r = MIXWord(val); // this automatically takes care of field spec
        r->sign = (lo > 0 ? 1: Memory[newAddr].sign); //set sign
        if (loadneg) r->sign = -r->sign; // swap
        // preserve negative zero, unless the field spec forbids it
    }
}

void store(MIXAddr addr, MIXByte index, MIXByte field, Opcode oc)
{
    int lo = field/8;
    int hi = field%8;
    
    int newAddr = addr.decode()+ IReg[index].decode();
    // same as in load: compute the offset into the (immutable) register array
    int whichReg = static_cast<int>(oc) - static_cast<int>(STA);
    
    // if it is an index register, or jump, i.e., only two bytes
    MIXWord toStore;
    if ((whichReg > 0 && whichReg < 7) || whichReg == 8) {
        // get the data as MIXAddr
        const MIXAddr *r = static_cast<const MIXAddr*>(registers[whichReg]);
        
        // change the word's sign if field specification warrants it
        toStore.sign = (lo>0? 1: r->sign);
        
        // set the high bytes to 0
        toStore.byte[0] = toStore.byte[1] = toStore.byte[2] = 0;
        toStore.byte[3]= r->byte[0];
        toStore.byte[4]= r->byte[1];
    } else { // A or X
        toStore = *static_cast<const MIXWord*>(registers[whichReg]);
        // preserve negative zero, unless the field spec forbids it
    }
    int newHi = lo > 0 ? hi-lo +1 : hi-lo;
    
    // shift the field to the left to fit
    int noSgnLo = lo > 0 ? lo-1 : 0; // start at 0 regardless
    // the purpose of this is to left shift it into place
    for (int j = 0; j < newHi; j++) {
        Memory[newAddr].byte[j] = toStore.byte[noSgnLo+j];
    }
    // also note that any bytes not referred to in the field spec
    // are unmodified. In particular, STJ will store the jump register
    // into the address field (0:2) of the memory word
    Memory[newAddr].sign = (lo >0 ? 1 : toStore.sign);
}

void move(MIXAddr addr, MIXByte index, MIXByte field, Opcode oc) {
    nullfunc(addr, index, field, oc); // again call nullfunc
}

void jump(MIXAddr addr, MIXByte index, MIXByte field, Opcode oc)
{
    int newAddr = addr.decode() + IReg[index].decode();
    bool jumpcond = false;
    enum {
        UNCOND,
        UNCOND_SAVE,
        OV,
        NOV,
        LESS,
        EQ,
        GREATER,
        GE,
        NE,
        LE
    };
    switch (field) {
        case UNCOND:
        case UNCOND_SAVE:
            jumpcond=true;
            break;
        case OV:
            jumpcond = overflowToggle;
            overflowToggle = false;
            break;
        case NOV:
            jumpcond = !overflowToggle;
            overflowToggle = false;
            break;
        case LESS:
            jumpcond =  compIndicator < 0;
            break;
        case EQ:
            jumpcond = compIndicator == 0;
            break;
        case GREATER:
            jumpcond = compIndicator > 0;
            break;
        case GE:
            jumpcond = compIndicator >=0;
            break;
        case NE:
            jumpcond = compIndicator != 0;
            break;
        case LE:
            jumpcond = compIndicator <= 0;
            break;
        default:
            std::clog << "Should not have gotten to default in jump instruction.\n";
            assert(0);
            break;
    }
    // if the condition is satisfied, update the program counter accordingly
    if (jumpcond) programCounter = newAddr;
    // otherwise just increment it
    else programCounter++;
}

void jumpRegCond(MIXAddr addr, MIXByte index, MIXByte field, Opcode oc)
{
    enum {
        NEG,
        ZERO,
        POS,
        NONNEG,
        NONZERO,
        NONPOS
    };
    bool jumpcond = false;
    int newAddr = addr.decode()+ IReg[index].decode();
   // int val = Memory[newAddr].decode(lo,hi); // full decoding and promotion
    
    // index into the array of registers (LDA is at the base)
    int whichReg = static_cast<int>(oc) - static_cast<int>(JAN);
    
    int val =0;
    if ((whichReg > 0 && whichReg < 7) ) {
        // get the data as MIXAddr
        const MIXAddr *r = static_cast<const MIXAddr*>(registers[whichReg]);
        val = r->decode();
    } else { // A or X
        const MIXWord *r = static_cast<const MIXWord*>(registers[whichReg]);
        val = r->decode();
    }

    switch (field) {
        case NEG:
            jumpcond= val<0;
            break;
        case ZERO:
            jumpcond = val ==0;
            break;
        case POS:
            jumpcond = val > 0;
            break;
        case NONNEG:
            jumpcond =  val >= 0;
            break;
        case NONZERO:
            jumpcond = val!=0;
            break;
        case NONPOS:
            jumpcond = val <= 0;
            break;
            
        default:
            std::clog << "Should not have gotten to default in jump instruction.\n";
            assert(0);
            break;
    }
    if (jumpcond) programCounter = newAddr;
    // otherwise just increment it
    else programCounter++;
}

void immed(MIXAddr addr, MIXByte index, MIXByte field, Opcode oc)
{
    enum {
        ENT_F,
        ENN_F,
        INC_F,
        DEC_F
    };
    int whichReg = static_cast<int>(oc) - static_cast<int>(INCA);
    int val = addr.decode() + IReg[index].decode();
    if (field == ENN_F || field == DEC_F) val = -val; // negate
    
    if ((whichReg > 0 && whichReg < 7) ) {
        MIXAddr *r = static_cast<MIXAddr*>(mutable_registers[whichReg]);
        if (field >= INC_F) val += r->decode(); // add what's there
        *r = MIXAddr(val);
    } else { // A or X
        MIXWord *r = static_cast<MIXWord*>(mutable_registers[whichReg]);
        if (field >= INC_F) val += r->decode();
        *r = MIXWord(val);
    }
}

void compare(MIXAddr addr, MIXByte index, MIXByte field, Opcode oc)
{
    int lo = field/8;
    int hi = field%8;
    
    int newAddr = addr.decode()+ IReg[index].decode();
    int val = Memory[newAddr].decode(lo,hi); // full decoding and promotion

    int whichReg = static_cast<int>(oc) - static_cast<int>(CMPA);
    
    int regVal;
    if ((whichReg > 0 && whichReg < 7) ) {
        const MIXAddr *r = static_cast<const MIXAddr*>(registers[whichReg]);
        regVal = r->decode();
    } else { // A or X
        const MIXWord *r = static_cast<const MIXWord*>(registers[whichReg]);
        regVal = r->decode();
    }
    if (val < regVal) compIndicator = -1;
    else if (val == regVal) compIndicator = 0;
    else compIndicator = 1;
}

// The actual optable
MIXOp opTable[64] = {&nop, &add, &sub, &mul, &div, &numChar, &shift, &move, // 0 to 7 : arithmetic and special
                    &load, &load, &load, &load, &load, &load, &load, &load, // 8 to 15 : load ops
                    &load, &load, &load, &load, &load, &load, &load, &load, // 16 to 23 : load neg ops
                     &store, &store, &store, &store, &store, &store, &store, &store, // 24 to 31 : store ops
      &store, &store, &nullfunc, &nullfunc, &nullfunc, &nullfunc, &nullfunc, &jump, // 32 to 39 : store, I/O, jump on indicators
&jumpRegCond, &jumpRegCond, &jumpRegCond, &jumpRegCond, &jumpRegCond, &jumpRegCond, &jumpRegCond, &jumpRegCond, // 40 to 47 : jump on registers
&immed, &immed, &immed, &immed, &immed, &immed, &immed, &immed, // 48 to 55 :  immediates
&compare, &compare, &compare, &compare, &compare, &compare, &compare, &compare}; // 55 to 63 : comparison