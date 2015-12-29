//
//  main.cpp
//  mix-simulator
//
//  Created by Chris on 12/24/15.
//  Copyright © 2015 Chris. All rights reserved.
//

#include <iostream>
#include <iomanip>
#include "mix.h"



int programCounter;

void octalDump();
void octalEntry();

int main(int argc, const char * argv[])
{
    
    // read file to load memory at address zero
    octalEntry();
    try {
        // main event loop: read the next instruction and interpret it
        while ( true ) {
            MIXWord V = Memory[programCounter]; // get the value
            Opcode oc = Opcode(V.byte[4]); // get the opcode
            MIXAddr AA(V); // get the address (first two bytes)
            opTable[oc](AA,V.byte[2],V.byte[3],oc); // call the op table
            // (this is the concept of "interpretive routine")
            
            // if it is a jump instruction, set the location of the next instruction accordingly
            if (static_cast<int>(oc) >= static_cast<int>(JMP)
                &&static_cast<int>(oc) <= static_cast<int>(JXN))  {
                if (oc != JSJ) JReg = MIXAddr(programCounter);
                programCounter = AA.decode();
            } else { // otherwise just increment the program counter
                ++programCounter;
            }
            // rudimentary managed environment:
            if (programCounter >= ADDR_CAP) throw memory_access_violation();
        }
    }
    catch(halted_exception&) {
        std::cerr << "Execution finished. ";
    }
    catch(address_violation&) {
        std::cerr << "Invalid address.\n";
    }
    catch(bad_opcode& ex) {
        std::cerr << ex.what;
    }
    catch (...) {
        std::cerr << "Unknown exception occurred.\n";
    }
    octalDump();
    // write memory to file
    return 0;
}

void octalEntry()
{
    for (;;) {
        std::string s;
        std::cerr << "Would you like to set the memory (Y/N)? ";
        std::cin >> s;
        if (s[0] != 'Y' && s[0] != 'y') break; // if anything but yes, exit
        int N,M;
        
        std::cerr << "How many memory entries do you want enter? ";
        std::cin >> N;
        std::cerr << "Where do you want to start? ";
        std::cin>> std::dec >> M;
        if (M < 0) M=0;
        
        for (int i=M; i < M+N && i<ADDR_CAP; i++) {
            int sgn, byte ;
            std::cerr << std::setfill('0') << std::setw(4) << i << ": ";
            std::cin >> sgn;
            Memory[i].sign = sgn/std::abs(sgn);
            for (int j = 0; j<5;j++) {
                std::cin >> std::oct >> byte;
                Memory[i].byte[j] = static_cast<MIXByte>(byte);
            }
        }
    }
        
}
void octalDump()
{
    for (;;) {
        std::string s;
        std::cerr << "Would you like to see the memory (Y/N)? ";
        std::cin >>s;
        if (s[0] != 'Y' && s[0] != 'y') break; // if anything but yes, exit
        int N, M;
        std::cerr << "How many memory entries do you want to see? ";
        std::cin>>  N;
        std::cerr << "Where do you want to start? ";
        std::cin>>  M;
        if (M < 0) M=0;
        
        std::cout << "Octal dump starting at location "
        << M << " for " << N << " entries:" << std::endl;
        std::cout << std::setfill('0');
        for (int i= M; i < M+N && i< ADDR_CAP; i++) {
            MIXWord V = Memory[i];
            std::cout << std::setw(4) << i << ": ";
            std::cout << V;
            std::cout << std::endl;
        }
    }
}