#ifndef __IR_INTERCODE_H__
#define __IR_INTERCODE_H__

#include "common.h"

class InterInst;


class InterCode
{
    std::vector<InterInst *> code;

public:
    ~InterCode();
    void addInst(InterInst *inst);
    void markFirst();
    void toString();
    std::vector<InterInst *> &getCode();
};

#endif
