#include "ir/interCode.h"
#include "ir/interInst.h"
#include "optimize/dfg.h"

#include "copyprop.h"

using namespace std;


CopyPropagation::CopyPropagation(DFG *g) : dfg(g)
{
    dfg->toCode(optCode);   // 提取中间代码

    for (auto inst : optCode) {
        Operator op = inst->getOp();
        if (op == Operator::OP_AS) {
            copyExpr.emplace_back(inst);
        }
    }

    U.init(copyExpr.size(), 1);
    E.init(copyExpr.size(), 0);

    for (auto inst : optCode) {
        inst->copyInfo.gen = E;
        inst->copyInfo.kill = E;

        Var *rs = inst->getResult();
        Operator op = inst->getOp();
        if (inst->unknown()) {
            inst->copyInfo.kill = U;    // 杀死所有表达式
        }
        else if (op >= Operator::OP_AS && op <= Operator::OP_GET) {
            for (size_t i = 0; i < copyExpr.size(); ++i) {
                if (copyExpr[i] == inst) {
                    inst->copyInfo.gen.set(i);
                }
                // 对于复写表达式 "x=y", 若该指令修改了其中的 x 或 y, 则称该指令杀死了复写表达式 "x=y"
                else if (rs == copyExpr[i]->getResult() || rs == copyExpr[i]->getArg1()) {
                    inst->copyInfo.kill.set(i);
                }
            }
        }
    }
}

// 复写传播传递函数: f(x)=(x - kill(B)) | gen(B)
bool CopyPropagation::translate(Block *block) {
    Set tmp = block->copyInfo.in;
    for (auto inst : block->insts) {
        Set &in = inst->copyInfo.in;
        Set &out = inst->copyInfo.out;
        in = tmp;
        out = (in - inst->copyInfo.kill) | inst->copyInfo.gen;
        tmp = out;
    }
    bool flag = tmp != block->copyInfo.out;
    block->copyInfo.out = tmp;
    return flag;
}

void CopyPropagation::analyse() {
    dfg->blocks[0]->copyInfo.out = E;
    for (size_t i = 1; i < dfg->blocks.size(); ++i) {
        dfg->blocks[i]->copyInfo.out = U;
    }

    bool change = true;
    while (change) {
        change = false;
        for (size_t i = 1; i < dfg->blocks.size(); ++i) {
            Block *block = dfg->blocks[i];
            if (!block->canReach) {
                continue;
            }

            Set tmp = U;
            for (auto prev : block->prevs) {
                tmp = tmp & prev->copyInfo.out;
            }
            block->copyInfo.in = tmp;

            if (translate(block)) {
                change = true;
            }
        }
    }
}

// 递归检测 var 赋值的源头的实现函数
Var *CopyPropagation::__find(const Set &in, Var *var, Var *src) {
    if (!var) {
        return nullptr;
    }

    for (size_t i = 0; i < copyExpr.size(); ++i) {
        if (in.get(i) && var == copyExpr[i]->getResult()) {
            var = copyExpr[i]->getArg1();
            if (src == var) {   // 查找过程中出现环. 比如: x=y;y=x
                break;
            }
            return __find(in, var, src);
        }
    }
    return var; // 找不到可替换的变量, 返回自身, 或者上层结果
}

Var *CopyPropagation::find(const Set &in, Var *var) {
    return __find(in, var, var);
}

void CopyPropagation::propagate() {
    analyse();

    for (auto inst : optCode) {
        Var *rs = inst->getResult();
        Operator op = inst->getOp();
        Var *arg1 = inst->getArg1();
        Var *arg2 = inst->getArg2();

        if (op == Operator::OP_SET) {       // 取值运算, 检查 *arg1=result
            Var *newRs = find(inst->copyInfo.in, rs);
            inst->replace(op, newRs, arg1); // 不论是否是新变量, 都更新指令
        }
        else if (op >= Operator::OP_AS && op <= Operator::OP_GET && op != Operator::OP_LEA) {
            Var *newArg1 = find(inst->copyInfo.in, arg1);
            Var *newArg2 = find(inst->copyInfo.in, arg2);
            inst->replace(op, rs, newArg1, newArg2);    // 不论是否是新变量, 都更新指令
        }
        else if (op == Operator::OP_JT || op == Operator::OP_JF || op == Operator::OP_ARG || op == Operator::OP_RETV) {                                                 // 条件表达式,参数表达式，返回表达式
            Var *newArg1 = find(inst->copyInfo.in, arg1);
            inst->setArg1(newArg1); // 更新参数变量/返回值
        }
    }
}
