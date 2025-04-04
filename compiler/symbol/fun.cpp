#include "error.h"
#include "compiler.h"
#include "platform.h"


#include "lexer/token.h"

#include "ir/genir.h"
#include "ir/interCode.h"
#include "ir/interInst.h"

#include "optimize/dfg.h"
#include "optimize/alloc.h"
#include "optimize/livevar.h"
#include "optimize/peephole.h"
#include "optimize/copyprop.h"
#include "optimize/constprop.h"
#include "optimize/redundelim.h"

#include "symbol/var.h"

#include "fun.h"

using namespace std;

#define SEMWARN(code, name) Error::semWarn(code, name)


Fun::Fun(bool ext, Tag t, std::string_view n, const vector<shared_ptr<Var>> &paraList)
    : externed(ext), type(t), name(n), paraVar(paraList)
{
    curEsp = Plat::stackBase;
    maxDepth = Plat::stackBase;

    int argOff = 4;
    for (auto para : paraVar) {
        para->setOffset(argOff);
        argOff += 4;
    }

    relocated = false;
    returnPoint = nullptr;
    dfg = nullptr;
}

Fun::~Fun() {
    if (dfg) {
        dfg = nullptr;
    }
}

void Fun::locate(shared_ptr<Var> var) {
    int size = var->getSize();
    size += (4 - size % 4) % 4;
    scopeEsp.back() += size;
    curEsp += size;
    var->setOffset(-curEsp);
}

bool Fun::match(shared_ptr<Fun> f) {
    if (name != f->name) {
        return false;
    }
    if (paraVar.size() != f->paraVar.size()) {
        return false;
    }

    for (size_t i = 0; i < paraVar.size(); ++i) {
        if (GenIR::typeCheck(paraVar[i], f->paraVar[i])) {
            if (paraVar[i]->getType() != f->paraVar[i]->getType()) {
                SEMWARN(cast_int(SemWarn::FUN_DEC_CONFLICT), name);
            }
        }
        else {
            return false;
        }
    }

    if (type != f->type) {
        SEMWARN(cast_int(SemWarn::FUN_RET_CONFLICT), name);
    }

    return true;
}

bool Fun::match(const vector<shared_ptr<Var>> &args) {
    if (paraVar.size() != args.size()) {
        return false;
    }
    for (size_t i = 0; i < paraVar.size(); ++i) {
        if (!GenIR::typeCheck(paraVar[i], args[i])) {
            return false;
        }
    }
    return true;
}

void Fun::define(const std::shared_ptr<Fun> def) {
    externed = false;
    paraVar = def->paraVar;
}

void Fun::addInst(shared_ptr<InterInst> inst) {
    interCode.addInst(inst);
}

void Fun::setReturnPoint(shared_ptr<InterInst> inst) {
    returnPoint = inst;
}

shared_ptr<InterInst> Fun::getReturnPoint() {
    return returnPoint;
}

void Fun::enterScope() {
    scopeEsp.emplace_back(0);
}

void Fun::leaveScope() {
    maxDepth = (curEsp > maxDepth) ? curEsp : maxDepth; // 计算最大深度
    curEsp -= scopeEsp.back();
    scopeEsp.pop_back();
}

void Fun::setExtern(bool ext) {
    externed = ext;
}

bool Fun::getExtern() const {
    return externed;
}

Tag Fun::getType() const {
    return type;
}

string &Fun::getName() {
    return name;
}

vector<shared_ptr<Var>> &Fun::getParaVar() {
    return paraVar;
}

int Fun::getMaxDep() const {
    return maxDepth;
}

void Fun::setMaxDep(int dep) {
    maxDepth = dep;
    // 设置函数栈帧被重定位标记, 用于生成不同的栈帧保护代码
    relocated = true;
}

bool Fun::isRelocated() const {
    return relocated;
}

void Fun::toString() const {
    printf("%s", tokenName[cast_int(type)]);
    printf(" %s", name.c_str());

    printf("(");
    for (size_t i = 0; i < paraVar.size(); i++) {
        printf("<%s>", paraVar[i]->getName().c_str());
        if (i != paraVar.size() - 1) {
            printf(", ");
        }
    }
    printf(")");

    if (externed) {
        printf(";\n");
    }
    else {
        printf(":\n");
        printf("\t\tmaxDepth = %d\n", maxDepth);
    }
}

void Fun::printInterCode() const {
    if (externed) {
        return;
    }
    printf("-------------<%s>Start--------------\n", name.c_str());
    interCode.toString();
    printf("--------------<%s>End---------------\n", name.c_str());
}

void Fun::printOptCode() const {
    if (externed) {
        return;
    }
    printf("-------------<%s>Start--------------\n", name.c_str());
    for (list<shared_ptr<InterInst>>::const_iterator i = optCode.begin(); i != optCode.end(); ++i) {
        (*i)->toString();
    }
    printf("--------------<%s>End---------------\n", name.c_str());
}

void Fun::optimize(shared_ptr<SymTab> tab) {
    if (externed) {
        return;
    }

    // 数据流图
    dfg = make_shared<DFG>(interCode);   // 创建数据流图
    if (Args::showBlock) {
        dfg->toString(); // 输出基本块和流图关系
    }
    if (!Args::opt) {
        return;
    }

    // 常量传播: 代数化简, 条件跳转优化, 不可达代码消除
    ConstPropagation conPro(dfg, tab, paraVar); // 常量传播
    conPro.propagate(); // 常量传播

//     // 冗余消除
//     RedundElim re(dfg, tab);
//     re.elimate();

    // 复写传播
    CopyPropagation cp(dfg);
    cp.propagate();

    // 活跃变量
    auto lv = make_shared<LiveVar>(dfg, tab, paraVar);
    lv->elimateDeadCode();

    // 优化结果存储在 optCode
    dfg->toCode(optCode);   // 导出数据流图为中间代码

#ifdef REG_OPT
    // 寄存器分配和局部变量栈地址重新计算
    CoGraph cg(optCode, paraVar, lv, shared_from_this());    // 初始化冲突图
    cg.alloc();                                 // 重新分配变量的寄存器和栈帧地址
#endif
}

// TODO
void Fun::genAsm(FILE *file) {
    if (externed) {
        return;
    }

    // 导出最终的代码, 如果优化则是优化后的中间代码, 否则就是普通的中间代码
    vector<shared_ptr<InterInst>> code;
    if (Args::opt) {    // 经过优化
        for (list<shared_ptr<InterInst>>::const_iterator it = optCode.begin(); it != optCode.end(); ++it) {
            code.emplace_back(*it);
        }
    }
    else {  // 未优化, 将中间代码导出
        code = interCode.getCode();
    }

    const char *pname = name.c_str();
    fprintf(file, "\nglobal %s\n", pname);
    fprintf(file, "%s:\n", pname);
    for (auto inst : code) {
        inst->toX86(file);
    }

//     // 窥孔优化
//     PeepHole ph(il);    // 窥孔优化器

//     if (Args::opt)
//         ph.filter();    // 优化过滤代码

//     il.outPut(file);
}
