#include <cstring>

#include "platform.h"
#include "symbol/var.h"
#include "symbol/fun.h"

#include "ir/genir.h"

#include "interInst.h"

using namespace std;


void InterInst::init() {
    op = Operator::OP_NOP;
    this->result = nullptr;
    this->target = nullptr;
    this->arg1 = nullptr;
    this->fun = nullptr;
    this->arg2 = nullptr;
    first = false;
    isDead = false;

    file = nullptr;

    block = nullptr;
}

InterInst::InterInst(Operator op, Var *rs, Var *arg1, Var *arg2) {
    init();
    this->op = op;
    this->result = rs;
    this->arg1 = arg1;
    this->arg2 = arg2;
}

InterInst::InterInst(Operator op, Fun *fun, Var *rs) {
    init();
    this->op = op;
    this->result = rs;
    this->fun = fun;
    this->arg2 = nullptr;
}

InterInst::InterInst(Operator op, Var *arg1) {
    init();
    this->op = op;
    this->result = nullptr;
    this->arg1 = arg1;
    this->arg2 = nullptr;
}

InterInst::InterInst() {
    init();
    label = GenIR::genLb();
}

InterInst::InterInst(Operator op, InterInst *tar, Var *arg1, Var *arg2) {
    init();
    this->op = op;
    this->target = tar;
    this->arg1 = arg1;
    this->arg2 = arg2;
}

void InterInst::replace(Operator op, Var *rs, Var *arg1, Var *arg2) {
    this->op = op;
    this->result = rs;
    this->arg1 = arg1;
    this->arg2 = arg2;
}

void InterInst::replace(Operator op, InterInst *tar, Var *arg1, Var *arg2) {
    this->op = op;
    this->target = tar;
    this->arg1 = arg1;
    this->arg2 = arg2;
}

void InterInst::callToProc() {
    this->result = nullptr; // 清除返回值
    this->op = Operator::OP_PROC;
}

InterInst::~InterInst() {
    // if (arg1 && arg1->isLiteral()) {
    //     delete arg1;
    // }
    // if (arg2 && arg2->isLiteral()) {
    //     delete arg2;
    // }
}

void InterInst::toString() {
    if (label != "") {
        printf("%s:\n", label.c_str());
        return;
    }

    switch (op) {
    case Operator::OP_NOP:
        printf("nop");
        break;
    case Operator::OP_DEC:
        printf("dec ");
        arg1->value();
        break;
    case Operator::OP_ENTRY:
        printf("entry");
        break;
    case Operator::OP_EXIT:
        printf("exit");
        break;
    case Operator::OP_AS:
        result->value();
        printf(" = ");
        arg1->value();
        break;
    case Operator::OP_ADD:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" + ");
        arg2->value();
        break;
    case Operator::OP_SUB:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" - ");
        arg2->value();
        break;
    case Operator::OP_MUL:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" * ");
        arg2->value();
        break;
    case Operator::OP_DIV:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" / ");
        arg2->value();
        break;
    case Operator::OP_MOD:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" %% ");
        arg2->value();
        break;
    case Operator::OP_NEG:
        result->value();
        printf(" = ");
        printf("-");
        arg1->value();
        break;
    case Operator::OP_GT:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" > ");
        arg2->value();
        break;
    case Operator::OP_GE:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" >= ");
        arg2->value();
        break;
    case Operator::OP_LT:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" < ");
        arg2->value();
        break;
    case Operator::OP_LE:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" <= ");
        arg2->value();
        break;
    case Operator::OP_EQU:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" == ");
        arg2->value();
        break;
    case Operator::OP_NE:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" != ");
        arg2->value();
        break;
    case Operator::OP_NOT:
        result->value();
        printf(" = ");
        printf("!");
        arg1->value();
        break;
    case Operator::OP_AND:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" && ");
        arg2->value();
        break;
    case Operator::OP_OR:
        result->value();
        printf(" = ");
        arg1->value();
        printf(" || ");
        arg2->value();
        break;
    case Operator::OP_JMP:
        printf("goto %s", target->label.c_str());
        break;
    case Operator::OP_JT:
        printf("if( ");
        arg1->value();
        printf(" )goto %s", target->label.c_str());
        break;
    case Operator::OP_JF:
        printf("if( !");
        arg1->value();
        printf(" )goto %s", target->label.c_str());
        break;
    // case Operator::OP_JG:
    //     printf("if( ");
    //     arg1->value();
    //     printf(" > ");
    //     arg2->value();
    //     printf(" )goto %s",
    //            target->label.c_str());
    //     break;
    // case Operator::OP_JGE:
    //     printf("if( ");
    //     arg1->value();
    //     printf(" >= ");
    //     arg2->value();
    //     printf(" )goto %s",
    //            target->label.c_str());
    //     break;
    // case Operator::OP_JL:
    //     printf("if( ");
    //     arg1->value();
    //     printf(" < ");
    //     arg2->value();
    //     printf(" )goto %s",
    //            target->label.c_str());
    //     break;
    // case Operator::OP_JLE:
    //     printf("if( ");
    //     arg1->value();
    //     printf(" <= ");
    //     arg2->value();
    //     printf(" )goto %s",
    //            target->label.c_str());
    //     break;
    // case Operator::OP_JE:
    //     printf("if( ");
    //     arg1->value();
    //     printf(" == ");
    //     arg2->value();
    //     printf(" )goto %s",
    //            target->label.c_str());
    //     break;
    case Operator::OP_JNE:
        printf("if( ");
        arg1->value();
        printf(" != ");
        arg2->value();
        printf(" )goto %s",
               target->label.c_str());
        break;
    case Operator::OP_ARG:
        printf("arg ");
        arg1->value();
        break;
    case Operator::OP_PROC:
        printf("%s()", fun->getName().c_str());
        break;
    case Operator::OP_CALL:
        result->value();
        printf(" = %s()", fun->getName().c_str());
        break;
    case Operator::OP_RET:
        printf("return goto %s", target->label.c_str());
        break;
    case Operator::OP_RETV:
        printf("return ");
        arg1->value();
        printf(" goto %s", target->label.c_str());
        break;
    case Operator::OP_LEA:
        result->value();
        printf(" = ");
        printf("&");
        arg1->value();
        break;
    case Operator::OP_SET:
        printf("*");
        arg1->value();
        printf(" = ");
        result->value();
        break;
    case Operator::OP_GET:
        result->value();
        printf(" = ");
        printf("*");
        arg1->value();
        break;
    }

    printf("\n");
}

bool InterInst::isJcond() {
    return op >= Operator::OP_JT && op <= Operator::OP_JNE;
}

bool InterInst::isJmp() {
    return op == Operator::OP_JMP || op == Operator::OP_RET || op == Operator::OP_RETV;
}

void InterInst::setFirst() {
    first = true;
}

bool InterInst::isFirst() {
    return first;
}

bool InterInst::isLb() {
    return label != "";
}

bool InterInst::isExpr() {
    return (op >= Operator::OP_AS && (op <= Operator::OP_OR || op == Operator::OP_GET));  // && result->isBase();
}

// 不确定运算结果影响的运算 (指针赋值, 函数调用)
bool InterInst::unknown() {
    return op == Operator::OP_SET || op == Operator::OP_PROC || op == Operator::OP_CALL;
}

Operator InterInst::getOp() {
    return op;
}

InterInst *InterInst::getTarget() {
    return target;
}

Var *InterInst::getResult() {
    return result;
}

void InterInst::setArg1(Var *arg1) {
    this->arg1 = arg1;
}

bool InterInst::isDec() {
    return op == Operator::OP_DEC;
}

Var *InterInst::getArg1() {
    return arg1;
}

Var *InterInst::getArg2() {
    return arg2;
}

string InterInst::getLabel() {
    return label;
}

Fun *InterInst::getFun() {
    return fun;
}


#define emit(format, args...) fprintf(file, "\t" format "\n", ##args);


void InterInst::loadVar(const string &reg32, const string &reg8, Var *var) {
    if (!var) {
        return;
    }
    const char *reg = var->isChar() ? reg8.c_str() : reg32.c_str();
    const char **regName = var->isChar() ? Plat::reg8Name : Plat::reg32Name;

    if (var->isChar()) {
        emit("mov %s, 0", reg32.c_str());
    }

    const string tmpName = var->getName();
    const char *name = tmpName.c_str();
    if (var->notConst()) {

#ifdef REG_OPT
        int id = var->regId;
        if (id != -1) {
            if (strcmp(reg, regName[id])) {
                emit("mov %s, %s", reg, regName[id]);
            }
            return;
        }
#endif

        int off = var->getOffset();
        if (!off) {
            if (!var->getArray()) {
                emit("mov %s, [%s]", reg, name);
            }
            else {
                emit("mov %s, %s", reg, name);
            }
        }
        else {
            if (!var->getArray()) {
                emit("mov %s, [ebp%+d]", reg, off);
            }
            else {
                emit("lea %s, [ebp%+d]", reg, off);
            }
        }
    }
    else {
        if (var->isBase()) {
            emit("mov %s, %d", reg, var->getVal());
        }
        else {
            emit("mov %s, %s", reg, name);
        }
    }
}

void InterInst::initVar(Var *var) {
    if (!var) {
        return;
    }
    if (!var->unInit()) {
        if (var->isBase()) {
            emit("mov eax, %d", var->getVal());
        }
        else {
            emit("mov eax, %s", var->getPtrVal().c_str());
        }
        storeVar("eax", "al", var);
    }
}

void InterInst::leaVar(const string &reg32, Var *var) {
    if (!var) {
        return;
    }
    const char *reg = reg32.c_str();
    const string tmpName = var->getName();
    const char *name = tmpName.c_str();
    int off = var->getOffset();
    if (!off) {
        emit("mov %s, %s", reg, name);
    }
    else {
        emit("lea %s, [ebp%+d]", reg, off);
    }
}

void InterInst::storeVar(const string &reg32, const string &reg8, Var *var) {
    if (!var) {
        return;
    }
    const char *reg = var->isChar() ? reg32.c_str() : reg8.c_str();
    const char **regName = var->isChar() ? Plat::reg32Name : Plat::reg8Name;
#ifdef REG_OPT
    int id = var->regId;
    if (id != -1) {
        if (strcmp(reg, regName[id])) {
            emit("mov %s, %s", regName[id], reg);
        }
        return;
    }
#endif

    const string tmpName = var->getName();
    const char *name = tmpName.c_str();
    int off = var->getOffset();
    if (!off) {
        emit("mov [%s], %s", name, reg);
    }
    else {
        emit("mov [ebp%+d], %s", off, reg);
    }
}

void InterInst::toX86(FILE *file) {
    this->file = file;
    if (label != "") {
        fprintf(file, "%s:\n", label.c_str());
        return;
    }

    switch (op) {
    case Operator::OP_DEC:
        initVar(arg1);
        break;
    case Operator::OP_ENTRY:
        emit("push ebp");
        emit("mov ebp, esp");
        emit("sub esp, %d", getFun()->getMaxDep());
        break;
    case Operator::OP_EXIT:
        emit("mov esp, ebp");
        emit("pop ebp");
        emit("ret");
        break;
    case Operator::OP_AS:
        loadVar("eax", "al", arg1);
        storeVar("eax", "al", result);
        break;
    case Operator::OP_ADD:
        loadVar("eax", "al", arg1);
        loadVar("ebx", "bl", arg2);
        emit("add eax, ebx");
        storeVar("eax", "al", result);
        break;
    case Operator::OP_SUB:
        loadVar("eax", "al", arg1);
        loadVar("ebx", "bl", arg2);
        emit("sub eax, ebx");
        storeVar("eax", "al", result);
        break;
    case Operator::OP_MUL:
        loadVar("eax", "al", arg1);
        loadVar("ebx", "bl", arg2);
        emit("imul ebx");
        storeVar("eax", "al", result);
        break;
    case Operator::OP_DIV:
        loadVar("eax", "al", arg1);
        loadVar("ebx", "bl", arg2);
        emit("idiv ebx");
        storeVar("eax", "al", result);
        break;
    case Operator::OP_MOD:
        loadVar("eax", "al", arg1);
        loadVar("ebx", "bl", arg2);
        emit("idiv ebx");
        storeVar("edx", "dl", result);
        break;
    case Operator::OP_NEG:
        loadVar("eax", "al", arg1);
        emit("neg eax");
        storeVar("eax", "al", result);
        break;
    case Operator::OP_GT:
        loadVar("eax", "al", arg1);
        loadVar("ebx", "bl", arg2);
        emit("mov ecx, 0");
        emit("cmp eax, ebx");
        emit("setg cl");
        storeVar("ecx", "cl", result);
        break;
    case Operator::OP_GE:
        loadVar("eax", "al", arg1);
        loadVar("ebx", "bl", arg2);
        emit("mov ecx, 0");
        emit("cmp eax, ebx");
        emit("setge cl");
        storeVar("ecx", "cl", result);
        break;
    case Operator::OP_LT:
        loadVar("eax", "al", arg1);
        loadVar("ebx", "bl", arg2);
        emit("mov ecx, 0");
        emit("cmp eax, ebx");
        emit("setl cl");
        storeVar("ecx", "cl", result);
        break;
    case Operator::OP_LE:
        loadVar("eax", "al", arg1);
        loadVar("ebx", "bl", arg2);
        emit("mov ecx, 0");
        emit("cmp eax, ebx");
        emit("setle cl");
        storeVar("ecx", "cl", result);
        break;
    case Operator::OP_EQU:
        loadVar("eax", "al", arg1);
        loadVar("ebx", "bl", arg2);
        emit("mov ecx, 0");
        emit("cmp eax, ebx");
        emit("sete cl");
        storeVar("ecx", "cl", result);
        break;
    case Operator::OP_NE:
        loadVar("eax", "al", arg1);
        loadVar("ebx", "bl", arg2);
        emit("mov ecx, 0");
        emit("cmp eax, ebx");
        emit("setne cl");
        storeVar("ecx", "cl", result);
        break;
    case Operator::OP_NOT:
        loadVar("eax", "al", arg1);
        emit("mov ebx, 0");
        emit("cmp eax, 0");
        emit("sete bl");
        storeVar("ebx", "bl", result);
        break;
    case Operator::OP_AND:
        loadVar("eax", "al", arg1);
        emit("cmp eax, 0");
        emit("setne al");
        loadVar("ebx", "bl", arg2);
        emit("cmp ebx, 0");
        emit("setne bl");
        emit("add eax, ebx");
        storeVar("eax", "al", result);
        break;
    case Operator::OP_OR:
        loadVar("eax", "al", arg1);
        emit("cmp eax, 0");
        emit("setne al");
        loadVar("ebx", "bl", arg2);
        emit("cmp ebx, 0");
        emit("setne bl");
        emit("or eax, ebx");
        storeVar("eax", "al", result);
        break;
    case Operator::OP_JMP:
        emit("jmp %s", target->label.c_str());
        break;
    case Operator::OP_JT:
        loadVar("eax", "al", arg1);
        emit("cmp eax, 0");
        emit("jne %s", target->label.c_str());
        break;
    case Operator::OP_JF:
        loadVar("eax", "al", arg1);
        emit("cmp eax, 0");
        emit("je %s", target->label.c_str());
        break;
    case Operator::OP_JNE:
        loadVar("eax", "al", arg1);
        loadVar("ebx", "bl", arg2);
        emit("cmp eax, ebx");
        emit("jne %s", target->label.c_str());
        break;
    case Operator::OP_ARG:
        loadVar("eax", "al", arg1);
        emit("push eax");
        break;
    case Operator::OP_PROC:
        emit("call %s", fun->getName().c_str());
        emit("add esp, %d", static_cast<int>(fun->getParaVar().size()) * 4);
        break;
    case Operator::OP_CALL:
        emit("call %s", fun->getName().c_str());
        emit("add esp, %d", static_cast<int>(fun->getParaVar().size()) * 4);
        storeVar("eax", "al", result);
        break;
    case Operator::OP_RET:
        emit("jmp %s", target->label.c_str());
        break;
    case Operator::OP_RETV:
        loadVar("eax", "al", arg1);
        emit("jmp %s", target->label.c_str());
        break;
    case Operator::OP_LEA:
        leaVar("eax", arg1);
        storeVar("eax", "al", result);
        break;
    case Operator::OP_SET:
        loadVar("eax", "al", result);
        loadVar("ebx", "bl", arg1);
        emit("mov [ebx], eax");
        break;
    case Operator::OP_GET:
        loadVar("eax", "al", arg1);
        emit("mov eax, [eax]");
        storeVar("eax", "al", result);
        break;

    // `OP_NOP`
    default:
        break;
    }
}
