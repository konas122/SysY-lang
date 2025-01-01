#ifndef __COMPILER_GENIR_H__
#define __COMPILER_GENIR_H__


#include "common.h"

class Var;
class Fun;
class SymTab;
class InterInst;


class GenIR
{
    static int lbNum;   // 标签号码, 用于产生唯一的标签

    SymTab &symtab;     // 符号表

    // break continue 辅助标签列表
    std::vector<InterInst *> heads;
    std::vector<InterInst *> tails;
    void push(InterInst *head, InterInst *tail);    // 添加一个作用域
    void pop();                                     // 删除一个作用域

    // 函数调用
    void genPara(Var *arg); // 参数传递语句

    // 双目运算
    Var *genAssign(Var *lval, Var *rval);   // 赋值语句

    Var *genOr(Var *lval, Var *rval);   // 或运算语句
    Var *genAnd(Var *lval, Var *rval);  // 与运算语句
    Var *genGt(Var *lval, Var *rval);   // 大于语句
    Var *genGe(Var *lval, Var *rval);   // 大于等于语句
    Var *genLt(Var *lval, Var *rval);   // 小于语句
    Var *genLe(Var *lval, Var *rval);   // 小于等于语句
    Var *genEqu(Var *lval, Var *rval);  // 等于语句
    Var *genNequ(Var *lval, Var *rval); // 不等于语句
    Var *genAdd(Var *lval, Var *rval);  // 加法语句
    Var *genSub(Var *lval, Var *rval);  // 减法语句
    Var *genMul(Var *lval, Var *rval);  // 乘法语句
    Var *genDiv(Var *lval, Var *rval);  // 除法语句
    Var *genMod(Var *lval, Var *rval);  // 取模

    // 单目运算
    Var *genNot(Var *val);      // 非
    Var *genMinus(Var *val);    // 负
    Var *genIncL(Var *val);     // 左自加语句
    Var *genDecL(Var *val);     // 右自减语句
    Var *genLea(Var *val);      // 取址语句
    Var *genPtr(Var *val);      // 指针取值语句
    Var *genIncR(Var *val);     // 右自加语句
    Var *genDecR(Var *val);     // 右自减

public:
    GenIR(SymTab &tab); // 重置内部数据

    Var *genAssign(Var *val);

    // 产生符号和语句
    Var *genArray(Var *array, Var *index);
    Var *genCall(Fun *function, std::vector<Var *> &args);
    Var *genTwoOp(Var *lval, Tag opt, Var *rval);
    Var *genOneOpLeft(Tag opt, Var *val);
    Var *genOneOpRight(Var *val, Tag opt);

    // 产生复合语句
    void genWhileHead(InterInst *&_while, InterInst *&_exit);
    void genWhileCond(Var *cond, InterInst *_exit);
    void genWhileTail(InterInst *&_while, InterInst *&_exit);
    void genDoWhileHead(InterInst *&_do, InterInst *&_exit);
    void genDoWhileTail(Var *cond, InterInst *_do, InterInst *_exit);
    void genForHead(InterInst *&_for, InterInst *&_exit);
    void genForCondBegin(Var *cond, InterInst *&_step, InterInst *&_block, InterInst *_exit);
    void genForCondEnd(InterInst *_for, InterInst *_block);
    void genForTail(InterInst *&_step, InterInst *&_exit);

    void genIfHead(Var *cond, InterInst *&_else);
    void genIfTail(InterInst *&_else);
    void genElseHead(InterInst *_else, InterInst *&_exit);
    void genElseTail(InterInst *&_exit);
    void genSwitchHead(InterInst *&_exit);
    void genSwitchTail(InterInst *_exit);
    void genCaseHead(Var *cond, Var *lb, InterInst *&_case_exit);
    void genCaseTail(InterInst *_case_exit);

    // 产生特殊语句
    void genBreak();
    void genContinue();
    void genReturn(Var *ret);
    bool genVarInit(Var*var);
    void genFunHead(Fun*function);
    void genFunTail(Fun*function);

    // 全局函数
    static std::string genLb();                     // 产生唯一的标签
    static bool typeCheck(Var *lval, Var *rval);    // 检查类型是否可以转换
};

#endif