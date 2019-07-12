//
// Created by Alex Chi on 2019-07-11.
//

#ifndef RISCV_SIMULATOR_REORDERBUFFER_H
#define RISCV_SIMULATOR_REORDERBUFFER_H

#include "Register.hpp"
#include <string>

class ROB {
public:
    Register<unsigned> Type;
    Register<unsigned> Dest, Value;
    Register<bool> Ready;

    unsigned __debug_identifier;

    ROB() : Type(0), Dest(0), Value(0), Ready(true) {}

    void tick() {
        Type.tick();
        Dest.tick();
        Value.tick();
        Ready.tick();
    }

    static void debug_header();

    void debug();

    std::string resolve(unsigned opcode) {
        switch (opcode) {
            case 0:
                return "none";
            case 0b0110111:
                return "lui";
            case 0b0010111:
                return "auipc";
            case 0b1101111:
                return "jal";
            case 0b1100111:
                return "jalr";
            case 0b1100011:
                return "branch";
            case 0b0000011:
                return "load";
            case 0b0100011:
                return "store";
            case 0b0010011:
                return "opimm";
            case 0b0110011:
                return "op";
            default:
                return "other";
        }
    }
};


#endif //RISCV_SIMULATOR_REORDERBUFFER_H
