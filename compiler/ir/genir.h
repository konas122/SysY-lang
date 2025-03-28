#ifndef __COMPILER_GENIR_H__
#define __COMPILER_GENIR_H__

#include <memory>
#include "common.h"

class Var;
class Fun;
class SymTab;
class InterInst;


class GenIR : public std::enable_shared_from_this<GenIR>
{
    static int lbNum;   // 标签号码, 用于产生唯一的标签

    std::weak_ptr<SymTab> symtab; // 符号表

    // break continue 辅助标签列表
    std::vector<std::shared_ptr<InterInst>> heads;
    std::vector<std::shared_ptr<InterInst>> tails;
    void push(std::shared_ptr<InterInst>head, std::shared_ptr<InterInst>tail);    // 添加一个作用域
    void pop();                                     // 删除一个作用域

    // 函数调用
    void genPara(std::shared_ptr<Var> arg); // 参数传递语句

    // 双目运算
    std::shared_ptr<Var> genAssign(std::shared_ptr<Var> lval, std::shared_ptr<Var> rval);   // 赋值语句

    std::shared_ptr<Var> genOr(std::shared_ptr<Var> lval, std::shared_ptr<Var> rval);   // 或运算语句
    std::shared_ptr<Var> genAnd(std::shared_ptr<Var> lval, std::shared_ptr<Var> rval);  // 与运算语句
    std::shared_ptr<Var> genGt(std::shared_ptr<Var> lval, std::shared_ptr<Var> rval);   // 大于语句
    std::shared_ptr<Var> genGe(std::shared_ptr<Var> lval, std::shared_ptr<Var> rval);   // 大于等于语句
    std::shared_ptr<Var> genLt(std::shared_ptr<Var> lval, std::shared_ptr<Var> rval);   // 小于语句
    std::shared_ptr<Var> genLe(std::shared_ptr<Var> lval, std::shared_ptr<Var> rval);   // 小于等于语句
    std::shared_ptr<Var> genEqu(std::shared_ptr<Var> lval, std::shared_ptr<Var> rval);  // 等于语句
    std::shared_ptr<Var> genNequ(std::shared_ptr<Var> lval, std::shared_ptr<Var> rval); // 不等于语句
    std::shared_ptr<Var> genAdd(std::shared_ptr<Var> lval, std::shared_ptr<Var> rval);  // 加法语句
    std::shared_ptr<Var> genSub(std::shared_ptr<Var> lval, std::shared_ptr<Var> rval);  // 减法语句
    std::shared_ptr<Var> genMul(std::shared_ptr<Var> lval, std::shared_ptr<Var> rval);  // 乘法语句
    std::shared_ptr<Var> genDiv(std::shared_ptr<Var> lval, std::shared_ptr<Var> rval);  // 除法语句
    std::shared_ptr<Var> genMod(std::shared_ptr<Var> lval, std::shared_ptr<Var> rval);  // 取模

    // 单目运算
    std::shared_ptr<Var> genNot(std::shared_ptr<Var> val);      // 非
    std::shared_ptr<Var> genMinus(std::shared_ptr<Var> val);    // 负
    std::shared_ptr<Var> genIncL(std::shared_ptr<Var> val);     // 左自加语句
    std::shared_ptr<Var> genDecL(std::shared_ptr<Var> val);     // 右自减语句
    std::shared_ptr<Var> genLea(std::shared_ptr<Var> val);      // 取址语句
    std::shared_ptr<Var> genPtr(std::shared_ptr<Var> val);      // 指针取值语句
    std::shared_ptr<Var> genIncR(std::shared_ptr<Var> val);     // 右自加语句
    std::shared_ptr<Var> genDecR(std::shared_ptr<Var> val);     // 右自减

public:
    const GenIR &operator=(const GenIR &&rhs) = delete;

    static std::shared_ptr<GenIR> create(std::shared_ptr<SymTab> tab);

    void init(std::shared_ptr<SymTab> tab); // 重置内部数据

    std::shared_ptr<Var> genAssign(std::shared_ptr<Var> val);

    // 产生符号和语句
    std::shared_ptr<Var> genArray(std::shared_ptr<Var> array, std::shared_ptr<Var> index);
    std::shared_ptr<Var> genCall(std::shared_ptr<Fun> function, const std::vector<std::shared_ptr<Var>> &args);
    std::shared_ptr<Var> genTwoOp(std::shared_ptr<Var> lval, Tag opt, std::shared_ptr<Var> rval);
    std::shared_ptr<Var> genOneOpLeft(Tag opt, std::shared_ptr<Var> val);
    std::shared_ptr<Var> genOneOpRight(std::shared_ptr<Var> val, Tag opt);

    // 产生复合语句
    void genWhileHead(std::shared_ptr<InterInst>& _while, std::shared_ptr<InterInst>& _exit);
    void genWhileCond(std::shared_ptr<Var> cond, std::shared_ptr<InterInst> _exit);
    void genWhileTail(std::shared_ptr<InterInst>& _while, const std::shared_ptr<InterInst>& _exit);
    void genDoWhileHead(std::shared_ptr<InterInst>& _do, std::shared_ptr<InterInst>& _exit);
    void genDoWhileTail(std::shared_ptr<Var> cond, std::shared_ptr<InterInst> _do, std::shared_ptr<InterInst> _exit);
    void genForHead(std::shared_ptr<InterInst>& _for, std::shared_ptr<InterInst>& _exit);
    void genForCondBegin(std::shared_ptr<Var> cond, std::shared_ptr<InterInst>& _step, std::shared_ptr<InterInst>& _block, std::shared_ptr<InterInst> _exit);
    void genForCondEnd(std::shared_ptr<InterInst> _for, std::shared_ptr<InterInst> _block);
    void genForTail(std::shared_ptr<InterInst>& _step, const std::shared_ptr<InterInst>& _exit);

    void genIfHead(std::shared_ptr<Var> cond, std::shared_ptr<InterInst>& _else);
    void genIfTail(const std::shared_ptr<InterInst>& _else);
    void genElseHead(std::shared_ptr<InterInst> _else, std::shared_ptr<InterInst>& _exit);
    void genElseTail(std::shared_ptr<InterInst>& _exit);
    void genSwitchHead(std::shared_ptr<InterInst>& _exit);
    void genSwitchTail(std::shared_ptr<InterInst> _exit);
    void genCaseHead(std::shared_ptr<Var> cond, std::shared_ptr<Var> lb, std::shared_ptr<InterInst>& _case_exit);
    void genCaseTail(std::shared_ptr<InterInst> _case_exit);

    // 产生特殊语句
    void genBreak();
    void genContinue();
    void genReturn(std::shared_ptr<Var> ret);
    bool genVarInit(std::shared_ptr<Var> var);
    void genFunHead(std::shared_ptr<Fun> function);
    void genFunTail(std::shared_ptr<Fun> function);

    // 全局函数
    static std::string genLb(); // 产生唯一的标签
    static bool typeCheck(const std::shared_ptr<Var> lval, const std::shared_ptr<Var> rval);    // 检查类型是否可以转换
};

#endif
