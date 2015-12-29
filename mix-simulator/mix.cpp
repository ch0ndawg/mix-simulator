//
//  mix.cpp
//  mix-simulator
//
//  Created by Chris on 12/27/15.
//  Copyright © 2015 Chris. All rights reserved.
//

#include "mix.h"
#include <iostream>
#include <iomanip>

// implementation file containing data types,

// registers, indicators, and memory

MIXWord AReg;
MIXAddr IReg[7];
MIXWord XReg;
MIXAddr JReg;
const MIXAddr ZReg;

MIXWord Memory[ADDR_CAP];

signed char compIndicator;
bool overflowToggle;

MIXWord::MIXWord(int i)
:sign(1)
{
    if (i < 0) {
        sign=-1;
        i = -i;
    }
    
    for (int j=1; j <=5; j++) {
        byte[5-j] = i%NUMBASE;
        i /= NUMBASE;
    }
}

int MIXWord::decode(int lo,int hi) const
{
    int total=0;
    int j = lo==0 ? 0 : lo-1; // field specification
    for ( ; j<hi; j++) {
        total = NUMBASE*total +byte[j];
    }
    return (sign >=0 || lo > 0) ? total : -total;
    
    // note: care must be taken to ensure +0 is preserved; this is a problem.
}

std::ostream &operator <<(std::ostream &os, MIXWord& V)
{
    // get old flags and state
    std::ios_base::fmtflags old_flags = os.flags();
    os.setf(std::ios::showpos);
    os << static_cast<int>(V.sign) << ' ';
    os.unsetf(std::ios::showpos);
    for (int j=0; j<5; j++) {
        os.setf(std::ios::oct, std::ios::basefield);
        os.width(2);
        os << static_cast<unsigned int>(V.byte[j]) << ' ';
    }
    // restore old flags
    os.setf(old_flags);
    return os;
}