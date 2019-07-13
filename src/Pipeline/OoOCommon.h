//
// Created by Alex Chi on 2019-07-10.
//

#ifndef RISCV_SIMULATOR_OOOCOMMON_H
#define RISCV_SIMULATOR_OOOCOMMON_H

enum RSID {
    NONE = 0,
    RS_BEGIN = 255,
    ADD1, ADD2, ADD3,
    STORE1, STORE2, STORE3,
    LOAD1, LOAD2, LOAD3,
    BRANCH1, RS_END
};

#endif //RISCV_SIMULATOR_OOOCOMMON_H
