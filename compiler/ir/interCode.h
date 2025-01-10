#ifndef __IR_INTERCODE_H__
#define __IR_INTERCODE_H__

#include "common.h"

class InterInst;


class InterCode
{
    std::vector<InterInst *> code;

public:
    InterCode() = default;

    InterCode(const InterCode &rhs) = delete;
    InterCode &operator=(const InterCode &rhs) = delete;

    ~InterCode();
    void addInst(InterInst *inst);
    void markFirst();
    void toString() const;
    std::vector<InterInst *> &getCode();
};

#endif
