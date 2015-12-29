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
#include <sstream>

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
}


void shift(MIXAddr addr, MIXByte index, MIXByte field, Opcode oc)
{
    nullfunc(addr,index,field,oc); // call nullfunc; not implemented yet
}

// Load instructions:
// This function covers all register cases, possibly with sign change.
// It uses indirection and some flags to
// avoid code duplication

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

// The actual optable
MIXOp opTable[64] = {&nop, &add, &sub, &mul, &div, &numChar, &shift, &move,
                    &load, &load, &load, &load, &load, &load, &load, &load,
                    &load, &load, &load, &load, &load, &load, &load, &load,
                     &store, &store, &store, &store, &store, &store, &store, &store,
      &store, &store, &nullfunc, &nullfunc, &nullfunc, &nullfunc, &nullfunc, &nullfunc,
&nullfunc, &nullfunc, &nullfunc, &nullfunc, &nullfunc, &nullfunc, &nullfunc, &nullfunc,
&nullfunc, &nullfunc, &nullfunc, &nullfunc, &nullfunc, &nullfunc, &nullfunc, &nullfunc,
&nullfunc, &nullfunc, &nullfunc, &nullfunc, &nullfunc, &nullfunc, &nullfunc, &nullfunc};