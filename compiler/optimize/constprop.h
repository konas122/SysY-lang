#ifndef __OPTIMIZE_CONSTPROP_H__
#define __OPTIMIZE_CONSTPROP_H__

#include "common.h"

#define UNDEF   0.5     // 表示变量未定义, 未初始化
#define NAC     -0.5    // 表示变量确定是无常量值的


class Var;
class DFG;
class Block;
class SymTab;
class InterInst;


//  常量传播数据流分析框架
class ConstPropagation
{
    SymTab *tab;        // 符号表
    DFG *dfg;           // 数据流图指针

    std::vector<Var *> vars;        // 变量集合
    std::vector<Var *> glbVars;     // 全局变量的集合, G 处理函数调用等附加影响操作
    std::vector<double> boundVals;  // 边界集合, Entry.out
    std::vector<double> initVals;   // 初值集合,B.out/B.out

    void join(Block *block);        // 交汇运算
    static double join(double left, double right);  // 元素交汇运算

    void translate(InterInst *inst, const std::vector<double> &in, std::vector<double> &out);   // 单指令传递函数
    bool translate(Block *block);           // 传递函数 fB

    void analyse();         // 常量传播分析
    void algebraSimplify(); // 代数化简
    void condJmpOpt();      // 条件跳转优化, 同时进行不可达代码消除

public:
    ConstPropagation(DFG *g, SymTab *tab, std::vector<Var *> &paraVar); // 常量传播分析初始化
    void propagate();   // 执行常量传播
};

#endif
