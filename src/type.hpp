#ifndef RISCV_TYPE_HPP
#define RISCV_TYPE_HPP

#include <iostream>

typedef unsigned int uint;
typedef unsigned char uchar;

union CMD {
    friend std::istream &operator>>(std::istream &is, CMD &cmd) {
        int tmp;
        std::cin >> std::hex >> tmp; cmd.in.o1 = tmp;
        std::cin >> std::hex >> tmp; cmd.in.o2 = tmp;
        std::cin >> std::hex >> tmp; cmd.in.o3 = tmp;
        std::cin >> std::hex >> tmp; cmd.in.o4 = tmp;
        return is;
    }
    CMD() { data = 0; }

    unsigned int data;
    struct IN {
        uint o1: 8;
        uint o2: 8;
        uint o3: 8;
        uint o4: 8;
    }in;

    struct R_type {
        uint opcode: 7;
        uint rd:     5;
        uint funct3: 3;
        uint rs1:    5;
        uint rs2:    5;
        uint funct7: 7;
    }R;

    struct I_type {
        uint opcode: 7;
        uint rd:     5;
        uint funct3: 3;
        uint rs1:    5;
        int imm:    12;

        int getImm() const {
            return imm;
        }
    }I;

    struct S_type {
        uint opcode: 7;
        uint imm_4_0:  5;
        uint funct3: 3;
        uint rs1:    5;
        uint rs2:    5;
        int imm_11_5:  7;

        int getImm() const {
            return (imm_11_5 << 5) | imm_4_0;
        }
    }S;

    struct B_type {
        uint opcode:     7;
        uint imm_11:     1;
        uint imm_4_1:    4;
        uint funct3:     3;
        uint rs1:        5;
        uint rs2:        5;
        uint imm_10_5:   6;
        int imm_12:     1;

        int getImm() const {
            return (imm_12 << 12) | (imm_11 << 11) | (imm_10_5 << 5) | (imm_4_1 << 1);
        }
    }B;

    struct U_type {
        uint opcode:     7;
        uint rd:         5;
        int imm_31_12:  20;

        int getImm() const {
            return imm_31_12 << 12;
        }
    }U;

    struct J_type {
        uint opcode:     7;
        uint rd:         5;
        uint imm_19_12:  8;
        uint imm_11:     1;
        uint imm_10_1:   10;
        int imm_20:     1;

        int getImm() const {
            return (imm_20 << 20) | (imm_19_12 << 12) | (imm_11 << 11) | (imm_10_1 << 1);
        }
    }J;

};

// enum TYPE {
//     R_, I_, S_, B_, U_, J_
// };
enum ROUGH_FUNCs { // roughly classify the instruction type
    LUI, AUIPC, JAL, JALR,
    B_func,
    LOAD_func,
    STORE_func,
    R_I_func,
    R_R_func
};
const char ROUGH_NAMEs[][11] = {
    "LUI", "AUIPC", "JAL", "JALR",
    "B_func",
    "LOAD_func",
    "STORE_func",
    "R_I_func",
    "R_R_func"
};
enum B_FUNCs { BEQ = 0, BNE, BLT = 4, BGE, BLTU, BGEU }; // according to funct3
enum LOAD_FUNCs { LB = 0, LH, LW, LBU = 4, LHU };
enum STORE_FUNCs { SB = 0, SH, SW };
enum R_I_FUNCs { ADDI = 0, SLLI, SLTI, SLTIU, XORI, SRLI_SRAI, ORI, ANDI };
enum R_R_FUNCs { ADD_SUB = 0, SLL, SLT, SLTU, XOR, SRL_SRA, OR, AND };

class Instruction {
public:
    CMD cmd;
    ROUGH_FUNCs rough;  // rough type
    uint fine;         // fine type in accord with XX_FUNCs
    int imm;           // immediate number
    int rs1, rs2;      // store the VALUE of rs1, rs2

    int branch_return;

    Instruction(const CMD &_cmd) {
        cmd = _cmd;
        imm = rs1 = rs2 = 0;
        branch_return = 0;
    }
};

#endif