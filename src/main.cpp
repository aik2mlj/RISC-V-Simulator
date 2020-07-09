#include <cstdio>
#include <cstring>
#include <iostream>
#include <bitset>
#include "func.hpp"
#include "mem.hpp"
#include "type.hpp"
using std::cin;
using std::cout;
using std::endl;
using std::hex;

const int END_CMD = 0x0ff00513;

//************* global vars *************
Register _reg_;
Memory _mem_;
//***************************************

int main() {
    Instruction_Fetcher IF;
    Instruction_Decorder ID;
    Executor EX;
    Memory_Accesser MA;
    Write_Backer WB;

    _mem_.init(cin);
    while(true) {
        auto cmd = IF.fetch(_reg_, _mem_);
        if(cmd.data == END_CMD) {
            cout << (((uint)_reg_[10]) & 255u) << endl;
            break;
        }
        auto inst = ID.decode(cmd, _reg_);
        auto ex_mem_tmp = EX.exec(inst);
        // std::cerr << "ex_mem.ans: " << ex_mem_tmp.ans << endl;
        auto mem_wb_tmp = MA.access(inst, ex_mem_tmp, _mem_);
        // std::cerr << "mem_wb.ans: " << mem_wb_tmp.data << endl;
        WB.write(inst, mem_wb_tmp, _reg_);
        // std::cerr << "wriiten ra:" << _reg_[1] << endl;
    }
    return 0;
}