#ifndef __COMPILER_SYMTAB_H__
#define __COMPILER_SYMTAB_H__

#include <memory>
#include <unordered_map>

#include "common.h"


class Var;
class Fun;
class GenIR;
class InterInst;


class SymTab : public std::enable_shared_from_this<SymTab>
{
    std::vector<std::string> varList;
    std::vector<std::string> funList;

    std::unordered_map<std::string, std::vector<std::shared_ptr<Var>>> varTab;
    std::unordered_map<std::string, std::shared_ptr<Var>> strTab;
    std::unordered_map<std::string, std::shared_ptr<Fun>> funTab;

    std::shared_ptr<Fun> curFun;
    int scopeId;
    std::vector<int> scopePath;

    std::shared_ptr<GenIR> ir;

public:
    // 特殊变量
    static std::shared_ptr<Var> voidVar;
    static std::shared_ptr<Var> zero;
    static std::shared_ptr<Var> one;
    static std::shared_ptr<Var> four;

    SymTab(const SymTab &rhs) = delete;
    SymTab &operator=(const SymTab &rhs) = delete;

    SymTab();
    ~SymTab();

    // 符号表作用域管理
    void enter();   // 进入局部作用域
    void leave();   // 离开局部作用域

    // 变量管理
    void addVar(std::shared_ptr<Var> v);    // 添加一个变量
    void addStr(std::shared_ptr<Var> v);    // 添加一个字符串常量
    std::shared_ptr<Var> getVar(const std::string &name);   // 获取一个变量
    std::vector<std::shared_ptr<Var>> getGlbVars(); // 获取所有全局变量

    // 函数管理
    void decFun(std::shared_ptr<Fun> fun);  // 声明一个函数
    void defFun(std::shared_ptr<Fun> fun);  // 定义一个函数
    void endDefFun();       // 结束定义一个函数

    // 根据调用类型, 获取一个函数
    std::shared_ptr<Fun>getFun(const std::string &name, const std::vector<std::shared_ptr<Var>> &args);

    void setIr(std::shared_ptr<GenIR> ir);
    std::vector<int> &getScopePath();
    std::shared_ptr<Fun> getCurFun();   // 获取当前分析的函数
    void toString();                    // 输出信息
    void addInst(std::shared_ptr<InterInst> inst);  // 添加一条中间代码
    void printInterCode();          // 输出中间指令
    void optimize();                // 执行优化操作
    void printOptCode();            // 输出中间指令
    void genData(FILE *file);       // 输出数据
    void genAsm(const std::string &fileName);    // 输出汇编文件
};

#endif
