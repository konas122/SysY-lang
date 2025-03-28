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


shared_ptr<GenIR> GenIR::create(shared_ptr<SymTab> tab) {
    auto ir = make_shared<GenIR>();
    ir->init(tab);
    return ir;
}

void GenIR::init(shared_ptr<SymTab> tab) {
    symtab = tab;
    symtab.lock()->setIr(shared_from_this());
    push(nullptr, nullptr);
}

string GenIR::genLb() {
    lbNum++;
    string lb = ".L";
    stringstream ss;
    ss << lbNum;
    return lb + ss.str();
}

shared_ptr<Var> GenIR::genArray(shared_ptr<Var> array, shared_ptr<Var> index) {
    if (!array || !index) {
        return nullptr;
    }
    if (array->isVoid() || index->isVoid()) {
        SEMERROR(cast_int(SemError::EXPR_IS_VOID));
        return nullptr;
    }
    if (array->isBase() || !index->isBase()) {
        SEMERROR(cast_int(SemError::ARR_TYPE_ERR));
        return index;
    }
    return genPtr(genAdd(array, index));
}

void GenIR::genPara(shared_ptr<Var> arg) {
    if (arg->isRef()) {
        arg = genAssign(arg);
    }

    // 无条件复制参数! 传值, 不传引用!
    // shared_ptr<Var> newVar=make_shared<Var>(symtab->getScopePath(), arg);    // 创建参数变量
    // symtab->addVar(newVar);   // 添加无效变量, 占领栈帧

    auto argInst = make_shared<InterInst>(Operator::OP_ARG, arg);  // push arg

    // argInst->offset = newVar->getOffset();  // 将变量的地址与 arg 指令地址共享. 没有优化地址也能用
    // argInst->path = symtab->getScopePath();  // 记录路径. 为了寄存器分配时计算地址

    symtab.lock()->addInst(argInst);
}

shared_ptr<Var> GenIR::genCall(shared_ptr<Fun> function, const vector<shared_ptr<Var>>& args) {
    if (!function) {
        return nullptr;
    }

    for (int i = args.size() - 1; i >= 0; --i) {
        genPara(args[i]);
    }

    auto tab = symtab.lock();
    if (function->getType() == Tag::KW_VOID) {
        tab->addInst(make_shared<InterInst>(Operator::OP_PROC, function));
        return Var::getVoid();
    }
    else {
        auto ret = make_shared<Var>(tab->getScopePath(), function->getType(), false);
        tab->addInst(make_shared<InterInst>(Operator::OP_CALL, function, ret));
        tab->addVar(ret);
        return ret;
    }
}

shared_ptr<Var> GenIR::genTwoOp(shared_ptr<Var> lval, Tag opt, shared_ptr<Var> rval) {
    if (!lval || !rval) {
        return nullptr;
    }
    if (lval->isVoid() || rval->isVoid()) {
        SEMERROR(cast_int(SemError::EXPR_IS_VOID));
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
        SEMERROR(cast_int(SemError::EXPR_NOT_BASE));
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

shared_ptr<Var> GenIR::genAssign(shared_ptr<Var> lval, shared_ptr<Var> rval) {
    if (!lval->getLeft()) {
        SEMERROR(cast_int(SemError::EXPR_NOT_LEFT_VAL));
        return rval;
    }
    if (!typeCheck(lval, rval)) {
        SEMERROR(cast_int(SemError::ASSIGN_TYPE_ERR));
        return rval;
    }

    auto tab = symtab.lock();
    // 考虑右值 (*p)
    if (rval->isRef()) {
        if (!lval->isRef()) {
            // lval = *(rval->ptr)
            tab->addInst(make_shared<InterInst>(Operator::OP_GET, lval, rval->getPointer()));
            return lval;
        }
        else {
            // *(lval->ptr) = *(rval->ptr);
            rval = genAssign(rval);
        }
    }

    if (lval->isRef()) {
        // *(lval->ptr) = rval
        tab->addInst(make_shared<InterInst>(Operator::OP_SET, rval, lval->getPointer()));
    }
    else {
        // lval = rval
        tab->addInst(make_shared<InterInst>(Operator::OP_AS, lval, rval));
    }
    return lval;
}

shared_ptr<Var> GenIR::genAssign(shared_ptr<Var> val) {
    auto tab = symtab.lock();
    auto tmp = make_shared<Var>(tab->getScopePath(), val);
    tab->addVar(tmp);
    if (val->isRef()) {
        // tmp = *(val->ptr)
        tab->addInst(make_shared<InterInst>(Operator::OP_GET, tmp, val->getPointer()));
    }
    else {
        // tmp = val
        tab->addInst(make_shared<InterInst>(Operator::OP_AS, tmp, val));
    }
    return tmp;
}

shared_ptr<Var> GenIR::genOr(shared_ptr<Var> lval, shared_ptr<Var> rval) {
    auto tab = symtab.lock();
    auto tmp = make_shared<Var>(tab->getScopePath(), Tag::KW_INT, false);
    tab->addVar(tmp);
    tab->addInst(make_shared<InterInst>(Operator::OP_OR, tmp, lval, rval));
    return tmp;
}

shared_ptr<Var> GenIR::genAnd(shared_ptr<Var> lval, shared_ptr<Var> rval) {
    auto tab = symtab.lock();
    auto tmp = make_shared<Var>(tab->getScopePath(), Tag::KW_INT, false);
    tab->addVar(tmp);
    tab->addInst(make_shared<InterInst>(Operator::OP_AND, tmp, lval, rval));
    return tmp;
}

shared_ptr<Var> GenIR::genGt(shared_ptr<Var> lval, shared_ptr<Var> rval) {
    auto tab = symtab.lock();
    auto tmp = make_shared<Var>(tab->getScopePath(), Tag::KW_INT, false);
    tab->addVar(tmp);
    tab->addInst(make_shared<InterInst>(Operator::OP_GT, tmp, lval, rval));
    return tmp;
}

shared_ptr<Var> GenIR::genGe(shared_ptr<Var> lval, shared_ptr<Var> rval) {
    auto tab = symtab.lock();
    auto tmp = make_shared<Var>(tab->getScopePath(), Tag::KW_INT, false);
    tab->addVar(tmp);
    tab->addInst(make_shared<InterInst>(Operator::OP_GE, tmp, lval, rval));
    return tmp;
}

shared_ptr<Var> GenIR::genLt(shared_ptr<Var> lval, shared_ptr<Var> rval) {
    auto tab = symtab.lock();
    auto tmp = make_shared<Var>(tab->getScopePath(), Tag::KW_INT, false);
    tab->addVar(tmp);
    tab->addInst(make_shared<InterInst>(Operator::OP_LT, tmp, lval, rval));
    return tmp;
}

shared_ptr<Var> GenIR::genLe(shared_ptr<Var> lval, shared_ptr<Var> rval) {
    auto tab = symtab.lock();
    auto tmp = make_shared<Var>(tab->getScopePath(), Tag::KW_INT, false);
    tab->addVar(tmp);
    tab->addInst(make_shared<InterInst>(Operator::OP_LE, tmp, lval, rval));
    return tmp;
}

shared_ptr<Var> GenIR::genEqu(shared_ptr<Var> lval, shared_ptr<Var> rval) {
    auto tab = symtab.lock();
    auto tmp = make_shared<Var>(tab->getScopePath(), Tag::KW_INT, false);
    tab->addVar(tmp);
    tab->addInst(make_shared<InterInst>(Operator::OP_EQU, tmp, lval, rval));
    return tmp;
}

shared_ptr<Var> GenIR::genNequ(shared_ptr<Var> lval, shared_ptr<Var> rval) {
    auto tab = symtab.lock();
    auto tmp = make_shared<Var>(tab->getScopePath(), Tag::KW_INT, false);
    tab->addVar(tmp);
    tab->addInst(make_shared<InterInst>(Operator::OP_NE, tmp, lval, rval));
    return tmp;
}

shared_ptr<Var> GenIR::genAdd(shared_ptr<Var> lval, shared_ptr<Var> rval) {
    shared_ptr<Var> tmp;
    auto tab = symtab.lock();
    if ((lval->getArray() || lval->getPtr()) && rval->isBase()) {
        tmp = make_shared<Var>(tab->getScopePath(), lval);
        rval = genMul(rval, Var::getStep(lval));
    }
    else if (rval->isBase() && (rval->getArray() || rval->getPtr())) {
        tmp = make_shared<Var>(tab->getScopePath(), rval);
        lval = genMul(lval, Var::getStep(rval));
    }
    else if (lval->isBase() && rval->isBase()) {
        tmp = make_shared<Var>(tab->getScopePath(), Tag::KW_INT, false);
    }
    else {
        SEMERROR(cast_int(SemError::EXPR_NOT_BASE));
        return lval;
    }

    tab->addVar(tmp);
    tab->addInst(make_shared<InterInst>(Operator::OP_ADD, tmp, lval, rval));
    return tmp;
}

shared_ptr<Var> GenIR::genSub(shared_ptr<Var> lval, shared_ptr<Var> rval) {
    shared_ptr<Var> tmp;
    auto tab = symtab.lock();
    if (!rval->isBase()) {
        SEMERROR(cast_int(SemError::EXPR_NOT_BASE));
        return lval;
    }

    // 指针和数组
    if ((lval->getArray() || lval->getPtr())) {
        tmp = make_shared<Var>(tab->getScopePath(), lval);
        rval = genMul(rval, Var::getStep(lval));
    }
    else {
        tmp = make_shared<Var>(tab->getScopePath(), Tag::KW_INT, false);
    }

    tab->addVar(tmp);
    tab->addInst(make_shared<InterInst>(Operator::OP_SUB, tmp, lval, rval));
    return tmp;
}

shared_ptr<Var> GenIR::genMul(shared_ptr<Var> lval, shared_ptr<Var> rval) {
    auto tab = symtab.lock();
    auto tmp = make_shared<Var>(tab->getScopePath(), Tag::KW_INT, false);
    tab->addVar(tmp);
    tab->addInst(make_shared<InterInst>(Operator::OP_MUL, tmp, lval, rval));
    return tmp;
}

shared_ptr<Var> GenIR::genDiv(shared_ptr<Var> lval, shared_ptr<Var> rval) {
    auto tab = symtab.lock();
    auto tmp = make_shared<Var>(tab->getScopePath(), Tag::KW_INT, false);
    tab->addVar(tmp);
    tab->addInst(make_shared<InterInst>(Operator::OP_DIV, tmp, lval, rval));
    return tmp;
}

shared_ptr<Var> GenIR::genMod(shared_ptr<Var> lval, shared_ptr<Var> rval) {
    auto tab = symtab.lock();
    auto tmp = make_shared<Var>(tab->getScopePath(), Tag::KW_INT, false);
    tab->addVar(tmp);
    tab->addInst(make_shared<InterInst>(Operator::OP_MOD, tmp, lval, rval));
    return tmp;
}

shared_ptr<Var> GenIR::genOneOpLeft(Tag opt, shared_ptr<Var> val) {
    if (!val) {
        return nullptr;
    }
    if (val->isVoid()) {
        SEMERROR(cast_int(SemError::EXPR_IS_VOID));
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

shared_ptr<Var> GenIR::genNot(shared_ptr<Var> val) {
    auto tab = symtab.lock();
    auto tmp = make_shared<Var>(tab->getScopePath(), Tag::KW_INT, false);
    tab->addVar(tmp);
    tab->addInst(make_shared<InterInst>(Operator::OP_NOT, tmp, val));
    return tmp;
}

shared_ptr<Var> GenIR::genMinus(shared_ptr<Var> val) {
    if (!val->isBase()) {
        SEMERROR(cast_int(SemError::EXPR_NOT_BASE));
        return val;
    }
    auto tab = symtab.lock();
    auto tmp = make_shared<Var>(tab->getScopePath(), Tag::KW_INT, false);
    tab->addVar(tmp);
    tab->addInst(make_shared<InterInst>(Operator::OP_NEG, tmp, val));
    return tmp;
}

shared_ptr<Var> GenIR::genIncL(shared_ptr<Var> val) {
    if (!val->getLeft()) {
        SEMERROR(cast_int(SemError::EXPR_NOT_LEFT_VAL));
        return val;
    }
    // ++*p => t1=*p t2=t1+1 *p=t2
    if (val->isRef()) {
        shared_ptr<Var> t1 = genAssign(val);
        shared_ptr<Var> t2 = genAdd(t1, Var::getStep(val));
        return genAssign(val, t2);
    }
    auto tab = symtab.lock();
    tab->addInst(make_shared<InterInst>(Operator::OP_ADD, val, val, Var::getStep(val)));
    return val;
}

shared_ptr<Var> GenIR::genDecL(shared_ptr<Var> val) {
    if (!val->getLeft()) {
        SEMERROR(cast_int(SemError::EXPR_NOT_LEFT_VAL));
        return val;
    }
    if (val->isRef()) {
        shared_ptr<Var> t1 = genAssign(val);
        shared_ptr<Var> t2 = genSub(t1, Var::getStep(val));
        return genAssign(val, t2);
    }
    auto tab = symtab.lock();
    tab->addInst(make_shared<InterInst>(Operator::OP_SUB, val, val, Var::getStep(val)));
    return val;
}

// 取地址语句
shared_ptr<Var> GenIR::genLea(shared_ptr<Var> val) {
    if (!val->getLeft()) {
        SEMERROR(cast_int(SemError::EXPR_NOT_LEFT_VAL));
        return val;
    }
    if (val->isRef()) { // 类似 &*p 运算
        return val->getPointer();
    }
    auto tab = symtab.lock();
    auto tmp = make_shared<Var>(tab->getScopePath(), val->getType(), true);
    tab->addVar(tmp);
    tab->addInst(make_shared<InterInst>(Operator::OP_LEA, tmp, val));  // 中间代码: tmp = &val
    return tmp;
}

shared_ptr<Var> GenIR::genPtr(shared_ptr<Var> val) {
    if (val->isBase()) {
        SEMERROR(cast_int(SemError::EXPR_IS_BASE));
        return val;
    }
    auto tab = symtab.lock();
    auto tmp = make_shared<Var>(tab->getScopePath(), val->getType(), false);
    tmp->setLeft(true);
    tmp->setPointer(val);
    tab->addVar(tmp);
    return tmp;
}

shared_ptr<Var> GenIR::genOneOpRight(shared_ptr<Var> val, Tag opt) {
    if (!val) {
        return nullptr;
    }
    if (val->isVoid()) {
        SEMERROR(cast_int(SemError::EXPR_IS_VOID));
        return nullptr;
    }
    if (!val->getLeft()) {
        SEMERROR(cast_int(SemError::EXPR_NOT_LEFT_VAL));
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

shared_ptr<Var> GenIR::genIncR(shared_ptr<Var> val) {
    auto tmp = genAssign(val);
    symtab.lock()->addInst(make_shared<InterInst>(Operator::OP_ADD, val, val, Var::getStep(val)));
    return tmp;
}

shared_ptr<Var> GenIR::genDecR(shared_ptr<Var> val) {
    auto tmp = genAssign(val);
    symtab.lock()->addInst(make_shared<InterInst>(Operator::OP_SUB, val, val, Var::getStep(val)));
    return tmp;
}

bool GenIR::typeCheck(const shared_ptr<Var> lval, const shared_ptr<Var> rval) {
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

void GenIR::genWhileHead(shared_ptr<InterInst>& _while, shared_ptr<InterInst>& _exit) {
    // auto _blank = make_shared<InterInst>();
    // symtab->addInst(make_shared<InterInst>(Operator::OP_JMP, _blank));

    _while = make_shared<InterInst>();
    symtab.lock()->addInst(_while);

    // symtab->addInst(_blank);

    _exit = make_shared<InterInst>();
    push(_while, _exit);
}

void GenIR::genWhileCond(shared_ptr<Var> cond, shared_ptr<InterInst> _exit) {
    if (cond) {
        if (cond->isVoid()) {
            cond = Var::getTrue();
        }
        else if (cond->isRef()) {
            cond = genAssign(cond);
        }
        symtab.lock()->addInst(make_shared<InterInst>(Operator::OP_JF, _exit, cond));
    }
}

void GenIR::genWhileTail(shared_ptr<InterInst>& _while, const shared_ptr<InterInst>& _exit) {
    auto tab = symtab.lock();
    tab->addInst(make_shared<InterInst>(Operator::OP_JMP, _while));
    tab->addInst(_exit);
    pop();
}

void GenIR::genDoWhileHead(shared_ptr<InterInst>& _do, shared_ptr<InterInst>& _exit) {
    _do = make_shared<InterInst>();
    _exit = make_shared<InterInst>();
    symtab.lock()->addInst(_do);
    push(_do, _exit);
}

void GenIR::genDoWhileTail(shared_ptr<Var> cond, shared_ptr<InterInst> _do, shared_ptr<InterInst> _exit) {
    auto tab = symtab.lock();
    if (cond) {
        if (cond->isVoid()) {
            cond = Var::getTrue();
        }
        else if (cond->isRef()) {
            cond = genAssign(cond);
        }
        tab->addInst(make_shared<InterInst>(Operator::OP_JT, _do, cond));
    }
    tab->addInst(_exit);
    pop();
}

void GenIR::genForHead(shared_ptr<InterInst>& _for, shared_ptr<InterInst>& _exit) {
    _for = make_shared<InterInst>();
    _exit = make_shared<InterInst>();
    symtab.lock()->addInst(_for);
}

void GenIR::genForCondBegin(shared_ptr<Var> cond, shared_ptr<InterInst>& _step, shared_ptr<InterInst>& _block, shared_ptr<InterInst> _exit) {
    auto tab = symtab.lock();
    _block = make_shared<InterInst>();
    _step = make_shared<InterInst>();
    if (cond) {
        if (cond->isVoid()) {
            cond = Var::getTrue();
        }
        else if (cond->isRef()) {
            cond = genAssign(cond);
        }
        tab->addInst(make_shared<InterInst>(Operator::OP_JF, _exit, cond));
        tab->addInst(make_shared<InterInst>(Operator::OP_JMP, _block));
    }
    tab->addInst(_step);
    push(_step, _exit);
}

void GenIR::genForCondEnd(shared_ptr<InterInst> _for, shared_ptr<InterInst> _block) {
    auto tab = symtab.lock();
    tab->addInst(make_shared<InterInst>(Operator::OP_JMP, _for));
    tab->addInst(_block);
}

void GenIR::genForTail(shared_ptr<InterInst>& _step, const shared_ptr<InterInst>& _exit) {
    auto tab = symtab.lock();
    tab->addInst(make_shared<InterInst>(Operator::OP_JMP, _step));
    tab->addInst(_exit);
    pop();
}

void GenIR::genIfHead(shared_ptr<Var> cond, shared_ptr<InterInst>& _else) {
    _else = make_shared<InterInst>();
    if (cond) {
        if (cond->isRef()) {
            cond = genAssign(cond);
        }
        symtab.lock()->addInst(make_shared<InterInst>(Operator::OP_JF, _else, cond));
    }
}

void GenIR::genIfTail(const shared_ptr<InterInst>& _else) {
    symtab.lock()->addInst(_else);
}

void GenIR::genElseHead(shared_ptr<InterInst> _else, shared_ptr<InterInst>& _exit) {
    _exit = make_shared<InterInst>();
    auto tab = symtab.lock();
    tab->addInst(make_shared<InterInst>(Operator::OP_JMP, _exit));
    tab->addInst(_else);
}

void GenIR::genElseTail(shared_ptr<InterInst>& _exit) {
    symtab.lock()->addInst(_exit);
}

void GenIR::genSwitchHead(shared_ptr<InterInst>& _exit) {
    _exit = make_shared<InterInst>();
    push(nullptr, _exit);
}

void GenIR::genSwitchTail(shared_ptr<InterInst> _exit) {
    symtab.lock()->addInst(_exit);
    pop();
}

void GenIR::genCaseHead(shared_ptr<Var> cond, shared_ptr<Var> lb, shared_ptr<InterInst>& _case_exit) {
    _case_exit = make_shared<InterInst>();
    if (lb) {
        symtab.lock()->addInst(make_shared<InterInst>(Operator::OP_JNE, _case_exit, cond, lb));
    }
}

void GenIR::genCaseTail(shared_ptr<InterInst> _case_exit) {
    symtab.lock()->addInst(_case_exit);
    // shared_ptr<InterInst> _case_exit_append = make_shared<InterInst>();
    // symtab->addInst(make_shared<InterInst>(Operator::OP_JMP, _case_exit_append));
    // symtab->addInst(_case_exit);
    // symtab->addInst(_case_exit_append);
}

void GenIR::push(shared_ptr<InterInst>head, shared_ptr<InterInst>tail) {
    heads.push_back(head);
    tails.push_back(tail);
}

void GenIR::pop() {
    heads.pop_back();
    tails.pop_back();
}

void GenIR::genBreak() {
    auto tail = tails.back();
    auto tab = symtab.lock();
    if (tail) {
        tab->addInst(make_shared<InterInst>(Operator::OP_JMP, tail));
    }
    else {
        SEMERROR(cast_int(SemError::BREAK_ERR));
    }
}

void GenIR::genContinue() {
    auto head = heads.back();
    auto tab = symtab.lock();
    if (head) {
        tab->addInst(make_shared<InterInst>(Operator::OP_JMP, head));
    }
    else {
        SEMERROR(cast_int(SemError::CONTINUE_ERR));
    }
}

// 指针取值语句
void GenIR::genReturn(shared_ptr<Var> ret) {
    if (!ret) {
        return;
    }
    auto tab = symtab.lock();
    auto fun = tab->getCurFun();
    if ((ret->isVoid() && fun->getType() != Tag::KW_VOID) || (ret->isBase() && fun->getType() == Tag::KW_VOID)) {
        SEMERROR(cast_int(SemError::RETURN_ERR));
        return;
    }
    auto returnPoint = fun->getReturnPoint();
    if (ret->isVoid()) {
        tab->addInst(make_shared<InterInst>(Operator::OP_RET, returnPoint));
    }
    else {
        if (ret->isRef()) {
            ret = genAssign(ret);
        }
        tab->addInst(make_shared<InterInst>(Operator::OP_RETV, returnPoint, ret));
    }
}

bool GenIR::genVarInit(shared_ptr<Var> var) {
    if (var->getName()[0] == '<') {
        return 0;
    }

    auto tab = symtab.lock();
    tab->addInst(make_shared<InterInst>(Operator::OP_DEC, var));   // 添加变量声明指令
    if (var->setInit()) {
        genTwoOp(var, Tag::ASSIGN, var->getInitData());
    }

    return 1;
}

void GenIR::genFunHead(shared_ptr<Fun> function) {
    auto tab = symtab.lock();
    function->enterScope();
    tab->addInst(make_shared<InterInst>(Operator::OP_ENTRY, function));
    function->setReturnPoint(make_shared<InterInst>());
}

void GenIR::genFunTail(shared_ptr<Fun> function) {
    auto tab = symtab.lock();
    tab->addInst(function->getReturnPoint());
    tab->addInst(make_shared<InterInst>(Operator::OP_EXIT, function));
    function->leaveScope();
}
