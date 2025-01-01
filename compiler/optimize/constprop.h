#ifndef __OPTIMIZE_CONSTPROP_H__
#define __OPTIMIZE_CONSTPROP_H__

#include "common.h"

class Var;
class DFG;
class SymTab;

//  常量传播数据流分析框架
class ConstPropagation
{

public:
    ConstPropagation(DFG *g, SymTab *tab, std::vector<Var *> &paraVar); // 常量传播分析初始化
    void propagate();                                              // 执行常量传播
};

#endif
