#ifndef __OPTIMIZE_COPYPROP_H__
#define __OPTIMIZE_COPYPROP_H__

#include "optimize/set.h"


class Var;
class DFG;
class Block;
class InterInst;


// 复写传播数据流分析框架
class CopyPropagation
{
    DFG *dfg;
    std::list<InterInst *> optCode;
    std::vector<InterInst *> copyExpr;

    Set U;  // 全集
    Set E;  // 空集

    void analyse();                 // 复写传播数据流分析
    bool translate(Block *block);   // 复写传播传递函数

    Var *__find(const Set &in, Var *var, Var *src); // 递归检测 var 赋值的源头的内部实现
    Var *find(const Set &in, Var *var);             // 递归检测 var 赋值的源头

public:
    CopyPropagation(DFG *g);
    void propagate();   // 执行复写传播
};

#endif
