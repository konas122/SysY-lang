#include "symbol/var.h"
#include "symbol/symtab.h"

#include "ir/interInst.h"

#include "optimize/dfg.h"

#include "constprop.h"

using namespace std;


ConstPropagation::ConstPropagation(DFG *g, SymTab *t, vector<Var *>& paraVar)
    : tab(t), dfg(g)
{   // TODO
    glbVars = tab->getGlbVars();
    int index = 0;

    // 添加全局变量
    for (auto var : glbVars) {
        var->index = index++;
        vars.emplace_back(var);

        double val = UNDEF;
        if (!var->isBase()) {
            val = NAC;
        }
        else if (!var->unInit()) {
            val = var->getVal();
        }
        boundVals.emplace_back(val);    // 添加边界值
    }

    // 参数变量, 初始值应该是 NAC
    for (size_t i = 0; i < paraVar.size(); ++i) {
        Var *var = paraVar[i];
        var->index = index++;
        vars.emplace_back(var);
        boundVals.emplace_back(NAC);
    }

    for (auto inst : dfg->codeList) {
        // 局部变量 DEC 声明
        if (inst->isDec()) {
            Var *var = inst->getArg1();
            var->index = index++;
            vars.emplace_back(var);

            double val = UNDEF;
            if (!var->isBase()) {
                val = NAC;
            }
            else if (!var->unInit()) {
                val = var->getVal();
            }
            boundVals.emplace_back(val);
        }
    }

    while (index--) {
        initVals.emplace_back(UNDEF);
    }
}

// 元素交汇运算规则
double ConstPropagation::join(double left, double right) {
    if (left == NAC || right == NAC) {
        return NAC;
    }
    else if (left == UNDEF) {
        return right;
    }
    else if (right == UNDEF) {
        return left;
    }
    else if (right == left) {
        return left;
    }
    return NAC;
}

// 集合交汇运算
void ConstPropagation::join(Block *block) {
    list<Block *> &prevs = block->prevs;
    vector<double> &in = block->inVals;

    int prevCount = prevs.size();   // 前驱个数
    if (prevCount == 1) {
        in = prevs.front()->outVals;
        return;
    }
    if (prevCount == 0) {
        in = initVals;
        return;
    }
    // 多个前驱, 处理 in 集合每个元素
    for (size_t i = 0; i < in.size(); ++i) {
        double val = UNDEF;
        for (const auto bl : prevs) {
            val = join(val, bl->outVals[i]);
        }
        in[i] = val;
    }
}

// 单指令传递函数
void ConstPropagation::translate(InterInst *inst, const vector<double> &in, vector<double> &out) {
    out = in;
    Operator op = inst->getOp();

    Var *arg1 = inst->getArg1();
    Var *arg2 = inst->getArg2();
    const Var *result = inst->getResult();

    if (inst->isExpr()) {
        double tmp = NAC;   // 保守预测

        // 一元运算: x=y x=-y x=!y
        if (op == Operator::OP_AS || op == Operator::OP_NEG || op == Operator::OP_NOT) {
            if (arg1->index == -1) { // 参数不在值列表, 必是常量
                if (arg1->isBase()) {
                    tmp = arg1->getVal();
                }
            }
            else {
                tmp = in[arg1->index];
            }

            if (tmp != UNDEF && tmp != NAC) {
                if (op == Operator::OP_NEG) {
                    tmp = -tmp;
                }
                else if (op == Operator::OP_NOT) {
                    tmp = static_cast<double>(!tmp);
                }
            }
        }
        // 二元运算
        else if (op >= Operator::OP_ADD && op <= Operator::OP_OR) {
            double lp = NAC, rp = NAC;  // 保守预测
            if (arg1->index == -1) {    // 参数不在值列表, 必是常量
                if (arg1->isBase()) {
                    lp = arg1->getVal();
                }
            }
            else {
                lp = in[arg1->index];
            }

            if (arg2->index == -1) {    // 参数不在值列表, 必是常量
                if (arg2->isBase()) {
                    rp = arg2->getVal();
                }
            }
            else {
                rp = in[arg2->index];
            }

            if (lp == NAC || rp == NAC) {
                tmp = NAC;
            }
            else if (lp == UNDEF || rp == UNDEF) {
                tmp = UNDEF;
            }
            else {  // 都是常量, 可以计算
                int left = lp, right = rp;
                switch (op) {
                case Operator::OP_ADD:
                    tmp = left + right;
                    break;
                case Operator::OP_SUB:
                    tmp = left - right;
                    break;
                case Operator::OP_MUL:
                    tmp = left * right;
                    break;
                case Operator::OP_DIV:
                    if (!right) {
                        tmp = NAC;
                    }
                    else {
                        tmp = left / right;
                    }
                    break;
                case Operator::OP_MOD:
                    if (!right) {
                        tmp = NAC;
                    }
                    else {
                        tmp = left % right;
                    }
                    break;
                case Operator::OP_GT:
                    tmp = static_cast<double>(left > right);
                    break;
                case Operator::OP_GE:
                    tmp = static_cast<double>(left >= right);
                    break;
                case Operator::OP_LT:
                    tmp = static_cast<double>(left < right);
                    break;
                case Operator::OP_LE:
                    tmp = static_cast<double>(left <= right);
                    break;
                case Operator::OP_EQU:
                    tmp = static_cast<double>(left == right);
                    break;
                case Operator::OP_NE:
                    tmp = static_cast<double>(left != right);
                    break;
                case Operator::OP_AND:
                    tmp = static_cast<double>(left && right);
                    break;
                case Operator::OP_OR:
                    tmp = static_cast<double>(left || right);
                    break;

                default:
                    tmp = NAC;
                    break;
                }
            }
        }
        else if (op == Operator::OP_GET) {    // 破坏运算 x=*y
            tmp = NAC;
        }

        out[result->index] = tmp;
    }
    else if (op == Operator::OP_SET || (op == Operator::OP_ARG && !arg1->isBase())) {
        for (auto &o : out) {
            o = NAC;
        }
    }
    else if (op == Operator::OP_PROC) { // 破坏运算 call f()
        for (const auto var : glbVars) {
            out[var->index] = NAC;      // 全局变量全部置为 NAC
        }
    }
    else if (op == Operator::OP_CALL) { // 破坏运算 call f()
        for (const auto var : glbVars) {
            out[var->index] = NAC;      // 全局变量全部置为 NAC
        }
        out[result->index] = NAC;       // 函数返回值失去常量性质 (保守预测)
    }

    // 拷贝信息指令的 in, out 信息,
    // 方便代数化简, 条件跳转优化和不可达代码消除
    inst->inVals = in;
    inst->outVals = out;
}

bool ConstPropagation::translate(Block *block) {
    vector<double> in = block->inVals;
    vector<double> out = block->outVals;

    for (auto inst : block->insts) {
        translate(inst, in, out);
        in = out;   // 下条指令的 in 为当前指令的 out
    }

    bool flag = false;  // 是否变化
    for (size_t i = 0; i < out.size(); ++i) {
        if (block->outVals[i] != out[i]) {
            flag = true;
            break;
        }
    }

    block->outVals = out;
    return flag;
}

void ConstPropagation::analyse() {
    dfg->blocks[0]->outVals = boundVals;

    for (size_t i = 1; i < dfg->blocks.size(); ++i) {
        dfg->blocks[i]->outVals = initVals;
        dfg->blocks[i]->inVals = initVals;
    }

    bool outChange = true;
    while (outChange) {
        outChange = false;
        for (size_t i = 1; i < dfg->blocks.size(); ++i) {
            join(dfg->blocks[i]);
            if (translate(dfg->blocks[i])) {
                outChange = true;
            }
        }
    }
}

// 执行常量传播
void ConstPropagation::propagate() {
    analyse();          // 常量传播分析
    algebraSimplify();  // 代数化简
    condJmpOpt();       // 条件跳转优化, 消除不可达代码
}

/**
 * @brief 代数化简算法, 合并常量
 * 
 * @example a=1+2 -> a=3
 */
void ConstPropagation::algebraSimplify() {
    for (const auto block : dfg->blocks) {
        for (auto inst : block->insts) {
            Operator op = inst->getOp();
            if (inst->isExpr()) {
                double rs;
                Var *result = inst->getResult();
                Var *arg1 = inst->getArg1();
                Var *arg2 = inst->getArg2();

                rs = inst->outVals[result->index];
                if (rs != UNDEF && rs != NAC) {
                    Var *newVar = new Var(static_cast<int>(rs));
                    tab->addVar(newVar);
                    inst->replace(Operator::OP_AS, result, newVar);
                }
                else if (op >= Operator::OP_ADD && op <= Operator::OP_OR
                        && !(op == Operator::OP_AS || op == Operator::OP_NEG || op == Operator::OP_NOT))
                {
                    double lp = NAC, rp = NAC;
                    if (arg1->index == -1) {
                        if (arg1->isBase()) {
                            lp = arg1->getVal();
                        }
                    }
                    else {
                        lp = inst->inVals[arg1->index];
                    }

                    if (arg2->index == -1) {
                        if (arg2->isBase()) {
                            rp = arg2->getVal();
                        }
                    }
                    else {
                        rp = inst->inVals[arg2->index];
                    }

                    int left, right;
                    bool dol = false, dor = false;
                    if (lp != UNDEF && lp != NAC) {
                        left = lp;
                        dol = true;
                    }
                    else if (rp != UNDEF && rp != NAC) {
                        right = rp;
                        dor = true;
                    }
                    else {
                        continue;
                    }

                    Var *newArg1 = nullptr;
                    Var *newArg2 = nullptr;
                    Operator newOp = Operator::OP_AS;

                    if (op == Operator::OP_ADD) {
                        if (dol && left == 0) {
                            newArg1 = arg2;
                        }
                        if (dor && right == 0) {
                            newArg1 = arg1;
                        }
                    }
                    else if (op == Operator::OP_SUB) {
                        if (dol && left == 0) {
                            newOp = Operator::OP_NEG;
                            newArg1 = arg2;
                        }
                        if (dor && right == 0) {
                            newArg1 = arg1;
                        }
                    }
                    else if (op == Operator::OP_MUL) {
                        if ((dol && left == 0) || (dor && right == 0)) {
                            newArg1 = new Var(0);
                        }
                        if (dol && left == 1) {
                            newArg1 = arg2;
                        }
                        if (dor && right == 1) {
                            newArg1 = arg1;
                        }
                    }
                    else if (op == Operator::OP_DIV) {
                        if (dol && left == 0) {
                            newArg1 = SymTab::zero;
                        }
                        if (dor && right == 1) {
                            newArg1 = arg1;
                        }
                    }
                    else if (op == Operator::OP_MOD) {
                        if ((dol && left == 0) || (dor && right == 1)) {
                            newArg1 = SymTab::zero;
                        }
                    }
                    else if (op == Operator::OP_AND) {
                        if ((dol && left == 0) || (dor && right == 0)) {
                            newArg1 = SymTab::zero;
                        }
                        if (dol && left != 0) {
                            newOp = Operator::OP_NE;
                            newArg1 = arg2;
                            newArg2 = SymTab::zero;
                        }
                        if (dor && right != 0) {
                            newOp = Operator::OP_NE;
                            newArg1 = arg1;
                            newArg2 = SymTab::zero;
                        }
                    }
                    else if (op == Operator::OP_OR) {
                        if ((dol && left != 0) || (dor && right != 0)) {
                            newArg1 = SymTab::one;
                        }
                        if (dol && left == 0) {
                            newOp = Operator::OP_NE;
                            newArg1 = arg2;
                            newArg2 = SymTab::zero;
                        }
                        if (dor && right == 0) {
                            newOp = Operator::OP_NE;
                            newArg1 = arg1;
                            newArg2 = SymTab::zero;
                        }
                    }

                    if (newArg1) {  // 代数化简成功
                        inst->replace(newOp, result, newArg1, newArg2);
                    }
                    else {          // 没法化简, 正常传播
                        if (dol) {  // 传播左操作数
                            newArg1 = new Var(left);
                            tab->addVar(newArg1);
                            newArg2 = arg2;
                        }
                        else if (dor) { // 传播右操作数
                            newArg2 = new Var(right);
                            tab->addVar(newArg2);
                            newArg1 = arg1;
                        }
                        inst->replace(op, result, newArg1, newArg2);
                    }
                }
            }
            else if (op == Operator::OP_ARG || op == Operator::OP_RETV) {
                const Var *arg1 = inst->getArg1();
                if (arg1->index != -1) {                    // 不是常数
                    double rs = inst->outVals[arg1->index]; // 结果变量常量传播结果

                    if (rs != UNDEF && rs != NAC) {     // 为常量
                        Var *newVar = new Var((int)rs); // 计算的表达式结果
                        tab->addVar(newVar);            // 添加到符号表
                        inst->setArg1(newVar);          // 替换新的操作符与常量操作数
                    }
                }
            }
        }
    }
}

// 条件跳转优化, 同时进行不可达代码消除
// 此时只需要处理 JT JF 两种情况, Jcond 指令会在短路规则优化内产生, 不影响表达式常量条件的处理
void ConstPropagation::condJmpOpt() {
    for (size_t j = 0; j < dfg->blocks.size(); ++j) {

        for (auto i = dfg->blocks[j]->insts.begin(), k = i;
             i != dfg->blocks[j]->insts.end(); i = k)
        {
            ++k;    // 记录下一个迭代器, 防止遍历时发生指令删除
            InterInst *inst = *i;
            if (inst->isJcond()) {
                Operator op = inst->getOp();
                InterInst *tar = inst->getTarget();
                Var *arg1 = inst->getArg1();

                double cond = NAC;
                if (arg1->index == -1) {    // 参数不在值列表, 必是常量
                    if (arg1->isBase()) {
                        cond = arg1->getVal();
                    }
                }
                else {
                    cond = inst->inVals[arg1->index];
                }

                if (cond == NAC || cond == UNDEF) {
                    continue;
                }

                if (op == Operator::OP_JT) {
                    if (cond == 0) {
                        inst->block->insts.remove(inst);            // 将该指令从基本块内删除
                        if (dfg->blocks[j + 1] != tar->block) {     // 目标块不是紧跟块
                            dfg->delLink(inst->block, tar->block);  // 从 DFG 内递归解除指令所在块到目标块的关联
                        }
                    }
                    else {
                        inst->replace(Operator::OP_JMP, tar);
                        if (dfg->blocks[j + 1] != tar->block) {             // 目标块不是紧跟块
                            dfg->delLink(inst->block, dfg->blocks[j + 1]);  // 从 DFG 内递归解除指令所在块到紧跟块的关联
                        }
                    }
                }
                else if (op == Operator::OP_JF) {
                    if (cond == 0) {
                        inst->replace(Operator::OP_JMP, tar);
                        if (dfg->blocks[j + 1] != tar->block) {             // 目标块不是紧跟块
                            dfg->delLink(inst->block, dfg->blocks[j + 1]);  // 从 DFG 内递归解除指令所在块到紧跟块的关联
                        }
                    }
                    else {
                        inst->block->insts.remove(inst);
                        if (dfg->blocks[j + 1] != tar->block) {     // 目标块不是紧跟块
                            dfg->delLink(inst->block, tar->block);  // 从 DFG 内递归解除指令所在块到目标块的关联
                        }
                    }
                }
            }
        }
    }
}
