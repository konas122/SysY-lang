#include "ir/interInst.h"

#include "interCode.h"

using namespace std;


void InterCode::addInst(InterInst *inst) {
    code.emplace_back(inst);
}

void InterCode::toString() const {
    for (size_t i = 0; i < code.size(); i++) {
        code[i]->toString();
    }
}

InterCode::~InterCode() {
    for (size_t i = 0; i < code.size(); i++) {
        delete code[i];
    }
}

void InterCode::markFirst() {
    size_t len = code.size();   // 指令个数, 最少为 2

    // 标识 Entry 与 Exit
    code[0]->setFirst();
    code[len - 1]->setFirst();

    // 标识第一条实际指令, 如果有的话
    if (len > 2) {
        code[1]->setFirst();
    }

    // 标识第 1 条实际指令到倒数第 2 条指令
    for (size_t i = 1; i < len - 1; ++i) {
        if (code[i]->isJmp() || code[i]->isJcond()) {
            code[i]->getTarget()->setFirst();
            code[i + 1]->setFirst();
        }
    }
}

vector<InterInst *> &InterCode::getCode() {
    return code;
}
