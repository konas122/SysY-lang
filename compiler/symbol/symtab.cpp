#include "error.h"
#include "common.h"
#include "compiler.h"
#include "ir/genir.h"
#include "ir/interInst.h"
#include "symbol/var.h"
#include "symbol/fun.h"

#include "symtab.h"

using namespace std;

#define SEMERROR(code, name) Error::semError(code, name)

static const char *entryAsm =
"\nglobal _start\n\
_start:\n\
\tcall main\n\
\tmov ebx, eax\n\
\tmov eax, 1\n\
\tint 0x80\n";

shared_ptr<Var> SymTab::voidVar;
shared_ptr<Var> SymTab::zero;
shared_ptr<Var> SymTab::one;
shared_ptr<Var> SymTab::four;


SymTab::SymTab() {
    voidVar = make_shared<Var>();
    zero = make_shared<Var>(1);
    one = make_shared<Var>(1);
    four = make_shared<Var>(4);

    addVar(voidVar);
    addVar(one);
    addVar(zero);
    addVar(four);

    scopeId = 0;
    scopePath.emplace_back(0);
}

SymTab::~SymTab() {
    funTab.clear();
    varTab.clear();
    strTab.clear();
}

void SymTab::setIr(shared_ptr<GenIR> ir) {
    this->ir = ir;
}

vector<int> &SymTab::getScopePath() {
    return scopePath;
}

void SymTab::addVar(shared_ptr<Var> var) {
    if (varTab.count(var->getName()) == 0) {
        varTab[var->getName()] = vector<shared_ptr<Var>>();
        varTab[var->getName()].emplace_back(var);
        varList.emplace_back(var->getName());
    }
    else {
        vector<shared_ptr<Var>> &list = varTab[var->getName()];
        size_t i = 0;
        for (; i < list.size(); ++i) {
            if (list[i]->getPath().back() == var->getPath().back()) {
                break;
            }
        }
        if (list.size() == i || var->getName()[0] == '<') {
            list.emplace_back(var);
        }
        else {
            SEMERROR(cast_int(SemError::VAR_RE_DEF), var->getName());
            return;
        }
    }

    if (ir) {
        int flag = ir->genVarInit(var);
        if (curFun && flag) {
            curFun->locate(var);
        }
    }
}

void SymTab::addStr(shared_ptr<Var> v) {
    strTab[v->getName()] = v;
}

shared_ptr<Var> SymTab::getVar(const string &name) {
    shared_ptr<Var> select = nullptr;
    if (varTab.count(name)) {
        vector<shared_ptr<Var>> &list = varTab[name];
        int maxLen = 0;
        int pathLen = scopePath.size();
        for (size_t i = 0; i < list.size(); ++i) {
            int len = list[i]->getPath().size();
            if (len <= pathLen && list[i]->getPath()[len - 1] == scopePath[len - 1]) {
                if (len > maxLen) {
                    maxLen = len;
                    select = list[i];
                }
            }
        }
    }

    if (!select) {
        SEMERROR(cast_int(SemError::VAR_UN_DEC), name);
    }

    return select;
}

vector<shared_ptr<Var>> SymTab::getGlbVars() {
    vector<shared_ptr<Var>> glbVars;
    for (auto &varName : varList) {
        if (varName[0] == '<') {
            continue;
        }
        vector<shared_ptr<Var>> &list = varTab[varName];
        for (auto &var : list) {
            if (var->getPath().size() == 1) {
                glbVars.emplace_back(var);
                break;
            }
        }
    }
    return glbVars;
}

shared_ptr<Fun> SymTab::getFun(const string &name, const vector<shared_ptr<Var>> &args) {
    if (funTab.count(name)) {
        auto fun = funTab[name];
        if (!fun->match(args)) {
            SEMERROR(cast_int(SemError::FUN_CALL_ERR), name);
            return nullptr;
        }
        return fun;
    }
    SEMERROR(cast_int(SemError::FUN_UN_DEC), name);
    return nullptr;
}

shared_ptr<Fun> SymTab::getCurFun() {
    return curFun;
}

void SymTab::decFun(shared_ptr<Fun> fun) {
    fun->setExtern(true);
    if (funTab.count(fun->getName()) == 0) {
        funTab[fun->getName()] = fun;
        funList.emplace_back(fun->getName());
    }
    else {
        auto last = funTab[fun->getName()];
        if (!last->match(fun)) {
            SEMERROR(cast_int(SemError::FUN_DEC_ERR), fun->getName());
        }
    }
}

void SymTab::defFun(shared_ptr<Fun> fun) {
    if (fun->getExtern()) {
        SEMERROR(cast_int(SemError::EXTERN_FUN_DEF), fun->getName());
        fun->setExtern(false);
    }

    if (funTab.count(fun->getName()) == 0) {
        funTab[fun->getName()] = fun;
        funList.emplace_back(fun->getName());
    }
    else {
        auto last = funTab[fun->getName()];
        if (last->getExtern()) {
            if (!last->match(fun)) {
                SEMERROR(cast_int(SemError::FUN_DEC_ERR), fun->getName());
            }
            last->define(fun);
        }
        else {
            SEMERROR(cast_int(SemError::FUN_RE_DEF), fun->getName());
        }
        fun = last;
    }
    curFun = fun;
    ir->genFunHead(curFun);
}

void SymTab::endDefFun() {
    ir->genFunTail(curFun);
    curFun = nullptr;
}

void SymTab::addInst(shared_ptr<InterInst> inst) {
    if (curFun) {
        curFun->addInst(inst);
    }
}

void SymTab::enter() {
    scopeId++;
    scopePath.emplace_back(scopeId);
    if (curFun) {
        curFun->enterScope();
    }
}

void SymTab::leave() {
    scopePath.pop_back();
    if (curFun) {
        curFun->leaveScope();
    }
}

void SymTab::optimize() {
    for (const auto &funName : funList) {
        funTab[funName]->optimize(shared_from_this());
    }
}

void SymTab::toString() {
    printf("---------- Var ----------\n");
    for (const auto &varName : varList) {
        const vector<shared_ptr<Var>> &list = varTab[varName];
        printf("%s:\n", varName.c_str());
        for (const auto &var : list) {
            printf("\t");
            var->toString();
            printf("\n");
        }
    }

    printf("---------- Str -----------\n");
    for (auto &strIt : strTab) {
        printf("%s = %s\n", strIt.second->getName().c_str(), strIt.second->getStrVal().c_str());
    }

    printf("---------- Fun -----------\n");
    for (const auto &funName : funList) {
        funTab[funName]->toString();
    }
}

void SymTab::printInterCode() {
    for (const auto &funName : funList) {
        funTab[funName]->printInterCode();
    }
}

void SymTab::printOptCode() {
    for (const auto &funName : funList) {
        funTab[funName]->printOptCode();
    }
}

void SymTab::genData(FILE *file) {
    fprintf(file, "\nsection .data\n");

    vector<shared_ptr<Var>> glbVars = getGlbVars();   // 获取所有全局变量
    for (unsigned int i = 0; i < glbVars.size(); i++) {
        const shared_ptr<Var>var = glbVars[i];
        fprintf(file, "global %s\n", var->getName().c_str());
        fprintf(file, "\t%s ", var->getName().c_str());

        int typeSize = var->getType() == Tag::KW_CHAR ? 1 : 4;
        if (var->getArray()) {
            fprintf(file, "times %d ", var->getSize() / typeSize);
        }

        const char *type = var->getType() == Tag::KW_CHAR && !var->getPtr() ? "db" : "dd";
        fprintf(file, "%s ", type);

        if (!var->unInit()) {
            if (var->isBase()) {
                fprintf(file, "%d\n", var->getVal());
            }
            else {  // 字符指针初始化
                fprintf(file, "%s\n", var->getPtrVal().c_str());
            }
        }
        else {
            fprintf(file, "0\n");
        }
    }

    for (const auto &strIt : strTab) {
        const shared_ptr<Var>str = strIt.second;
        fprintf(file, "\t%s db %s\n", str->getName().c_str(), str->getRawStr().c_str());
    }
    fprintf(file, "\nsection .bss\n");
}

// TODO
void SymTab::genAsm(const std::string &fileName) {
    string newName = fileName;
    int pos = newName.rfind(".c");
    if (pos > 0 && static_cast<size_t>(pos) == newName.length() - 2) {
        newName.replace(pos, 2, ".s");
    }
    else {
        newName = newName + ".s";
    }
    FILE *file = fopen(newName.c_str(), "w"); // 创建输出文件

    if (Args::opt) {
        fprintf(file, "\n; optimized code\n");
    }
    else {
        fprintf(file, "\n; unoptimized code\n");
    }

    fprintf(file, "section .text\n");
    for (const auto &fun : funList) {
        funTab[fun]->genAsm(file);
    }
    fprintf(file, "%s", entryAsm);

    genData(file);  // 生成数据段

    fclose(file);
}
