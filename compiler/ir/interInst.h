#ifndef __IR_INTERINST_H__
#define __IR_INTERINST_H__

#include <memory>

#include "common.h"
#include "optimize/set.h"

class Var;
class Fun;
class Block;


class InterInst
{
    FILE *file;

    std::string label;
    Operator op;
    Var *result;
    Var *arg1;
    Var *arg2;
    Fun *fun;
    std::shared_ptr<InterInst> target;

    bool first;     // 是否是首指令
    void init();    // 初始化

public:
    std::shared_ptr<Block> block;   // 指令所在的基本块指针

    // 数据流信息
    std::vector<double> inVals;     // 常量传播 in 集合
    std::vector<double> outVals;    // 常量传播 out 集合
    Set e_use;                      // 使用的表达式集合
    Set e_kill;                     // 杀死的表达式集合
    RedundInfo info;                // 冗余删除数据流信息
    CopyInfo copyInfo;              // 复写传播数据流信息
    LiveInfo liveInfo;              // 活跃变量数据流信息
    bool isDead;                    // 标识指令是否是死代码

    InterInst(Operator op, Var *rs, Var *arg1, Var *arg2 = nullptr);    // 一般运算指令
    InterInst(Operator op, Fun *fun, Var *rs = nullptr);                // 函数调用指令, ENTRY,EXIT
    explicit InterInst(Operator op, Var *arg1 = nullptr);               // 参数进栈指令, NOP
    InterInst();                                                        // 产生唯一标号
    InterInst(Operator op, std::shared_ptr<InterInst> tar, Var *arg1 = nullptr, Var *arg2 = nullptr);   // 条件跳转指令, return

    void replace(Operator op, Var *rs, Var *arg1, Var *arg2 = nullptr);                     // 替换表达式指令信息, 用于常量表达式处理
    void replace(Operator op, std::shared_ptr<InterInst> tar, Var *arg1 = nullptr, Var *arg2 = nullptr);    // 替换跳转指令信息, 条件跳转优化
    ~InterInst();

    void setFirst();    // 标记首指令

    bool isJcond() const;   // 是否条件转移指令 JT, JF, Jcond
    bool isJmp() const;     // 是否直接转移指令 JMP, return
    bool isFirst() const;   // 是首指令
    bool isLb() const;      // 是否是标签
    bool isDec() const;     // 是否是声明
    bool isExpr() const;    // 是基本类型表达式运算, 可以对指针取值
    bool unknown() const;   // 不确定运算结果影响的运算 (指针赋值, 函数调用)

    Operator getOp() const; // 获取操作符
    void callToProc();      // 替换操作符, 用于将 CALL 转化为 PROC
    std::shared_ptr<InterInst> getTarget(); // 获取跳转指令的目标指令
    Var *getResult();       // 获取返回值
    Var *getArg1();         // 获取第一个参数
    Var *getArg2();         // 获取第二个参数
    std::string getLabel(); // 获取标签
    Fun *getFun();          // 获取函数对象
    void setArg1(Var *arg1);// 设置第一个参数
    void toString() const;  // 输出指令

public:
    void initVar(const Var *var);
    void leaVar(const std::string &reg32, const Var *var);
    void loadVar(const std::string &reg32, const std::string &reg8, const Var *var);
    void storeVar(const std::string &reg32, const std::string &reg8, const Var *var);
    void toX86(FILE *file);
};

#endif
