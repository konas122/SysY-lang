#ifndef __COMPILER_SELECTOR_H__
#define __COMPILER_SELECTOR_H__

#include "common.h"

class ILoc;
class InterInst;


class Selector
{
    std::vector<InterInst *> &ir;
    ILoc &iloc;
    void translate(InterInst *inst);
public:
    Selector(std::vector<InterInst *> &ir, ILoc &iloc);
    void select();
};

#endif
