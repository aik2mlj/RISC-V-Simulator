#ifndef RISCV_FUNC_HPP
#define RISCV_FUNC_HPP

#include <iostream>
#include <cassert>
#include <iomanip>
#include "type.hpp"
#include "mem.hpp"

class Instruction_Fetcher {
public:
    CMD fetch(Register &reg, const Memory &mem) {
        CMD newcmd;
        newcmd.in.o1 = mem[reg.pc++];
        newcmd.in.o2 = mem[reg.pc++];
        newcmd.in.o3 = mem[reg.pc++];
        newcmd.in.o4 = mem[reg.pc++];
        // std::cerr << std::setw(8) << std::setfill('0') << std::hex << newcmd.data << std::endl;
        return newcmd;
    }
};

class Instruction_Decorder {
    static const int OPCODE[9];

public:
    Instruction decode(const CMD &cmd, const Register &reg) {
        Instruction ret;
        switch(cmd.B.opcode) {
        case 0b0110111:
            ret.rough = LUI;
            ret.imm = cmd.U.getImm();
            break;
        case 0b0010111:
            ret.rough = AUIPC;
            ret.imm = cmd.U.getImm();
            break;
        case 0b1101111:
            ret.rough = JAL;
            ret.imm = cmd.J.getImm();
            break;
        case 0b1100111:
            ret.rough = JALR;
            ret.imm = cmd.I.getImm();
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
            ret.rough = R_R_func; // no Imm
            break;
        default:
            assert(0); break;
        }
        ret.fine = cmd.B.funct3;
        ret.rs1 = reg[cmd.B.rs1];
        ret.rs2 = reg[cmd.B.rs2];
        ret.pc = reg.pc;
        return ret;
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

class EX_MEM { // connect EX & MEM
public:
    uint new_pc;
    int ans;
    EX_MEM(const uint _pc) { new_pc = _pc; ans = 0; }
};

class Executor {
public:
    EX_MEM exec(const Instruction &inst) {
        EX_MEM ret(inst.pc);
        switch(inst.rough) {
        case LUI:
            ret.ans = inst.imm;
            break;
        case AUIPC:
            ret.new_pc += inst.imm;
            ret.ans = ret.new_pc;
            break;
        case JAL:
            ret.new_pc += inst.imm;
            ret.ans = ret.new_pc + 4;
            break;
        case JALR:
            ret.new_pc = (inst.imm + inst.rs1) & 0;
            ret.ans = ret.new_pc + 4;
            break;

        case B_func:
            switch((B_FUNCs)inst.fine) {
            case BEQ:
                if(inst.rs1 == inst.rs2) ret.new_pc += inst.imm;
                break;
            case BNE:
                if(inst.rs1 != inst.rs2) ret.new_pc += inst.imm;
                break;
            case BLT:
                if(inst.rs1 < inst.rs2) ret.new_pc += inst.imm;
                break;
            case BGE:
                if(inst.rs1 >= inst.rs2) ret.new_pc += inst.imm;
                break;
            case BLTU:
                if((uint)inst.rs1 < (uint)inst.rs2) ret.new_pc += inst.imm;
                break;
            case BGEU:
                if((uint)inst.rs1 >= (uint)inst.rs2) ret.new_pc += inst.imm;
                break;
            }
            break;

        case LOAD_func:
            ret.ans = inst.imm + inst.rs1; // return the effective address
            break;

        case STORE_func:
            ret.ans = inst.imm + inst.rs1; // return the effective address
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
        return ret;
    }
};

class MEM_WB { // connect MEM & WB
public:
    uint new_pc;
    int data;
    MEM_WB(const uint _pc) { new_pc = _pc; data = 0; }
};

class Memory_Accesser {
public:
    MEM_WB access(const Instruction &inst, EX_MEM ex_mem, Memory &mem) {
        MEM_WB ret(ex_mem.new_pc);
        ret.data = ex_mem.ans; // inherit the ans
        CMD tmp;
        switch(inst.rough) {
        case LOAD_func:
            switch((LOAD_FUNCs)inst.fine) {
            case LB:
                ret.data = (char)mem[ex_mem.ans];
                break;
            case LH:
                tmp.in.o3 = mem[ex_mem.ans++];
                tmp.in.o4 = mem[ex_mem.ans];
                ret.data = ((int)tmp.data >> 16);
                break;
            case LW:
                tmp.in.o1 = mem[ex_mem.ans++];
                tmp.in.o2 = mem[ex_mem.ans++];
                tmp.in.o3 = mem[ex_mem.ans++];
                tmp.in.o4 = mem[ex_mem.ans];
                ret.data = (int)tmp.data;
                break;
            case LBU:
                ret.data = (uchar)mem[ex_mem.ans];
                break;
            case LHU:
                tmp.in.o3 = mem[ex_mem.ans++];
                tmp.in.o4 = mem[ex_mem.ans];
                ret.data = ((uint)tmp.data >> 16);
                break;
            }
            break;

        case STORE_func:
            switch((STORE_FUNCs)inst.fine) {
            case SB:
                mem[ex_mem.ans] = (uchar)(inst.rs2 & 255u);
                break;
            case SH:
                tmp.data = inst.rs2;
                mem[ex_mem.ans++] = tmp.in.o1;
                mem[ex_mem.ans] = tmp.in.o2;
                break;
            case SW:
                tmp.data = inst.rs2;
                mem[ex_mem.ans++] = tmp.in.o1;
                mem[ex_mem.ans++] = tmp.in.o2;
                mem[ex_mem.ans++] = tmp.in.o3;
                mem[ex_mem.ans] = tmp.in.o4;
                break;
            }
            break;

        default: break;
        }
        return ret;
    }
};

class Write_Backer {    // excellent name though
public:
    void write(const Instruction &inst, MEM_WB mem_wb, Register &reg) {
        reg.pc = mem_wb.new_pc; // write back pc
        int rd = inst.cmd.R.rd;
        if(inst.rough != B_func && inst.rough != STORE_func)
            reg[rd] = mem_wb.data;
    }
};


#endif