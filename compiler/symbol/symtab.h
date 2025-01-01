#ifndef __COMPILER_SYMTAB_H__
#define __COMPILER_SYMTAB_H__

#include <unordered_map>

#include "common.h"


class Var;
class Fun;
class GenIR;
class InterInst;


class SymTab
{
    std::vector<std::string> varList;
    std::vector<std::string> funList;

    std::unordered_map<std::string, std::vector<Var *>> varTab;
    std::unordered_map<std::string, Var *> strTab;
    std::unordered_map<std::string, Fun *> funTab;

    Fun *curFun;
    int scopeId;
    std::vector<int> scopePath;

    GenIR *ir = nullptr;

public:
    // 特殊变量
    static Var *voidVar;
    static Var *zero;
    static Var *one;
    static Var *four;

    SymTab();
    ~SymTab();

    // 符号表作用域管理
    void enter();   // 进入局部作用域
    void leave();   // 离开局部作用域

    // 变量管理
    void addVar(Var *v);        // 添加一个变量
    void addStr(Var *v);        // 添加一个字符串常量
    Var *getVar(const std::string &name);   // 获取一个变量
    std::vector<Var*> getGlbVars(); // 获取所有全局变量

    // 函数管理
    void decFun(Fun *fun);  // 声明一个函数
    void defFun(Fun *fun);  // 定义一个函数
    void endDefFun();       // 结束定义一个函数

    // 根据调用类型, 获取一个函数
    Fun *getFun(const std::string &name, const std::vector<Var *> &args);

    void setIr(GenIR *ir);
    std::vector<int> &getScopePath();
    Fun *getCurFun();               // 获取当前分析的函数
    void toString();                // 输出信息
    void addInst(InterInst *inst);  // 添加一条中间代码
    void printInterCode();          // 输出中间指令
    void optimize();                // 执行优化操作
    void printOptCode();            // 输出中间指令
    void genData(FILE *file);       // 输出数据
    void genAsm(const std::string &fileName);    // 输出汇编文件
};

#endif
