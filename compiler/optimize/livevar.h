#ifndef __OPTIMIZE_LIVEVAR_H__
#define __OPTIMIZE_LIVEVAR_H__

#include "optimize/set.h"


class Var;
class DFG;
class Block;
class SymTab;
class InterInst;


// 活跃变量数据流分析框架
class LiveVar
{
    SymTab *tab;    // 符号表
    DFG *dfg;       // 数据流图指针

    std::vector<Var *> varList;     // 变量列表
    std::list<InterInst *> optCode; // 记录前面阶段优化后的代码

    Set U;  // 全集
    Set E;  // 空集
    Set G;  // 全局变量集

    bool translate(Block* block);   // 活跃变量传递函数

public:
    LiveVar(DFG *g, SymTab *t, const std::vector<Var *> &paraVar);

    void analyse(); // 活跃变量数据流分析
    void elimateDeadCode(int stop = false); // 死代码消除

    Set &getE();    // 返回空集
    std::vector<Var*> getCoVar(const Set &liveout) const;   // 根据提供的 liveout 集合提取优化后的变量集合 (冲突变量)
};

#endif
