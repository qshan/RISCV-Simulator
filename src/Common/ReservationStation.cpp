//
// Created by Alex Chi on 2019-07-10.
//

#include "ReservationStation.h"
#include "../Module/ALUUnit.h"
#include <iomanip>

void RS::debug() {
    using std::cout;
    using std::setw;
    using std::endl;
    cout << resolve(__debug_identifier) << (Busy.current() ? " (⚪busy)" : "       ") << "\t";
    cout << setw(8) << ALUUnit::resolve((ALUUnit::OP) Op.current());
    char buffer[100];
    sprintf(buffer, "#%d", Qj.current());
    cout << setw(8) << buffer;
    sprintf(buffer, "#%d", Qk.current());
    cout << setw(8) << buffer;
    debug_immediate(Vj.current(), 11);
    debug_immediate(Vk.current(), 11);
    debug_immediate(A.current(), 7);
    debug_immediate(Dest.current(), 7);
    debug_immediate(Tag.current(), 7);
    cout << endl;
}

void RS::debug_header() {
    char buffer[1000];
    sprintf(buffer, "%16s%8s%8s%8s%24s%24s%16s%16s%16s\n",
            "", "Op", "Qj", "Qk", "Vj", "Vk", "A", "Dest", "Tag");
    std::cout << buffer << std::endl;
}

