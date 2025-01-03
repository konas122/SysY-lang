#include "ir/interInst.h"
#include "ir/interCode.h"

#include "dfg.h"

using namespace std;


Block::Block(const vector<InterInst *> &codes) 
    : visited(false), canReach(true)
{
    for (auto code : codes) {
        code->block = this;
        insts.emplace_back(code);
    }
}

void Block::toString() {
    printf("-----------%8p----------\n", this);
    printf("Prev: ");
    for (list<Block *>::iterator i = prevs.begin(); i != prevs.end(); ++i) {
        printf("%8p ", *i);
    }
    printf("\n");
    printf("Next: ");
    for (list<Block *>::iterator i = succs.begin(); i != succs.end(); ++i) {
        printf("%8p ", *i);
    }
    printf("\n");
    for (list<InterInst *>::iterator i = insts.begin(); i != insts.end(); ++i) {
        (*i)->toString();
    }
    printf("-----------------------------\n");
}


DFG::DFG(InterCode &code): codeList(code.getCode())
{
    code.markFirst();           // 标识首指令
    createBlocks();             // 创建基本块
    linkBlocks();               // 链接基本块关系
}

void DFG::createBlocks() {
    vector<InterInst *> tmpList;

    for (auto code : codeList) {
        if (tmpList.empty() && code->isFirst()) {
            tmpList.emplace_back(code); // 添加第一条首指令
            continue;
        }
        if (!tmpList.empty()) {
            if (code->isFirst()) {
                blocks.emplace_back(new Block(tmpList));
                tmpList.clear();
            }
            tmpList.emplace_back(code);
        }
    }
    blocks.emplace_back(new Block(tmpList));
}

void DFG::linkBlocks() {
    // 链接基本块顺序关系
    for (size_t i = 0; i < blocks.size() - 1; ++i) {    // 后继关系
        InterInst *last = blocks[i]->insts.back();      // 当前基本块的最后一条指令
        if (!last->isJmp()) {                           // 不是直接跳转, 可能顺序执行
            blocks[i]->succs.emplace_back(blocks[i + 1]);
        }
    }

    for (size_t i = 1; i < blocks.size(); ++i) {        // 前驱关系
        InterInst *last = blocks[i - 1]->insts.back();  // 前个基本块的最后一条指令
        if (!last->isJmp()) {                           // 不是直接跳转, 可能顺序执行
            blocks[i]->prevs.emplace_back(blocks[i - 1]);
        }
    }

    for (size_t i = 0; i < blocks.size(); ++i) {    // 跳转关系
        InterInst *last = blocks[i]->insts.back();  // 基本块的最后一条指令
        if (last->isJmp() || last->isJcond()) {     // (直接/条件) 跳转
            blocks[i]->succs.emplace_back(last->getTarget()->block);    // 跳转目标块为后继
            last->getTarget()->block->prevs.emplace_back(blocks[i]);    // 相反为前驱
        }
    }
}

DFG::~DFG() {
    for (auto block : blocks) {
        delete block;
    }
    for (auto code : newCode) {
        delete code;
    }
}

bool DFG::reachable(Block *block) {
    if (block == blocks[0]) {
        return true;
    }
    else if (block->visited) {
        return false;
    }

    block->visited = true;
    bool flag = false;
    for (auto prev : block->prevs) {
        flag = reachable(prev);
        if (flag) {
            break;
        }
    }
    return flag;
}

void DFG::release(Block *block) {
    if (!reachable(block)) {
        list<Block *> delList;
        for (auto succ : block->succs) {
            delList.emplace_back(succ);
        }

        for (auto del : delList) {
            block->succs.remove(del);
            del->prevs.remove(block);
        }

        for (auto del : delList) {
            release(del);
        }
    }
}

void DFG::delLink(Block *begin, Block *end) {
    resetVisit();
    if (begin) {
        begin->succs.remove(end);
        end->prevs.remove(begin);
    }
    release(end);
}

void DFG::resetVisit() {
    for (auto block : blocks) {
        block->visited = false;
    }
}

void DFG::toCode(list<InterInst *>& opt) {
    opt.clear();
    for (auto block : blocks) {
        resetVisit();
        if (reachable(block)) {
            list<InterInst *> tmpInsts;
            for (auto inst : block->insts) {
                if (inst->isDead) { // 跳过死代码
                    continue;
                }
                tmpInsts.emplace_back(inst);
            }
            opt.splice(opt.end(),tmpInsts); // 合并有效基本块
        }
        else {
            block->canReach = false;
        }
    }
}

void DFG::toString() {
    for (auto block : blocks) {
        block->toString();
    }
}
