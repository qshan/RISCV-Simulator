//
// Created by Alex Chi on 2019-07-10.
//

#include <vector>
#include <cassert>

#include "Issue.h"
#include "OoOExecute.h"

#include "../Module/ALUUnit.h"
#include "../Module/BranchPrediction.h"
#include "../Common/Session.h"

void Issue::update() {
    // Note that pc is register. No matter how
    // many times update is executed, pc always
    // stays the same.

    auto _inst = session->memory.read_word(pc);
    auto inst = parse_inst(_inst & 0x7f, _inst);

    pc = issue(inst);
}

void Issue::tick() {
    if (instruction_stalled()) stat.stall_cycle++;
    if (__jump_flag) {
        __jump_flag = false;
        pc = __jump_dest;
    }
    pc.tick();
    issue_cnt.tick();
}

ALUUnit::OP get_op_ri(InstructionBase inst) {
    switch (inst.funct3) {
        case 0b000:
            return ALUUnit::OP::ADD;
        case 0b010:
            return ALUUnit::OP::SLT;
        case 0b011:
            return ALUUnit::OP::SLTU;
        case 0b100:
            return ALUUnit::OP::XOR;
        case 0b110:
            return ALUUnit::OP::OR;
        case 0b111:
            return ALUUnit::OP::AND;
        case 0b001:
            return ALUUnit::OP::SLL;
        case 0b101:
            if (inst.imm & (1 << 9))
                return ALUUnit::OP::SRA;
            else
                return ALUUnit::OP::SRL;
    }
    assert(false);
    return ALUUnit::OP::NONE_OP;
}

ALUUnit::OP get_op_rr(InstructionBase inst) {
    switch (inst.funct3) {
        case 0b000:
            if (inst.funct7 == 0b0000000) return ALUUnit::OP::ADD;
            if (inst.funct7 == 0b0100000) return ALUUnit::OP::SUB;
        case 0b001:
            return ALUUnit::OP::SLL;
        case 0b010:
            return ALUUnit::OP::SLT;
        case 0b011:
            return ALUUnit::OP::SLTU;
        case 0b100:
            return ALUUnit::OP::XOR;
        case 0b101:
            if (inst.funct7 == 0b0000000)
                return ALUUnit::OP::SRL;
            if (inst.funct7 == 0b0100000)
                return ALUUnit::OP::SRA;
        case 0b110: // OR
            return ALUUnit::OP::OR;
        case 0b111: // AND
            return ALUUnit::OP::AND;
    }
    assert(false);
    return ALUUnit::OP::NONE_OP;
}

ALUUnit::OP get_op_branch(InstructionBase inst) {
    switch (inst.funct3) {
        case 0b000: // BEQ
        case 0b001: // BNE
            return ALUUnit::OP::SUB;
        case 0b100: // BLT
        case 0b101: // BGE
            return ALUUnit::OP::SLT;
        case 0b110: // BLTU
        case 0b111: // BGEU
            return ALUUnit::OP::SLTU;
    }
    assert(false);
    return ALUUnit::OP::NONE_OP;
}

Immediate Issue::issue(const InstructionBase &inst) {
    _debug_dispatched_inst = inst;
    issue_cnt.prev = issue_cnt.next = pc;

    if (inst.t == InstructionBase::J) { // JAL
        return issue_jal(inst);
    } else if (inst.t == InstructionBase::B) { // Branch
        return issue_branch(inst);
    } else if (inst.opcode == 0b1100111) { // JALR
        return issue_jalr(inst);
    } else if (inst.opcode == 0b0000011) { // Load
        return issue_load(inst);
    } else if (inst.opcode == 0b0100011) { // Store
        return issue_store(inst);
    } else if (inst.opcode == 0b0010011) { // Immediate Op
        return issue_immediate_op(inst);
    } else if (inst.opcode == 0b0110011) { // Op
        return issue_op(inst);
    } else if (inst.opcode == 0b0110111) { // LUI
        return issue_lui(inst);
    } else if (inst.opcode == 0b0110111) { // AUIPC
        return issue_auipc(inst);
    }
    return pc;
}

Issue::Issue(Session *session) :
    session(session),
    __jump_flag(false) {}

InstructionBase Issue::parse_inst(unsigned opcode, Immediate inst) {
    if (opcode == 0b0110011) { return InstructionR(inst); } // ***
    if (opcode == 0b0010011) { return InstructionI(inst); } // ***I
    if (opcode == 0b0100011) { return InstructionS(inst); } // S*
    if (opcode == 0b0000011) { return InstructionI(inst); } // L*
    if (opcode == 0b1100011) { return InstructionB(inst); } // B**
    if (opcode == 0b1100111) { return InstructionI(inst); } // JALR
    if (opcode == 0b1101111) { return InstructionJ(inst); } // JAL
    if (opcode == 0b0010111) { return InstructionU(inst); } // AUIPC
    if (opcode == 0b0110111) { return InstructionU(inst); } // LUI
    return InstructionBase::nop();
}

void Issue::debug() {
    std::cout << "PC prev->current" << std::endl;
    std::cout << std::hex << "\t" << pc << "\t" << pc.current();
    if (instruction_stalled()) {
        std::cout << " ⭕(instruction stalled)";
    }
    std::cout << std::endl;
    std::cout << "Instruction" << std::endl;
    _debug_dispatched_inst.debug();
    if (__jump_flag) {
        std::cout << "⭕" << "(jump to "
                  << std::hex << __jump_dest << ")" << std::endl;
    }
}

RSID Issue::find_available_op_unit() {
    static const std::vector<RSID> op_unit = {ADD1, ADD2, ADD3, ADD4};
    return find_available_unit_in(op_unit);
}

std::vector<RSID> Issue::find_available_op_units(unsigned size) {
    static const std::vector<RSID> op_unit = {ADD1, ADD2, ADD3, ADD4};
    return find_available_units_in(op_unit, size);
}

RSID Issue::find_available_load_unit() {
    static const std::vector<RSID> load_unit = {LOAD1, LOAD2, LOAD3};
    return find_available_unit_in(load_unit);
}

RSID Issue::find_available_store_unit() {
    static const std::vector<RSID> store_unit = {STORE1, STORE2, STORE3};
    return find_available_unit_in(store_unit);
}


Immediate Issue::issue_jalr(const InstructionBase &inst) {
    auto e = session->e;

    auto unit_ids = find_available_op_units(2);
    if (unit_ids.size() == 0) return pc;

    // we need 2 slots for jalr instruction
    if (!e->probe_rob(2)) return pc;

    auto robs = e->acquire_robs(2);

    {   // save pc + 4 to rd
        auto b = robs[0];
        auto unit_id = unit_ids[0];
        auto rs = e->occupy_unit(unit_id);

        rs->Op = ALUUnit::OP::ADD;
        rs->Tag = issue_cnt;

        issue_imm_to_Vj(pc, rs, unit_id);
        issue_imm_to_Vk(4, rs, unit_id);

        rs->Dest = b;

        e->rob[b].Dest = inst.rd;
        InstructionBase _mock_inst = inst;
        _mock_inst.opcode = 0b1101111; // it just works like JAL
        e->rob[b].Inst = _mock_inst;

        e->occupy_register(inst.rd, b);
    }

    {   // process jalr
        auto b = robs[1];
        auto unit_id = unit_ids[1];
        auto rs = e->occupy_unit(unit_id);

        rs->Op = ALUUnit::OP::ADD;
        rs->Tag = issue_cnt;

        issue_rs_to_Vj(inst.rs1, rs, unit_id);
        issue_imm_to_Vk(inst.imm, rs, unit_id);

        rs->Dest = b;

        e->rob[b].Dest = 0;
        e->rob[b].Inst = inst;
    }

    // TODO: just stall instead of executing non-sense instructions
    auto pred_pc = pc + 4;

    return pred_pc;
}

Immediate Issue::issue_branch(const InstructionBase &inst) {
    auto e = session->e;

    auto unit_id = find_available_op_unit();
    if (unit_id == NONE) return pc;

    auto b = e->acquire_rob();
    if (b == -1) return pc;

    auto rs = e->occupy_unit(unit_id);

    rs->Op = get_op_branch(inst);
    rs->Tag = issue_cnt;

    issue_rs_to_Vj(inst.rs1, rs, unit_id);
    issue_rs_to_Vk(inst.rs2, rs, unit_id);

    rs->Dest = b;

    // TODO: here we should return predicted branch,
    //       now we just apply always-not-taken strategy
    bool take = session->branch->take(pc);
    auto pred_pc = take ? pc + inst.imm : pc + 4;

    e->rob[b].Dest = pc;
    e->rob[b].Inst = inst;
    e->rob[b].Tag = pred_pc;

    e->occupy_register(inst.rd, b);

    return pred_pc;
}

Immediate Issue::issue_immediate_op(const InstructionBase &inst) {
    auto e = session->e;

    auto unit_id = find_available_op_unit();
    if (unit_id == NONE) return pc;

    auto b = e->acquire_rob();
    if (b == -1) return pc;

    auto rs = e->occupy_unit(unit_id);

    rs->Op = get_op_ri(inst);
    rs->Tag = issue_cnt;

    issue_rs_to_Vj(inst.rs1, rs, unit_id);
    issue_imm_to_Vk(inst.imm, rs, unit_id);

    rs->Dest = b;

    e->rob[b].Dest = inst.rd;
    e->rob[b].Inst = inst;

    e->occupy_register(inst.rd, b);

    return pc + 4;
}

Immediate Issue::issue_op(const InstructionBase &inst) {
    auto e = session->e;

    auto unit_id = find_available_op_unit();
    if (unit_id == NONE) return pc;

    auto b = e->acquire_rob();
    if (b == -1) return pc;

    auto rs = e->occupy_unit(unit_id);

    rs->Op = get_op_rr(inst);
    rs->Tag = issue_cnt;

    issue_rs_to_Vj(inst.rs1, rs, unit_id);
    issue_rs_to_Vk(inst.rs2, rs, unit_id);

    rs->Dest = b;

    e->rob[b].Dest = inst.rd;
    e->rob[b].Inst = inst;

    e->occupy_register(inst.rd, b);

    return pc + 4;
}

void Issue::issue_rs_to_Vj(unsigned reg_id, RS *rs, RSID unit_id) {
    auto e = session->e;
    if (reg_id != 0 && e->Busy[reg_id]) {
        auto h = e->Reorder[reg_id];
        if (e->rob[h].Ready) {
            rs->Vj = e->rob[h].Value;
            rs->Qj = 0;
        } else rs->Qj = h;
    } else {
        rs->Vj = session->rf.read(reg_id);
        rs->Qj = NONE;
    }
}

void Issue::issue_rs_to_Vk(unsigned reg_id, RS *rs, RSID unit_id) {
    auto e = session->e;
    if (reg_id != 0 && e->Busy[reg_id]) {
        auto h = e->Reorder[reg_id];
        if (e->rob[h].Ready) {
            rs->Vk = e->rob[h].Value;
            rs->Qk = 0;
        } else rs->Qk = h;
    } else {
        rs->Vk = session->rf.read(reg_id);
        rs->Qk = NONE;
    }
}

void Issue::issue_imm_to_Vk(Immediate imm, RS *rs, RSID) {
    rs->Qk = NONE;
    rs->Vk = imm;
}

void Issue::issue_imm_to_Vj(Immediate imm, RS *rs, RSID) {
    rs->Qj = NONE;
    rs->Vj = imm;
}

Immediate Issue::issue_load(const InstructionBase &inst) {
    auto e = session->e;

    auto unit_id = find_available_load_unit();
    if (unit_id == NONE) return pc;

    auto b = e->acquire_rob();
    if (b == -1) return pc;

    auto rs = e->occupy_unit(unit_id);

    rs->Op = inst.funct3;
    rs->Tag = issue_cnt;

    issue_rs_to_Vj(inst.rs1, rs, unit_id);
    issue_imm_to_Vk(0, rs, unit_id);
    issue_imm_to_A(inst.imm, rs, unit_id);

    rs->Dest = b;

    e->rob[b].Inst = inst;
    e->rob[b].Dest = inst.rd;

    e->occupy_register(inst.rd, b);

    return pc + 4;
}

Immediate Issue::issue_store(const InstructionBase &inst) {
    auto e = session->e;

    auto unit_id = find_available_store_unit();
    if (unit_id == NONE) return pc;

    auto b = e->acquire_rob();
    if (b == -1) return pc;

    auto rs = e->occupy_unit(unit_id);

    rs->Op = inst.funct3;
    rs->Tag = issue_cnt;

    issue_rs_to_Vj(inst.rs1, rs, unit_id);
    issue_rs_to_Vk(inst.rs2, rs, unit_id);
    issue_imm_to_A(inst.imm, rs, unit_id);

    rs->Dest = b;

    e->rob[b].Inst = inst;

    return pc + 4;
}

Immediate Issue::issue_lui(const InstructionBase &inst) {
    auto e = session->e;

    auto unit_id = find_available_op_unit();
    if (unit_id == NONE) return pc;

    auto b = e->acquire_rob();
    if (b == -1) return pc;

    auto rs = e->occupy_unit(unit_id);

    rs->Op = ALUUnit::OP::ADD;
    rs->Tag = issue_cnt;

    issue_imm_to_Vj(0, rs, unit_id);
    issue_imm_to_Vk(inst.imm, rs, unit_id);

    rs->Dest = b;

    e->rob[b].Dest = inst.rd;
    e->rob[b].Inst = inst;

    e->occupy_register(inst.rd, b);

    return pc + 4;
}

Immediate Issue::issue_auipc(const InstructionBase &inst) {
    auto e = session->e;

    auto unit_id = find_available_op_unit();
    if (unit_id == NONE) return pc;

    auto b = e->acquire_rob();
    if (b == -1) return pc;

    auto rs = e->occupy_unit(unit_id);

    rs->Op = ALUUnit::OP::ADD;
    rs->Tag = issue_cnt;

    issue_imm_to_Vj(pc, rs, unit_id);
    issue_imm_to_Vk(inst.imm, rs, unit_id);

    rs->Dest = b;

    e->rob[b].Dest = inst.rd;
    e->rob[b].Inst = inst;

    e->occupy_register(inst.rd, b);

    return pc + 4;
}

RSID Issue::find_available_unit_in(const std::vector<RSID> &src) {
    for (auto &&unit : src) {
        if (session->e->available(unit)) return unit;
    }
    return NONE;
}

std::vector<RSID> Issue::find_available_units_in(const std::vector<RSID> &src, unsigned size) {
    std::vector<RSID> rs;
    for (auto &&unit : src) {
        if (session->e->available(unit)) {
            rs.push_back(unit);
            --size;
        }
        if (size == 0) return rs;
    }
    return std::vector<RSID>();
}

Immediate Issue::issue_jal(const InstructionBase &inst) {
    auto e = session->e;

    auto unit_id = find_available_op_unit();
    if (unit_id == NONE) return pc;

    auto b = e->acquire_rob();
    if (b == -1) return pc;

    auto rs = e->occupy_unit(unit_id);

    rs->Op = ALUUnit::OP::ADD;
    rs->Tag = issue_cnt;

    issue_imm_to_Vk(pc, rs, unit_id);
    issue_imm_to_Vj(4, rs, unit_id);

    rs->Dest = b;

    e->rob[b].Dest = inst.rd;
    e->rob[b].Inst = inst;

    e->occupy_register(inst.rd, b);

    return pc + inst.imm;
}

void Issue::issue_imm_to_A(Immediate imm, RS *rs, RSID) {
    rs->A = imm;
}

void Issue::notify_jump(Immediate jump_dest) {
    __jump_flag = true;
    __jump_dest = jump_dest;
}

void Issue::report(std::ostream &out) {
    out << "\t--- Issue Stage Report ---" << std::endl;
    out << "\t" << stat.stall_cycle;
    out << " (" << 100.0 * stat.stall_cycle / session->stat.cycle << "%)";
    out << " cycles stall instruction";
    out << std::endl;
    out << std::endl;
}

bool Issue::instruction_stalled() {
    return pc.current() == pc;
}
