## RISCV

### bit field

```c++
union {
    unsigned int a;
    struct {
        unsigned int o1: 5; // bit_field
        unsigned int o2: 20;
    }check;
}u;
```

https://www.geeksforgeeks.org/bit-fields-c/

### 主流程

1. IF: send pc to mem and fetch the current instruction from mem
2. ID:
   1. read the regs
   2. do equality test on the regs(for possible branch)
   3. sign-extend the offset/imm
   4. compute possible branch target(offset + pc)
3. EX:
   1. Mem reference
   2. R-R
   3. R-I

4. MA：

   load & store funcs：access the mem

5. WB：

   not store or branch funcs：write back to regs

#### 面向对象的思维

不要直接写函数，考虑封装成一个类

类的好处：方便的传值，存储临时值，直接调用

### 五级流水

some observations:

1. separate instruction and data memories
2. perform the register write in the first half of the clock cycle and the read in the second half (in ID & WB respectively)

3. **edge-triggered property**

use **pipeline register**, which is a register between two steps that stores the intermediate results

#### Hazard

- Data hazards: depend on previous results
  - stall the next inst
  - FORWARDING
- Control hazards: change pc (branch, etc.)

**剪切板有毒！剪切板有毒！不要直接从剪切板读入，从文件读入**

**throw只用来抛出异常！不要用它实现一般的功能！奇慢无比！**