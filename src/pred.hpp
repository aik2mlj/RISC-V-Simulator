#ifndef RISCV_PRED_HPP
#define RISCV_PRED_HPP
#include "type.hpp"
#include <iostream>
#include <cstring>

class Predictor {
    static const uint M = 1 << 14;
    union Two_Bit {
        uint data;
        struct {
            uint o1: 1;
            uint o2: 1;
        };
    }tb[M];
    uint jump_pc[M];

    uint total_num = 0;
    uint success_num = 0;
public:
    Predictor() { memset(jump_pc, 0, sizeof(jump_pc)); }
    void feedback(const uint &pc, const uint &jp, const bool &taken) {
        uint hash_pc = pc & (M - 1); // get the lower 14bit for hash
        if(!jump_pc[hash_pc]) jump_pc[hash_pc] = jp;      // store the jump_pc
        if(taken) {
            if(tb[hash_pc].data < 3) ++tb[hash_pc].data;
        } else {
            if(tb[hash_pc].data > 0) --tb[hash_pc].data;
        }
    }
    bool predict(const uint &pc, uint &jp) const {
        uint hash_pc = pc & (M - 1);
        jp = jump_pc[hash_pc];
        if(!jump_pc[hash_pc]) {
            return false;
        }
        if(tb[hash_pc].o2) return true;
        else return false;
    }
    void feedback_rate(const bool &correct) {
        ++total_num;
        if(correct) ++success_num;
    }
    double get_rate() const { return (double)success_num / total_num; }
};

#endif