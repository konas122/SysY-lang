#ifndef __IR_INTERCODE_H__
#define __IR_INTERCODE_H__

#include <memory>

#include "common.h"

class InterInst;


class InterCode
{
    std::vector<std::shared_ptr<InterInst>> code;

public:
    InterCode() = default;

    const InterCode &operator=(const InterCode &&rhs) = delete;

    ~InterCode();
    void addInst(std::shared_ptr<InterInst> inst);
    void markFirst();
    void toString() const;
    std::vector<std::shared_ptr<InterInst>> &getCode();
};

#endif
