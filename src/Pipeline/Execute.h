//
// Created by Alex Chi on 2019-07-01.
//

#ifndef RISCV_SIMULATOR_EXECUTE_H
#define RISCV_SIMULATOR_EXECUTE_H

#include "Stage.hpp"
#include "../Module/ALU.h"

class Execute : public Stage {
private:
    ALU alu;
public:
    Immediate e_val;

    class InvalidAccess {
    };

    class InvalidOp {

    };

    Execute(Session *session);

    void tick();

    Immediate dispatch(Wire wire) override { throw InvalidKey(); };
};


#endif //RISCV_SIMULATOR_EXECUTE_H
