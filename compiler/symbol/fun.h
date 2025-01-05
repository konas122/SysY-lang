#ifndef __COMPILER_SYMBOL_H__
#define __COMPILER_SYMBOL_H__

#include "common.h"
#include "lexer/token.h"

#include "ir/genir.h"
#include "ir/interCode.h"


class DFG;
class SymTab;
class InterInst;


class Fun
{
    // 基本信息
    bool externed;              // 声明或定义
    Tag type;                   // 变量类型
    std::string name;           // 变量名称
    std::vector<Var *> paraVar; // 形参变量列表

    // 临时变量地址分配
    int maxDepth;   // 栈的最大深度, 初始 0,标识函数栈分配的最大空间
    int curEsp;     // 当前栈指针位置, 初始化为 0, 即 ebp 存储点
    bool relocated; // 栈帧重定位标记

    // 作用域管理
    std::vector<int> scopeEsp;      // 当前作用域初始 esp, 动态控制作用域的分配和释放
    InterCode interCode;            // 中间代码
    InterInst *returnPoint;         // 返回点
    DFG *dfg;                       // 数据流图指针
    std::list<InterInst *> optCode; // 优化后的中间代码

public:
    Fun(bool ext, Tag t, const std::string &n, const std::vector<Var *> &paraList);
    ~Fun();

    // 声明定义与使用
    bool match(Fun *f);                         // 声明定义匹配
    bool match(const std::vector<Var *> &args); // 行参实参匹配
    void define(const Fun *def);                // 将函数声明转换为定义, 需要拷贝参数列表, 设定 extern

    // 作用域管理, 局部变量地址计算
    void enterScope();      // 进入一个新的作用域
    void leaveScope();      // 离开当前作用域
    void locate(Var *var);  // 定位局部变了栈帧偏移

    // 中间代码
    void addInst(InterInst *inst);          // 添加一条中间代码
    void setReturnPoint(InterInst *inst);   // 设置函数返回点
    InterInst *getReturnPoint();            // 获取函数返回点
    int getMaxDep() const;                  // 获取最大栈帧深度
    void setMaxDep(int dep);                // 设置最大栈帧深度
    void optimize(SymTab *tab);             // 执行优化操作

    bool getExtern() const;             // 获取 extern
    void setExtern(bool ext);           // 设置 extern
    Tag getType() const;                // 获取函数类型
    std::string &getName();             // 获取名字
    bool isRelocated() const;           // 栈帧是否重定位了
    std::vector<Var *> &getParaVar();   // 获取参数列表, 用于为参数生成加载代码

    void toString() const;      // 输出信息
    void printInterCode() const;// 输出中间代码
    void printOptCode() const;  // 输出优化后的中间代码
    void genAsm(FILE *file);    // 输出汇编代码
};

#endif
