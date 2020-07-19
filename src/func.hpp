#ifndef RISCV_FUNC_HPP
#define RISCV_FUNC_HPP

#include <iostream>
#include <cassert>
#include <iomanip>
#include <bitset>
#include "type.hpp"
#include "mem.hpp"
#include "pred.hpp"
#define debug std::cout

class stall_throw {
public:
    int stall_num;
    stall_throw(const int _): stall_num(_) {}
};

class Instruction_Fetcher {
public:
    int fetch(Pipeline_Register &new_pr, const Register &reg, Register_Tmp &new_reg, const Memory &mem, const Predictor &pred) {

        // debug << "Fetching..." << std::endl;
        IF_ID ret;
        ret.new_pc = reg.pc; // FIXME:
        ret.cmd.in.o1 = (uint)mem[ret.new_pc++];
        ret.cmd.in.o2 = (uint)mem[ret.new_pc++];
        ret.cmd.in.o3 = (uint)mem[ret.new_pc++];
        ret.cmd.in.o4 = (uint)mem[ret.new_pc++];

        // Prediction:
        uint jp = 0;
        if(pred.predict(reg.pc, jp)) {
            // debug << "predicted JUMP!" << std::endl;
            new_reg.pc = jp; // the next IF is the branched one
        } else {
            new_reg.pc = ret.new_pc;
        }
        ret.predicted_pc = new_reg.pc;
        // switch(new_pr.id_ex.rough) {
        // case AUIPC:
        // case JAL:
        // case JALR:
        //     return ret;
        // case B_func:
        //     if(new_pr.id_ex.cond) return ret;
        // default: break;
        // }
        // debug << "pc: " << std::hex << reg.pc << '\t';
        // debug << std::setw(8) << std::setfill('0') << std::hex << ret.cmd.data << "\t" << new_reg.pc << std::endl;

        new_pr.if_id = ret;
        return 0;
    }
};

class Instruction_Decorder {
    static const int OPCODE[9];

public:
    int decode(const Pipeline_Register &pr, Pipeline_Register &new_pr, const Register &reg, Register_Tmp &new_reg, Predictor &pred) {
        if(pr.if_id.cmd.data == 0u) { new_pr.id_ex = Instruction(CMD(0u)); return 0; }
        if(pr.stalled == 1) { // stalled IF, in the next round don't do ID
            ++new_pr.stalled;
            return 0;
        }

        // debug << "Decoding..." << std::endl;

        // #branch hazard FIXME:
        // switch(pr.id_ex.rough) {
        // case AUIPC:
        // case JAL:
        // case JALR:
        //     // debug << "here!" << std::endl;
        //     new_reg.pc = pr.id_ex.new_pc; // pc = jump_pc rather than former_pc + 4
        //     new_pr.id_ex = NOP;
        //     throw stall_throw(1);
        //     break;
        // // case B_func:
        // //     if(pr.id_ex.cond) {
        // //         new_reg.pc = pr.id_ex.new_pc;
        // //         new_pr.id_ex = NOP;
        // //         throw stall_throw(1);
        // //     }
        // //     break;
        // default: break;
        // }

        // ##END
        if(pr.if_id.cmd.data == END_CMD) {
            // debug << "END_CMD!" << std::endl;
            new_pr.end_flag = 1;
            new_pr.id_ex = Instruction(CMD(0u)); // 0 inst: no inst
            return 0;
        }

        // get ROUGH
        auto cmd = pr.if_id.cmd;
        Instruction ret(pr.if_id);
        // switch(cmd.B.opcode) {
        // case 0b0110111:
        //     ret.rough = LUI;
        //     break;
        // case 0b0010111:
        //     ret.rough = AUIPC;
        //     break;
        // case 0b1101111:
        //     ret.rough = JAL;
        //     break;
        // case 0b1100111:
        //     ret.rough = JALR;
        //     break;
        // case 0b1100011: // finish the branch inst at this stage
        //     ret.rough = B_func;
        //     break;
        // case 0b0000011:
        //     ret.rough = LOAD_func;
        //     break;
        // case 0b0100011:
        //     ret.rough = STORE_func;
        //     break;
        // case 0b0010011:
        //     ret.rough = R_I_func;
        //     break;
        // case 0b0110011:
        //     ret.rough = R_R_func; // no Imm
        //     break;
        // default:
        //     // debug << std::bitset<7>(cmd.B.opcode) << std::endl;
        //     assert(0); break;
        // }
        switch(cmd.B.opcode) {
        case 0b0110111:
            ret.rough = LUI;
            ret.imm = cmd.U.getImm();
            break;
        case 0b0010111:
            ret.rough = AUIPC;
            ret.imm = cmd.U.getImm();
            ret.new_pc += ret.imm - 4;
            // ret.cond = 1;
            ret.branch_return = ret.new_pc; // adds this offset to the pc, then places the result in register rd.
            break;
        case 0b1101111:
            ret.rough = JAL;
            ret.imm = cmd.J.getImm();
            ret.branch_return = reg.pc;
            ret.new_pc += ret.imm - 4;
            // debug << "here: " << ret.new_pc << std::endl;
            // ret.cond = 1;
            break;
        case 0b1100111:
            ret.rough = JALR;
            ret.imm = cmd.I.getImm();
            // ret.cond = 1;
            break;
        case 0b1100011:
            ret.rough = B_func;
            ret.imm = cmd.B.getImm();
            break;
        case 0b0000011:
            ret.rough = LOAD_func;
            ret.imm = cmd.I.getImm();
            break;
        case 0b0100011:
            ret.rough = STORE_func;
            ret.imm = cmd.S.getImm();
            break;
        case 0b0010011:
            ret.rough = R_I_func;
            ret.imm = cmd.I.getImm();
            break;
        case 0b0110011:
            ret.rough = R_R_func;
            break;
        default:
            // debug << std::bitset<7>(cmd.B.opcode) << std::endl;
            assert(0); break;
        }

        // #RAW hazard (calculate after load)
        if(pr.id_ex.rough == LOAD_func && pr.id_ex.cmd.I.rd) {
            switch(ret.rough) {
            case JALR: // JALR
            case LOAD_func: // LOAD_func
            case R_I_func: // R_I // rs1-only funcs
                if(cmd.R.rs1 == pr.id_ex.cmd.I.rd) {
                    // debug << "RAW hazard detected!" << std::endl;
                    new_pr.id_ex.rough = STALLED_func; // FIXME:
                    return 2;
                }
                break;
            case B_func: // B
            case STORE_func: // STORE
            case R_R_func: // R_R // rs1 & rs2
                if(cmd.R.rs1 == pr.id_ex.cmd.I.rd || cmd.R.rs2 == pr.id_ex.cmd.I.rd) {
                    // debug << "RAW hazard2 detected!" << std::endl;
                    new_pr.id_ex.rough = STALLED_func;
                    return 2;
                }
                break;
            default:
                break;
            }
        }


    // JALR & B_func special operations:

        // if(pr.id_ex.cmd.I.rd) {
        //     switch(pr.id_ex.rough) {
        //     case LUI:
        //     case AUIPC:
        //     case JAL:
        //     case JALR:
        //     case R_I_func:
        //     case R_R_func:
        //         switch(ret.rough) {     // stall for rd funcs because of calculating JALR & B_func in ID
        //         case JALR:
        //             if(cmd.R.rs1 == pr.id_ex.cmd.I.rd) {
        //                 // debug << "data hazard caused by JALR!" << std::endl;
        //                 new_pr.id_ex.rough = STALLED_func; // FIXME:
        //                 throw stall_throw(2);
        //             }
        //             break;
        //         // case B_func:
        //         //     if(cmd.R.rs1 == pr.id_ex.cmd.I.rd || cmd.R.rs2 == pr.id_ex.cmd.I.rd) {
        //         //         debug << "data hazard caused by B_func!" << std::endl;
        //         //         new_pr.id_ex.rough = STALLED_func;
        //         //         throw stall_throw(2);
        //         //     }
        //         //     break;
        //         default: break;
        //         }
        //     default: break;
        //     }
        // }

        // if(pr.ex_mem.inst.cmd.I.rd) {
        //     if(pr.id_ex.rough != STALLED2_func && pr.ex_mem.inst.rough == LOAD_func) // the previous_previous inst is LOAD & STALLed less than twice
        //         switch(ret.rough) {
        //         case JALR:
        //             if(cmd.R.rs1 == pr.ex_mem.inst.cmd.I.rd) {
        //                 // debug << "data hazard caused by JALR!" << std::endl;
        //                 new_pr.id_ex.rough = STALLED2_func; // FIXME:
        //                 throw stall_throw(2);
        //             }
        //             break;
        //         // case B_func:
        //         //     if(cmd.R.rs1 == pr.ex_mem.inst.cmd.I.rd || cmd.R.rs2 == pr.ex_mem.inst.cmd.I.rd) {
        //         //         debug << "data hazard caused by B_func!" << std::endl;
        //         //         new_pr.id_ex.rough = STALLED2_func;
        //         //         throw stall_throw(2);
        //         //     }
        //         //     break;
        //         default: break;
        //         }
        // }

        ret.fine = cmd.B.funct3;
        ret.rs1 = reg[cmd.B.rs1];
        ret.rs2 = reg[cmd.B.rs2];

        // ID Forwarding (get renewed data in MEM_WB)
        // debug << "@ " << pr.mem_wb.inst.rough << " " << ret.rough << " " << ret.cmd.R.rs1 << " " << pr.mem_wb.inst.cmd.I.rd << std::endl;
        if(pr.mem_wb.inst.cmd.I.rd) { // not x0
            switch(pr.mem_wb.inst.rough) { // insts that get ans in MEM
            case LUI:
            case AUIPC:
            case JAL:
            case JALR:
            case R_I_func:
            case R_R_func:
            case LOAD_func: // here load inst can also affect the reg
                switch(ret.rough) {
                case JALR:
                case LOAD_func:
                case R_I_func: // rs1
                    if(ret.cmd.R.rs1 == pr.mem_wb.inst.cmd.I.rd) {
                        ret.rs1 = pr.mem_wb.data;
                        // debug << "ID forwarding." << std::endl;
                    }
                    break;
                case B_func:
                case STORE_func:
                case R_R_func: // rs1 & rs2
                    if(ret.cmd.R.rs1 == pr.mem_wb.inst.cmd.I.rd) {
                        ret.rs1 = pr.mem_wb.data;
                        // debug << "ID forwarding." << std::endl;
                    }
                    if(ret.cmd.R.rs2 == pr.mem_wb.inst.cmd.I.rd) {
                        ret.rs2 = pr.mem_wb.data;
                        // debug << "ID forwarding." << std::endl;
                    }
                    break;
                default: break;
                }
                break;
            default: break;
            }
        }
        // Forwarding for JALR & B_func
        // if(pr.ex_mem.inst.cmd.I.rd) { // not x0
        //     switch(pr.ex_mem.inst.rough) { // inst that get ans in EX
        //     case LUI:
        //     case AUIPC:
        //     case JAL:
        //     case JALR:
        //     case R_I_func:
        //     case R_R_func: // no load inst
        //         switch(ret.rough) {
        //         case JALR:
        //             if(ret.cmd.R.rs1 == pr.ex_mem.inst.cmd.I.rd) {
        //                 ret.rs1 = pr.ex_mem.ans;
        //             }
        //             break;
        //         // case B_func:
        //         //     if(ret.cmd.R.rs1 == pr.ex_mem.inst.cmd.I.rd) ret.rs1 = pr.ex_mem.ans;
        //         //     if(ret.cmd.R.rs2 == pr.ex_mem.inst.cmd.I.rd) ret.rs2 = pr.ex_mem.ans;
        //         //     break;
        //         // default: break;
        //         }
        //         break;
        //     default: break;
        //     }
        // }
        // Forwarding end

    // JALR & B_func operations end

        new_pr.id_ex = ret;
        if((ret.rough == AUIPC || ret.rough == JAL) && ret.new_pc != new_reg.pc) { // AUIPC/JAL
            // debug << "id throw!: " << new_reg.pc << " " << ret.new_pc << std::endl;
            new_reg.pc = ret.new_pc;
            return 1; // re-IF
        }
        new_reg.pc = ret.new_pc; // used for branch hazard

        // debug << ":" << new_reg.pc << std::endl;

        // debug << ROUGH_NAMEs[ret.rough] << " " << ret.fine << " " << std::hex << " " << cmd.B.rs1 << "#" << ret.rs1 << " " << cmd.B.rs2 << "#" << ret.rs2 << " " << ret.new_pc << std::endl;
        return 0;

    }
};
const int Instruction_Decorder::OPCODE[] = {
    0b0110111, 0b0010111, 0b1101111, 0b1100111, // LUI, AUIPC, JAL, JALR
    0b1100011,                                  // BEQ, BNE, BLT, BGE, BATU, BGEU
    0b0000011,                                  // LB, LH, LW, LBU, LHU
    0b0100011,                                  // SB, SH, SW
    0b0010011,                                  // ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI
    0b0110011,                                  // ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
};

class Executor {
public:
    int exec(const Pipeline_Register &pr, Pipeline_Register &new_pr, const Register &reg, Register_Tmp &new_reg, Predictor &pred) {
        if(pr.id_ex.cmd.data == 0u) { new_pr.ex_mem = EX_MEM(); return 0; }
        if(pr.stalled == 2) {
            ++new_pr.stalled;
            return 0;
        }


        auto inst = pr.id_ex;
        EX_MEM ret(inst);
        // debug << "EXing..." << inst.rs1 << " " << inst.rs2 << std::endl;

        // Forwarding
        if(pr.mem_wb.inst.cmd.I.rd) { // not x0
            switch(pr.mem_wb.inst.rough) { // inst that get ans in MEM
            case LUI:
            case AUIPC:
            case JAL:
            case JALR:
            case R_I_func:
            case R_R_func:
            case LOAD_func: // here load inst can also affect the reg
                switch(inst.rough) {
                case JALR:
                case LOAD_func:
                case R_I_func: // rs1-only funcs
                    if(inst.cmd.R.rs1 == pr.mem_wb.inst.cmd.I.rd) {
                        // debug << "rs1 Forwarding: " << inst.rs1 << " " << pr.mem_wb.data << std::endl;
                        inst.rs1 = pr.mem_wb.data;
                    }
                    break;
                case B_func:
                case STORE_func:
                case R_R_func: // rs1 & rs2
                    if(inst.cmd.R.rs1 == pr.mem_wb.inst.cmd.I.rd) inst.rs1 = pr.mem_wb.data;
                    if(inst.cmd.R.rs2 == pr.mem_wb.inst.cmd.I.rd) {
                        // debug << "R_R forwarding2_: " << inst.rs2 << " " << pr.mem_wb.data << std::endl;
                        inst.rs2 = pr.mem_wb.data;
                    }
                    break;
                default: break;
                }
                break;
            default: break;
            }
        }
        if(pr.ex_mem.inst.cmd.I.rd) { // not x0
            switch(pr.ex_mem.inst.rough) { // inst that get ans in EX
            case LUI:
            case AUIPC:
            case JAL:
            case JALR:
            case R_I_func:
            case R_R_func: // no load inst
                switch(inst.rough) {
                case JALR:
                case LOAD_func:
                case R_I_func: // rs1-only funcs
                    if(inst.cmd.R.rs1 == pr.ex_mem.inst.cmd.I.rd) {
                        inst.rs1 = pr.ex_mem.ans;
                    }
                    break;
                case B_func:
                case STORE_func:
                case R_R_func: // rs1 & rs2
                    if(inst.cmd.R.rs1 == pr.ex_mem.inst.cmd.I.rd) {
                        // debug << "R_R forwarding1: " << pr.ex_mem.ans;
                        inst.rs1 = pr.ex_mem.ans;
                    }
                    if(inst.cmd.R.rs2 == pr.ex_mem.inst.cmd.I.rd) {
                        // debug << "R_R forwarding2: " << inst.rs2 << " " << pr.ex_mem.ans;
                        inst.rs2 = pr.ex_mem.ans;
                    }
                    break;
                default: break;
                }
                break;
            default: break;
            }
        }
        // Forwarding end


        switch(inst.rough) {
        case LUI:
            ret.ans = inst.imm;
            break;
        case AUIPC:
        case JAL:
            ret.ans = inst.branch_return; // if it's a branch inst, the return value is the branch_return
            break;
        case JALR:
            // debug << "JALR throw!" << std::endl;
            ret.ans = inst.new_pc; // original pc + 4
            new_reg.pc = inst.new_pc = ((inst.imm + inst.rs1) >> 1) << 1; // set the least bit to 0
            new_pr.ex_mem = ret;
            new_pr.id_ex = NOP;
            return 1; // re-IF
            break;
        case B_func:
            switch((B_FUNCs)inst.fine) {
            case BEQ:
                if(inst.rs1 == inst.rs2) inst.new_pc += inst.imm - 4;
                break;
            case BNE:
                if(inst.rs1 != inst.rs2) inst.new_pc += inst.imm - 4;
                break;
            case BLT:
                if(inst.rs1 < inst.rs2) inst.new_pc += inst.imm - 4;
                break;
            case BGE:
                if(inst.rs1 >= inst.rs2) inst.new_pc += inst.imm - 4;
                break;
            case BLTU:
                if((uint)inst.rs1 < (uint)inst.rs2) inst.new_pc += inst.imm - 4;
                break;
            case BGEU:
                if((uint)inst.rs1 >= (uint)inst.rs2) inst.new_pc += inst.imm - 4;
                break;
            }

        case LOAD_func:
            ret.addr = inst.imm + inst.rs1; // return the effective address
            break;

        case STORE_func:
            ret.addr = inst.imm + inst.rs1; // return the effective address
            ret.ans = inst.rs2;
            break;

        case R_I_func:
            switch((R_I_FUNCs)inst.fine) {
            case ADDI:  // add imm
                ret.ans = inst.rs1 + inst.imm;
                break;
            case SLLI:  // left shift
                ret.ans = inst.rs1 << (inst.imm & 31u);
                break;
            case SLTI:  // less than imm
                ret.ans = (inst.rs1 < inst.imm);
                break;
            case SLTIU: // less than imm(unsigned)
                ret.ans = ((uint)inst.rs1 < (uint)inst.imm);
                break;
            case XORI:  // xor imm
                ret.ans = inst.rs1 ^ inst.imm;
                break;
            case SRLI_SRAI: // right shift(logical & arithmetic)
                if(inst.cmd.R.funct7) ret.ans = inst.rs1 >> (inst.imm & 31u);
                else ret.ans = (uint)inst.rs1 >> (inst.imm & 31u);
                break;
            case ORI:   // or imm
                ret.ans = inst.rs1 | inst.imm;
                break;
            case ANDI:  // and imm
                ret.ans = inst.rs1 & inst.imm;
                break;
            }
            break;

        case R_R_func:
            switch((R_R_FUNCs)inst.fine) {
            case ADD_SUB:
                if(inst.cmd.R.funct7) ret.ans = inst.rs1 - inst.rs2;
                else ret.ans = inst.rs1 + inst.rs2;
                break;
            case SLL:
                ret.ans = inst.rs1 << (inst.rs2 & 31u);
                break;
            case SLT:
                ret.ans = (inst.rs1 < inst.rs2);
                break;
            case SLTU:
                ret.ans = ((uint)inst.rs1 < (uint)inst.rs2);
                break;
            case XOR:
                ret.ans = inst.rs1 ^ inst.rs2;
                break;
            case SRL_SRA:
                if(inst.cmd.R.funct7) ret.ans = inst.rs1 >> (inst.rs2 & 31u);
                else ret.ans = (uint)inst.rs1 >> (inst.rs2 & 31u);
                break;
            case OR:
                ret.ans = inst.rs1 | inst.rs2;
                break;
            case AND:
                ret.ans = inst.rs1 & inst.rs2;
                break;
            }
            break;
        }
        // debug << "EX_ans: " << ret.ans << std::endl;
        new_pr.ex_mem = ret;

        // Prediction feedback
        if(inst.rough == B_func) {
            if(inst.new_pc != pr.id_ex.new_pc) {
                // debug << "taken branch: " << pr.id_ex.new_pc - 4 << " " << inst.new_pc << std::endl;
                pred.feedback(pr.id_ex.new_pc - 4, inst.new_pc, true); // the branch is taken!
            } else {
                pred.feedback(pr.id_ex.new_pc - 4, inst.new_pc, false); // not taken
            }
            // pred.feedback_rate(inst.new_pc == inst.predicted_pc);
            if(inst.new_pc != inst.predicted_pc) { // the prediction is NOT correct!
                // debug << "branch throw! " << inst.new_pc << " " << inst.predicted_pc << std::endl;
                new_pr.id_ex = NOP; // let the next inst be NOP
                new_reg.pc = inst.new_pc; // renew the pc
                return 1;
            }
        }
        return 0;
    }
};

class Memory_Accesser {
public:
    int access(const Pipeline_Register &pr, Pipeline_Register &new_pr, Memory &mem) {
        if(pr.ex_mem.inst.cmd.data == 0u) { new_pr.mem_wb = MEM_WB(); return 0; }
        if(pr.stalled == 3) {
            ++new_pr.stalled;
            return 0;
        }
        // debug << "MEMing...";

        auto ex_mem = pr.ex_mem;
        MEM_WB ret(ex_mem.inst);
        ret.data = ex_mem.ans; // inherit the ans
        CMD tmp;
        switch(ret.inst.rough) {
        case LOAD_func:
            new_pr.clock += 2; // 3 cycles
            switch((LOAD_FUNCs)ret.inst.fine) {
            case LB:
                ret.data = (char)mem[ex_mem.addr];
                break;
            case LH:
                tmp.in.o3 = mem[ex_mem.addr++];
                tmp.in.o4 = mem[ex_mem.addr];
                ret.data = ((int)tmp.data >> 16);
                break;
            case LW:
                tmp.in.o1 = mem[ex_mem.addr++];
                tmp.in.o2 = mem[ex_mem.addr++];
                tmp.in.o3 = mem[ex_mem.addr++];
                tmp.in.o4 = mem[ex_mem.addr];
                ret.data = (int)tmp.data;
                break;
            case LBU:
                ret.data = (uchar)mem[ex_mem.addr];
                break;
            case LHU:
                tmp.in.o3 = mem[ex_mem.addr++];
                tmp.in.o4 = mem[ex_mem.addr];
                ret.data = ((uint)tmp.data >> 16);
                break;
            }
            // debug << "LOAD: " << ret.data << std::endl;
            break;

        case STORE_func:
            new_pr.clock += 2; // 3 cycles
            switch((STORE_FUNCs)ret.inst.fine) {
            case SB:
                mem[ex_mem.addr] = (uchar)(ret.data & 255u);
                break;
            case SH:
                tmp.data = ret.data;
                mem[ex_mem.addr++] = tmp.in.o1;
                mem[ex_mem.addr] = tmp.in.o2;
                break;
            case SW:
                tmp.data = ret.data;
                mem[ex_mem.addr++] = tmp.in.o1;
                mem[ex_mem.addr++] = tmp.in.o2;
                mem[ex_mem.addr++] = tmp.in.o3;
                mem[ex_mem.addr] = tmp.in.o4;
                break;
            }
            // debug << "STORE: " << ret.data << std::endl;
            break;

        default: break;
        }
        new_pr.mem_wb = ret;
        return 0;
    }
};

class Write_Backer {    // excellent name though
public:
    int write(const Pipeline_Register &pr, Pipeline_Register &new_pr, Register &reg, Register_Tmp &new_reg) {
        if(pr.mem_wb.inst.cmd.data == 0u) return 0;
        if(pr.stalled == 4) {
            new_pr.stalled = 0;
            return 0;
        }
        // debug << "WBing..." << std::endl;


        uint rd = pr.mem_wb.inst.cmd.R.rd;
        if(pr.mem_wb.inst.rough != B_func && pr.mem_wb.inst.rough != STORE_func) {
            if(rd) {
                new_reg.rd = rd;
                new_reg.new_value = pr.mem_wb.data; // discard modification on x0
            }
            // debug << rd << "#" << reg[rd] << std::endl;
        }
        // debug << "new_pc: " << reg.pc << std::endl << std::endl;
        // debug << std::endl;
        return 0;
    }
};


#endif