#include <sstream>

#include "error.h"

#include "ir/interInst.h"

#include "symbol/var.h"
#include "symbol/fun.h"
#include "symbol/symtab.h"

#include "genir.h"

using namespace std;

#define SEMERROR(code) Error::semError(code)


int GenIR::lbNum = 0;


GenIR::GenIR(SymTab &tab) : symtab(tab)
{
    symtab.setIr(this);
    lbNum = 0;
    push(nullptr, nullptr);
}

string GenIR::genLb() {
    lbNum++;
    string lb = ".L";
    stringstream ss;
    ss << lbNum;
    return lb + ss.str();
}

Var *GenIR::genArray(Var *array, Var *index) {
    if (!array || !index) {
        return nullptr;
    }
    if (array->isVoid() || index->isVoid()) {
        SEMERROR(static_cast<int>(SemError::EXPR_IS_VOID));
        return nullptr;
    }
    if (array->isBase() || !index->isBase()) {
        SEMERROR(static_cast<int>(SemError::ARR_TYPE_ERR));
        return index;
    }
    return genPtr(genAdd(array, index));
}

void GenIR::genPara(Var *arg) {
    if (arg->isRef()) {
        arg = genAssign(arg);
    }

    // 无条件复制参数! 传值, 不传引用!
    // Var *newVar=new Var(symtab.getScopePath(),arg);  // 创建参数变量
    // symtab.addVar(newVar);   // 添加无效变量, 占领栈帧

    InterInst *argInst = new InterInst(Operator::OP_ARG, arg);  // push arg

    // argInst->offset = newVar->getOffset();  // 将变量的地址与 arg 指令地址共享. 没有优化地址也能用
    // argInst->path = symtab.getScopePath();  // 记录路径. 为了寄存器分配时计算地址

    symtab.addInst(argInst);
}

Var *GenIR::genCall(Fun *function, vector<Var *>& args) {
    if (!function) {
        return nullptr;
    }

    for (int i = args.size() - 1; i >= 0; --i) {
        genPara(args[i]);
    }

    if (function->getType() == Tag::KW_VOID) {
        symtab.addInst(new InterInst(Operator::OP_PROC, function));
        return Var::getVoid();
    }
    else {
        Var *ret = new Var(symtab.getScopePath(), function->getType(), false);
        symtab.addInst(new InterInst(Operator::OP_CALL, function, ret));
        symtab.addVar(ret);
        return ret;
    }
}

Var *GenIR::genTwoOp(Var *lval, Tag opt, Var *rval) {
    if (!lval || !rval)
        return nullptr;
    if (lval->isVoid() || rval->isVoid())
    {
        SEMERROR(static_cast<int>(SemError::EXPR_IS_VOID));
        return nullptr;
    }
    // 赋值单独处理
    if (opt == Tag::ASSIGN) {
        return genAssign(lval, rval); // 赋值
    }
    // 先处理 (*p) 变量
    if (lval->isRef()) {
        lval = genAssign(lval);
    }
    if (rval->isRef()) {
        rval = genAssign(rval);
    }
    if (opt == Tag::OR) {
        return genOr(lval, rval);
    }
    if (opt == Tag::AND) {
        return genAnd(lval, rval);
    }
    if (opt == Tag::EQU) {
        return genEqu(lval, rval);
    }
    if (opt == Tag::NEQU) {
        return genNequ(lval, rval);
    }
    if (opt == Tag::ADD) {
        return genAdd(lval, rval);
    }
    if (opt == Tag::SUB) {
        return genSub(lval, rval);
    }

    if (!lval->isBase() || !rval->isBase()) {
        SEMERROR(static_cast<int>(SemError::EXPR_NOT_BASE));
        return lval;
    }

    if (opt == Tag::GT) {
        return genGt(lval, rval);
    }
    if (opt == Tag::GE) {
        return genGe(lval, rval);
    }
    if (opt == Tag::LT) {
        return genLt(lval, rval);
    }
    if (opt == Tag::LE) {
        return genLe(lval, rval);
    }
    if (opt == Tag::MUL) {
        return genMul(lval, rval);
    }
    if (opt == Tag::DIV) {
        return genDiv(lval, rval);
    }
    if (opt == Tag::MOD) {
        return genMod(lval, rval);
    }

    return lval;
}

Var *GenIR::genAssign(Var *lval, Var *rval) {
    if (!lval->getLeft()) {
        SEMERROR(static_cast<int>(SemError::EXPR_NOT_LEFT_VAL));
        return rval;
    }
    if (!typeCheck(lval, rval)) {
        SEMERROR(static_cast<int>(SemError::ASSIGN_TYPE_ERR));
        return rval;
    }

    // 考虑右值 (*p)
    if (rval->isRef()) {
        if (!lval->isRef()) {
            // lval = *(rval->ptr)
            symtab.addInst(new InterInst(Operator::OP_GET, lval, rval->getPointer()));
            return lval;
        }
        else {
            // *(lval->ptr) = *(rval->ptr);
            rval = genAssign(rval);
        }
    }

    if (lval->isRef()) {
        // *(lval->ptr) = rval
        symtab.addInst(new InterInst(Operator::OP_SET, rval, lval->getPointer()));
    }
    else {
        // lval = rval
        symtab.addInst(new InterInst(Operator::OP_AS, lval, rval));
    }
    return lval;
}

Var *GenIR::genAssign(Var *val) {
    Var *tmp = new Var(symtab.getScopePath(), val);
    symtab.addVar(tmp);
    if (val->isRef()) {
        // tmp = *(val->ptr)
        symtab.addInst(new InterInst(Operator::OP_GET, tmp, val->getPointer()));
    }
    else {
        // tmp = val
        symtab.addInst(new InterInst(Operator::OP_AS, tmp, val));
    }
    return tmp;
}

Var *GenIR::genOr(Var *lval, Var *rval) {
    Var *tmp = new Var(symtab.getScopePath(), Tag::KW_INT, false);
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(Operator::OP_OR, tmp, lval, rval));
    return tmp;
}

Var *GenIR::genAnd(Var *lval, Var *rval) {
    Var *tmp = new Var(symtab.getScopePath(), Tag::KW_INT, false);
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(Operator::OP_AND, tmp, lval, rval));
    return tmp;
}

Var *GenIR::genGt(Var *lval, Var *rval) {
    Var *tmp = new Var(symtab.getScopePath(), Tag::KW_INT, false);
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(Operator::OP_GT, tmp, lval, rval));
    return tmp;
}

Var *GenIR::genGe(Var *lval, Var *rval) {
    Var *tmp = new Var(symtab.getScopePath(), Tag::KW_INT, false);
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(Operator::OP_GE, tmp, lval, rval));
    return tmp;
}

Var *GenIR::genLt(Var *lval, Var *rval) {
    Var *tmp = new Var(symtab.getScopePath(), Tag::KW_INT, false);
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(Operator::OP_LT, tmp, lval, rval));
    return tmp;
}

Var *GenIR::genLe(Var *lval, Var *rval) {
    Var *tmp = new Var(symtab.getScopePath(), Tag::KW_INT, false);
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(Operator::OP_LE, tmp, lval, rval));
    return tmp;
}

Var *GenIR::genEqu(Var *lval, Var *rval) {
    Var *tmp = new Var(symtab.getScopePath(), Tag::KW_INT, false);
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(Operator::OP_EQU, tmp, lval, rval));
    return tmp;
}

Var *GenIR::genNequ(Var *lval, Var *rval) {
    Var *tmp = new Var(symtab.getScopePath(), Tag::KW_INT, false);
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(Operator::OP_NE, tmp, lval, rval));
    return tmp;
}

Var *GenIR::genAdd(Var *lval, Var *rval) {
    Var *tmp = nullptr;
    if ((lval->getArray() || lval->getPtr()) && rval->isBase()) {
        tmp = new Var(symtab.getScopePath(), lval);
        rval = genMul(rval, Var::getStep(lval));
    }
    else if (rval->isBase() && (rval->getArray() || rval->getPtr())) {
        tmp = new Var(symtab.getScopePath(), rval);
        lval = genMul(lval, Var::getStep(rval));
    }
    else if (lval->isBase() && rval->isBase()) {
        tmp = new Var(symtab.getScopePath(), Tag::KW_INT, false);
    }
    else {
        SEMERROR(static_cast<int>(SemError::EXPR_NOT_BASE));
        return lval;
    }

    symtab.addVar(tmp);
    symtab.addInst(new InterInst(Operator::OP_ADD, tmp, lval, rval));
    return tmp;
}

Var *GenIR::genSub(Var *lval, Var *rval) {
    Var *tmp = nullptr;
    if (!rval->isBase()) {
        SEMERROR(static_cast<int>(SemError::EXPR_NOT_BASE));
        return lval;
    }

    // 指针和数组
    if ((lval->getArray() || lval->getPtr())) {
        tmp = new Var(symtab.getScopePath(), lval);
        rval = genMul(rval, Var::getStep(lval));
    }
    else {
        tmp = new Var(symtab.getScopePath(), Tag::KW_INT, false);
    }

    symtab.addVar(tmp);
    symtab.addInst(new InterInst(Operator::OP_SUB, tmp, lval, rval));
    return tmp;
}

Var *GenIR::genMul(Var *lval, Var *rval) {
    Var *tmp = new Var(symtab.getScopePath(), Tag::KW_INT, false);
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(Operator::OP_MUL, tmp, lval, rval));
    return tmp;
}

Var *GenIR::genDiv(Var *lval, Var *rval) {
    Var *tmp = new Var(symtab.getScopePath(), Tag::KW_INT, false);
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(Operator::OP_DIV, tmp, lval, rval));
    return tmp;
}

Var *GenIR::genMod(Var *lval, Var *rval) {
    Var *tmp = new Var(symtab.getScopePath(), Tag::KW_INT, false);
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(Operator::OP_MOD, tmp, lval, rval));
    return tmp;
}

Var *GenIR::genOneOpLeft(Tag opt, Var *val) {
    if (!val) {
        return nullptr;
    }
    if (val->isVoid()) {
        SEMERROR(static_cast<int>(SemError::EXPR_IS_VOID));
        return nullptr;
    }

    if (opt == Tag::LEA) {
        return genLea(val);
    }
    if (opt == Tag::MUL) {
        return genPtr(val);
    }
    if (opt == Tag::INC) {
        return genIncL(val);
    }
    if (opt == Tag::DEC) {
        return genDecL(val);
    }
    // 处理 (*p)
    if (val->isRef()) {
        val = genAssign(val);
    }
    if (opt == Tag::NOT) {
        return genNot(val);
    }
    if (opt == Tag::SUB) {
        return genMinus(val);
    }
    return val;
}

Var *GenIR::genNot(Var *val) {
    Var *tmp = new Var(symtab.getScopePath(), Tag::KW_INT, false);
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(Operator::OP_NOT, tmp, val));
    return tmp;
}

Var *GenIR::genMinus(Var *val) {
    if (!val->isBase()) {
        SEMERROR(static_cast<int>(SemError::EXPR_NOT_BASE));
        return val;
    }
    Var *tmp = new Var(symtab.getScopePath(), Tag::KW_INT, false);
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(Operator::OP_NEG, tmp, val));
    return tmp;
}

Var *GenIR::genIncL(Var *val) {
    if (!val->getLeft()) {
        SEMERROR(static_cast<int>(SemError::EXPR_NOT_LEFT_VAL));
        return val;
    }
    // ++*p => t1=*p t2=t1+1 *p=t2
    if (val->isRef()) {
        Var *t1 = genAssign(val);
        Var *t2 = genAdd(t1, Var::getStep(val));
        return genAssign(val, t2);
    }
    symtab.addInst(new InterInst(Operator::OP_ADD, val, val, Var::getStep(val)));
    return val;
}

Var *GenIR::genDecL(Var *val) {
    if (!val->getLeft()) {
        SEMERROR(static_cast<int>(SemError::EXPR_NOT_LEFT_VAL));
        return val;
    }
    if (val->isRef()) {
        Var *t1 = genAssign(val);
        Var *t2 = genSub(t1, Var::getStep(val));
        return genAssign(val, t2);
    }
    symtab.addInst(new InterInst(Operator::OP_SUB, val, val, Var::getStep(val)));
    return val;
}

// 取地址语句
Var *GenIR::genLea(Var *val) {
    if (!val->getLeft()) {
        SEMERROR(static_cast<int>(SemError::EXPR_NOT_LEFT_VAL));
        return val;
    }
    if (val->isRef()) { // 类似 &*p 运算
        return val->getPointer();
    }
    Var *tmp = new Var(symtab.getScopePath(), val->getType(), true);
    symtab.addVar(tmp);
    symtab.addInst(new InterInst(Operator::OP_LEA, tmp, val));  // 中间代码: tmp = &val
    return tmp;
}

Var *GenIR::genPtr(Var *val) {
    if (val->isBase()) {
        SEMERROR(static_cast<int>(SemError::EXPR_IS_BASE));
        return val;
    }
    Var *tmp = new Var(symtab.getScopePath(), val->getType(), false);
    tmp->setLeft(true);
    tmp->setPointer(val);
    symtab.addVar(tmp);
    return tmp;
}

Var *GenIR::genOneOpRight(Var *val, Tag opt) {
    if (!val) {
        return nullptr;
    }
    if (val->isVoid()) {
        SEMERROR(static_cast<int>(SemError::EXPR_IS_VOID));
        return nullptr;
    }
    if (!val->getLeft()) {
        SEMERROR(static_cast<int>(SemError::EXPR_NOT_LEFT_VAL));
        return val;
    }

    if (opt == Tag::INC) {
        return genIncR(val);
    }
    if (opt == Tag::DEC) {
        return genDecR(val);
    }
    return val;
}

Var *GenIR::genIncR(Var *val) {
    Var *tmp = genAssign(val);
    symtab.addInst(new InterInst(Operator::OP_ADD, val, val, Var::getStep(val)));
    return tmp;
}

Var *GenIR::genDecR(Var *val) {
    Var *tmp = genAssign(val);
    symtab.addInst(new InterInst(Operator::OP_SUB, val, val, Var::getStep(val)));
    return tmp;
}

bool GenIR::typeCheck(Var *lval, Var *rval) {
    bool flag = false;
    if (!rval) {
        return false;
    }
    if (lval->isBase() && rval->isBase()) {
        flag = true;
    }
    else if (!lval->isBase() && !rval->isBase()) {
        flag = (rval->getType() == lval->getType());
    }
    return flag;
}

void GenIR::genWhileHead(InterInst *&_while, InterInst *&_exit) {
    // InterInst* _blank=new InterInst();
    // symtab.addInst(new InterInst(Operator::OP_JMP, _blank));

    _while = new InterInst();
    symtab.addInst(_while);

    // symtab.addInst(_blank);

    _exit = new InterInst();
    push(_while, _exit);
}

void GenIR::genWhileCond(Var *cond, InterInst *_exit) {
    if (cond) {
        if (cond->isVoid()) {
            cond = Var::getTrue();
        }
        else if (cond->isRef()) {
            cond = genAssign(cond);
        }
        symtab.addInst(new InterInst(Operator::OP_JF, _exit, cond));
    }
}

void GenIR::genWhileTail(InterInst *&_while, InterInst *&_exit) {
    symtab.addInst(new InterInst(Operator::OP_JMP, _while));
    symtab.addInst(_exit);
    pop();
}

void GenIR::genDoWhileHead(InterInst *&_do, InterInst *&_exit) {
    _do = new InterInst();
    _exit = new InterInst();
    symtab.addInst(_do);
    push(_do, _exit);
}

void GenIR::genDoWhileTail(Var *cond, InterInst *_do, InterInst *_exit) {
    if (cond) {
        if (cond->isVoid()) {
            cond = Var::getTrue();
        }
        else if (cond->isRef()) {
            cond = genAssign(cond);
        }
        symtab.addInst(new InterInst(Operator::OP_JT, _do, cond));
    }
    symtab.addInst(_exit);
    pop();
}

void GenIR::genForHead(InterInst *&_for, InterInst *&_exit) {
    _for = new InterInst();
    _exit = new InterInst();
    symtab.addInst(_for);
}

void GenIR::genForCondBegin(Var *cond, InterInst *&_step, InterInst *&_block, InterInst *_exit) {
    _block = new InterInst();
    _step = new InterInst();
    if (cond) {
        if (cond->isVoid()) {
            cond = Var::getTrue();
        }
        else if (cond->isRef()) {
            cond = genAssign(cond);
        }
        symtab.addInst(new InterInst(Operator::OP_JF, _exit, cond));
        symtab.addInst(new InterInst(Operator::OP_JMP, _block));
    }
    symtab.addInst(_step);
    push(_step, _exit);
}

void GenIR::genForCondEnd(InterInst *_for, InterInst *_block) {
    symtab.addInst(new InterInst(Operator::OP_JMP, _for));
    symtab.addInst(_block);
}

void GenIR::genForTail(InterInst *&_step, InterInst *&_exit) {
    symtab.addInst(new InterInst(Operator::OP_JMP, _step));
    symtab.addInst(_exit);
    pop();
}

void GenIR::genIfHead(Var *cond, InterInst *&_else) {
    _else = new InterInst();
    if (cond) {
        if (cond->isRef()) {
            cond = genAssign(cond);
        }
        symtab.addInst(new InterInst(Operator::OP_JF, _else, cond));
    }
}

void GenIR::genIfTail(InterInst *&_else) {
    symtab.addInst(_else);
}

void GenIR::genElseHead(InterInst *_else, InterInst *&_exit) {
    _exit = new InterInst();
    symtab.addInst(new InterInst(Operator::OP_JMP, _exit));
    symtab.addInst(_else);
}

void GenIR::genElseTail(InterInst *&_exit) {
    symtab.addInst(_exit);
}

void GenIR::genSwitchHead(InterInst *&_exit) {
    _exit = new InterInst();
    push(nullptr, _exit);
}

void GenIR::genSwitchTail(InterInst *_exit) {
    symtab.addInst(_exit);
    pop();
}

void GenIR::genCaseHead(Var *cond, Var *lb, InterInst *& _case_exit) {
    _case_exit = new InterInst();
    if (lb) {
        symtab.addInst(new InterInst(Operator::OP_JNE, _case_exit, cond, lb));
    }
}

void GenIR::genCaseTail(InterInst *_case_exit) {
    symtab.addInst(_case_exit);
    // InterInst *_case_exit_append = new InterInst();
    // symtab.addInst(new InterInst(Operator::OP_JMP, _case_exit_append));
    // symtab.addInst(_case_exit);
    // symtab.addInst(_case_exit_append);
}

void GenIR::push(InterInst *head, InterInst *tail) {
    heads.push_back(head);
    tails.push_back(tail);
}

void GenIR::pop() {
    heads.pop_back();
    tails.pop_back();
}

void GenIR::genBreak() {
    InterInst *tail = tails.back();
    if (tail) {
        symtab.addInst(new InterInst(Operator::OP_JMP, tail));
    }
    else {
        SEMERROR(static_cast<int>(SemError::BREAK_ERR));
    }
}

void GenIR::genContinue() {
    InterInst *head = heads.back();
    if (head) {
        symtab.addInst(new InterInst(Operator::OP_JMP, head));
    }
    else {
        SEMERROR(static_cast<int>(SemError::CONTINUE_ERR));
    }
}

// 指针取值语句
void GenIR::genReturn(Var *ret) {
    if (!ret) {
        return;
    }
    Fun *fun = symtab.getCurFun();
    if ((ret->isVoid() && fun->getType() != Tag::KW_VOID) || (ret->isBase() && fun->getType() == Tag::KW_VOID)) {
        SEMERROR(static_cast<int>(SemError::RETURN_ERR));
        return;
    }
    InterInst *returnPoint = fun->getReturnPoint();
    if (ret->isVoid()) {
        symtab.addInst(new InterInst(Operator::OP_RET, returnPoint));
    }
    else {
        if (ret->isRef()) {
            ret = genAssign(ret);
        }
        symtab.addInst(new InterInst(Operator::OP_RETV, returnPoint, ret));
    }
}

bool GenIR::genVarInit(Var *var)
{
    if (var->getName()[0] == '<') {
        return 0;
    }

    symtab.addInst(new InterInst(Operator::OP_DEC, var));   // 添加变量声明指令
    if (var->setInit()) {
        genTwoOp(var, Tag::ASSIGN, var->getInitData());
    }

    return 1;
}

void GenIR::genFunHead(Fun *function) {
    function->enterScope();
    symtab.addInst(new InterInst(Operator::OP_ENTRY, function));
    function->setReturnPoint(new InterInst);
}

void GenIR::genFunTail(Fun *function) {
    symtab.addInst(function->getReturnPoint());
    symtab.addInst(new InterInst(Operator::OP_EXIT, function));
    function->leaveScope();
}
