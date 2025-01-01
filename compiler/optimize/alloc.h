#ifndef __OPTIMIZE_ALLOC_H__
#define __OPTIMIZE_ALLOC_H__

#include "common.h"

class Var;
class Fun;
class LiveVar;
class InterInst;


class CoGraph
{
public:
    CoGraph(std::list<InterInst *> &optCode, std::vector<Var *> &para, LiveVar *lv, Fun *f);
    ~CoGraph();
    void alloc();
};

#endif
