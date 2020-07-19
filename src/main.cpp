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

//************* global vars *************
Register _reg_;
Memory _mem_;
//***************************************

int main() {
    Instruction_Fetcher IF;
    Instruction_Decorder ID;
    Executor EX;
    Memory_Accesser MEM;
    Write_Backer WB;

    Pipeline_Register PR;
    Pipeline_Register PR_tmp;
    Predictor PRED;

    bool used[4] = {0}; // whether or not an inst is in pipeline

    Register_Tmp reg_tmp; // simulate the "simutaneous" pipeline by modifying reg_tmp in stages rather than write into _reg_ directly

    _mem_.init(cin);
    int tmp;
    while(true) {
        // debug << PR.clock << " ------------" << endl;
        ++PR_tmp.clock;

        WB.write(PR, PR_tmp, _reg_, reg_tmp);
        if(tmp = MEM.access(PR, PR_tmp, _mem_)) {
            PR_tmp.MEM_stalled = tmp;
            PR = PR_tmp;
            _reg_ = reg_tmp;
            continue;
        }
        if(tmp = EX.exec(PR, PR_tmp, _reg_, reg_tmp, PRED)) {
            PR_tmp.stalled = tmp;
            PR = PR_tmp;
            _reg_ = reg_tmp;
            continue;
        }
        if(tmp = ID.decode(PR, PR_tmp, _reg_, reg_tmp, PRED)) {
            PR_tmp.stalled = tmp;
            PR = PR_tmp;
            _reg_ = reg_tmp;
            continue;
        }
        if(!PR.end_flag) IF.fetch(PR_tmp, _reg_, reg_tmp, _mem_, PRED);


        // cout << "catched: " << _throw.stall_num << endl;

        _reg_ = reg_tmp;
        PR = PR_tmp;
        if(PR.end_flag && PR.mem_wb.inst.cmd.data == 0u) break; // have done the last WB: quit
    }
    cout << std::dec << (((uint)_reg_[10]) & 255u) << endl;
    // cout << PRED.get_rate() << endl;
    return 0;
}