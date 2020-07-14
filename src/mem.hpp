#ifndef RISCV_MEM_HPP
#define RISCV_MEM_HPP

#include <iostream>
#include <cstring>
#include <string>
#include <cassert>
#include "type.hpp"

class Memory {
    static const int MAX_MEM = 0x20000;
    unsigned char *data;

    uint getHex(const std::string &s, uint i = 0) const {
        uint ret = 0;
        while(i < s.length()) {
            if(s[i] >= '0' && s[i] <= '9') ret = (ret << 4) | (s[i] - '0');
            else if(s[i] >= 'A' && s[i] <= 'F') ret = (ret << 4) | (s[i] - 'A' + 10);
            else assert(0);
            ++i;
        }
        return ret;
    }

public:
    Memory() {
        data = new unsigned char[MAX_MEM];
        for(int i = 0; i < MAX_MEM; ++i) data[i] = 0u;
    }
    ~Memory() { delete [] data; }
    unsigned char &operator[](const uint &p) { return data[p]; }
    const unsigned char &operator[](const uint &p) const { return data[p]; }
    void init(std::istream &in) {
        uint p = 0;
        std::string s;
        while(!in.eof()) {
            in >> s;
            if(s[0] == '@') {
                p = getHex(s, 1);
            } else
                data[p++] = getHex(s, 0);
        }
    }

};

class Register {
public:
    int reg[32], pc;
    Register() { memset(reg, 0, sizeof(reg)); pc = 0; }
    int &operator[](const uint &p) { return reg[p]; }
    const int &operator[](const uint &p) const { return reg[p]; }
};


#endif