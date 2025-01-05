#include "symbol/var.h"
#include "symbol/symtab.h"

#include "ir/interInst.h"
#include "optimize/dfg.h"

#include "livevar.h"

using namespace std;

LiveVar::LiveVar(DFG *g, SymTab *t, const vector<Var *>& paraVar) : tab(t), dfg(g)
{
    varList = tab->getGlbVars();
    int glbNum = varList.size();
    std::copy(paraVar.begin(), paraVar.end(), std::back_inserter(varList));
    dfg->toCode(optCode);

    // 统计所有的局部变量
    for (auto inst : optCode) {
        Operator op = inst->getOp();
        if (op == Operator::OP_DEC) {
            varList.emplace_back(inst->getArg1());
        }
    }

    U.init(varList.size(), 1);
    E.init(varList.size(), 0);
    G = E;
    // 初始化全局变量集合
    for (int i = 0; i < glbNum; ++i) {
        G.set(i);
    }

    // 记录变量的列表索引
    for (size_t i = 0; i < varList.size(); ++i) {
        varList[i]->index = i;
    }

    // 初始化指令的指令的 use 和 def 集合
    for (auto inst : optCode) {
        inst->liveInfo.use = E;
        inst->liveInfo.def = E;

        Operator op = inst->getOp();
        Var *rs = inst->getResult();
        const Var *arg1 = inst->getArg1();
        const Var *arg2 = inst->getArg2();

        // 常规运算
        if (op >= Operator::OP_AS && op <= Operator::OP_LEA) {
            inst->liveInfo.use.set(arg1->index);        // 使用 arg1
            if (arg2) {
                inst->liveInfo.use.set(arg2->index);    // 使用 arg2
            }
            if (rs != arg1 && rs != arg2) {
                inst->liveInfo.def.set(rs->index);      // 定值 result
            }
        }
        else if (op == Operator::OP_SET) {      // *arg1 = result
            inst->liveInfo.use.set(rs->index);  // 使用 result
        }
        else if (op == Operator::OP_GET) {  // result = *arg1
            inst->liveInfo.use = U;         // 可能使用了任何变量, 保守认为使用了所有变量
        }
        else if (op == Operator::OP_RETV) { // return arg1
            inst->liveInfo.use.set(arg1->index);
        }
        else if (op == Operator::OP_ARG) {
            if (arg1->isBase()) {
                inst->liveInfo.use.set(arg1->index);
            }
            else {
                inst->liveInfo.use = U; // 可能使用全部的变量, 保守认为没有杀死所有变量
            }
        }
        else if (op == Operator::OP_CALL || op == Operator::OP_PROC) {
            inst->liveInfo.use = G; // 可能使用了所有的全局变量, 保守认为没有杀死所有变量
            if (rs && rs->getPath().size() > 1) {   // 有返回值, 且返回值不是全局变量
                inst->liveInfo.def.set(rs->index);  // 定值了返回值 result
            }
        }
        else if (op == Operator::OP_JF || op == Operator::OP_JT) {
            inst->liveInfo.use.set(arg1->index);    // 使用条件 arg1
        }
    }
}

// 活跃变量传递函数: f(x)=(x - def(B)) | use(B)
bool LiveVar::translate(Block *block) {
    Set tmp = block->liveInfo.out;

    for (auto i = block->insts.rbegin(); i != block->insts.rend(); ++i) {
        InterInst *inst = *i;
        if (inst->isDead) {
            continue;
        }
        Set &in = inst->liveInfo.in;
        Set &out = inst->liveInfo.out;
        out = tmp;
        in = inst->liveInfo.use | (out - inst->liveInfo.def);
        tmp = in;
    }

    bool flag = tmp != block->liveInfo.in;
    block->liveInfo.in = tmp;
    return flag;
}


void LiveVar::analyse() {
    // 解除寄存器分配时随意取变量 liveout 集合的 bug
    dfg->blocks.back()->liveInfo.out = E;
    translate(dfg->blocks.back());

    for (auto block : dfg->blocks) {
        block->liveInfo.in = E;
    }

    bool change = true;
    while (change) {
        change = false;
        for (int i = dfg->blocks.size() - 2; i >= 0; --i) {
            if (!dfg->blocks[i]->canReach) {
                continue;
            }

            Set tmp = E;
            for (auto block : dfg->blocks[i]->succs) {
                tmp = tmp | block->liveInfo.in;
            }

            dfg->blocks[i]->liveInfo.out = tmp;
            if (translate(dfg->blocks[i])) {
                change = true;
            }
        }
    }
}

vector<Var *> LiveVar::getCoVar(const Set &liveout) const {
    vector<Var *> coVar;
    for (size_t i = 0; i < varList.size(); i++) {
        if (liveout.get(i)) {
            coVar.emplace_back(varList[i]);    // 将活跃的变量保存
        }
    }
    return coVar;
}

Set& LiveVar::getE() {
    return E;
}

// 死代码消除
void LiveVar::elimateDeadCode(int stop) {
    if (stop) { // 没有新的死代码生成, 清理多余的声明

        // 计算哪些变量存在非死代码中, 记录变量活跃性
        for (auto inst : optCode) {
            Operator op = inst->getOp();
            if (inst->isDead || op == Operator::OP_DEC) {
                continue;
            }
            Var *rs = inst->getResult();
            Var *arg1 = inst->getArg1();
            Var *arg2 = inst->getArg2();
            if (rs) {
                rs->live = true;
            }
            if (arg1) {
                arg1->live = true;
            }
            if (arg2) {
                arg2->live = true;
            }
        }
        // 删除无效的 DEC 命令
        for (auto inst : optCode) {
            Operator op = inst->getOp();
            if (op == Operator::OP_DEC) {
                const Var *arg1 = inst->getArg1();
                if (!arg1->live) {
                    inst->isDead = true;
                }
            }
        }
        return;
    }

    stop = true;    // 希望停止
    analyse();

    // 清洗一次代码
    for (auto inst : optCode) {
        if (inst->isDead) {
            continue;
        }
        Operator op = inst->getOp();
        Var *rs = inst->getResult();
        Var *arg1 = inst->getArg1();

        if ((op >= Operator::OP_AS && op <= Operator::OP_LEA) || op == Operator::OP_GET) {
            if (rs->getPath().size() == 1) {
                continue;   // 全局变量不作处理
            }
            // 出口不活跃或者是 a=a 的形式 (来源于复写传播的处理结果)
            if (!inst->liveInfo.out.get(rs->index) || (op == Operator::OP_AS && rs == arg1)) {
                inst->isDead = true;
                stop = false;   // 识别新的死代码
            }
        }
        else if (op == Operator::OP_CALL) {
            // 处理函数返回值, 若函数返回值不活跃, 则将 call 指令替换为 proc 指令
            if (!inst->liveInfo.out.get(rs->index)) {
                inst->callToProc();
            }
        }
    }
    elimateDeadCode(stop);
}
