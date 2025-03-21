#ifndef __OPTIMIZE_COPYPROP_H__
#define __OPTIMIZE_COPYPROP_H__

#include <memory>

#include "optimize/set.h"


class Var;
class DFG;
class Block;
class InterInst;


// 复写传播数据流分析框架
class CopyPropagation
{
    std::shared_ptr<DFG> dfg;
    std::list<std::shared_ptr<InterInst>> optCode;
    std::vector<std::shared_ptr<InterInst>> copyExpr;

    Set U;  // 全集
    Set E;  // 空集

    void analyse();                 // 复写传播数据流分析
    bool translate(std::shared_ptr<Block> block);   // 复写传播传递函数

    Var *__find(const Set &in, Var *var, Var *src) const;   // 递归检测 var 赋值的源头的内部实现
    Var *find(const Set &in, Var *var) const;               // 递归检测 var 赋值的源头

public:
    explicit CopyPropagation(std::shared_ptr<DFG> g);
    void propagate();   // 执行复写传播
};

#endif
