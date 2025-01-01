#ifndef __COMPILER_PARSER_H__
#define __COMPILER_PARSER_H__

#include "common.h"

class Var;
class Fun;
class Lexer;
class Token;
class SymTab;
class GenIR;


class Parser
{
    // 文法开始
    void program();
    void segment();
    Tag type();

    // 声明与定义
    void def(bool ext, Tag t);
    Var *defdata(bool ext, Tag t);
    void deflist(bool ext, Tag t);
    Var *varrdef(bool ext, Tag t, bool ptr, const std::string &name);
    Var *init(bool ext, Tag t, bool ptr, const std::string &name);
    void idtail(bool ext, Tag t, bool ptr, const std::string &name);

    // 函数
    Var *paradatatail(Tag t, std::string &name);
    Var *paradata(Tag t);
    void para(std::vector<Var *> &list);
    void paralist(std::vector<Var *> &list);
    void funtail(Fun *f);
    void block();
    void subprogram();
    void localdef();

    // 语句
    void statement();
    void whilestat();
    void dowhilestat();
    void forstat();
    void forinit();
    void ifstat();
    void elsestat();
    void switchstat();
    void casestat(Var *cond);
    Var *caselabel();

    // 表达式
    Var *altexpr();
    Var *expr();
    Var *assexpr();
    Var *asstail(Var *lval);
    Var *orexpr();
    Var *ortail(Var *lval);
    Var *andexpr();
    Var *andtail(Var *lval);
    Var *cmpexpr();
    Var *cmptail(Var *lval);
    Tag cmps();
    Var *aloexpr();
    Var *alotail(Var *lval);
    Tag adds();
    Var *item();
    Var *itemtail(Var *lval);
    Tag muls();
    Var *factor();
    Tag lop();
    Var *val();
    Tag rop();
    Var *elem();
    Var *literal();
    Var *idexpr(const std::string& name);
    void realarg(std::vector<Var *> &args);
    void arglist(std::vector<Var *> &args);
    Var *arg();

    // 词法分析
    Lexer &lexer;           // 词法分析器
    Token *look = nullptr;  // 超前查看的字符

    // 符号表
    SymTab &symtab;

    // 中间代码生成器
    GenIR &ir;

    // 语法分析与错误修复
    void move();                                             // 移进
    bool match(Tag t);                                       // 匹配,成功则移进
    void recovery(bool cond, SynError lost, SynError wrong); // 错误修复

public:
    // 构造与初始化
    Parser(Lexer &lex, SymTab &tab, GenIR &inter);

    // 外部调用接口
    void analyse(); // 语法分析主程序
};

#endif

/**
 * <program>    -> <segment> <program> | ^
 * 
 * <segment>    -> rsv_extern <type> <def> | <type> <def>
 * <type>       -> rsv_int | rsv_char | rsv_bool | rsv_void
 * 
 * <def>        -> mul ident <init> <deflist> | ident <idtail>
 * <idtail>     -> <varrdef> <deflist> | lparen <para> rparen <funtail>
 * <deflist>    -> comma <defdata> <deflist> | semicon
 * <defdata>    -> ident <varrdef> | mul ident <init>
 * <varrdef>    -> lbrack num rbrack | <init>
 * <init>       -> assign <expr> | ^
 * 
 * <para>           -> <type> <paradata> <paralist> | ^
 * <paralist>       -> comma <type> <paradata> <paralist> | ^
 * <paradata>       -> mul ident | ident <paradatatail>
 * <paradatatail>   -> lbrack rbrack | lbrack num rbrack | ^
 * 
 * <funtail>    -> <block> | semicon
 * <block>      -> lbrac <subprogram> rbrac
 * <subprogram> -> <localdef> <subprogram> | <statements> <subprogram> | ^
 * <localdef>   -> <type> <defdata> <deflist>
 *
 * <statement>  -> <altexpr> semicon
 *               | <whilestat> | <forstat> | <dowhilestat>
 *               | <ifstat> | <switchstat>
 *               | rsv_break semicon
 *               | rsv_continue semicon
 *               | rsv_return <altexpr> semicon
 *
 * <whilestat>  -> rsv_while lparen <altexpr> rparen <block>
 * <block>      -> <block> | <statement>
 * 
 * <dowhilestat>    -> rsv_do <block> rsv_while lparen <altexpr> rparen semicon
 * <block>          -> <block> | <statement>
 * 
 * <forstat>    -> rsv_for lparen <forinit> semicon <altexpr> semicon <altexpr> rparen <block>
 * <forinit>    -> <localdef> | <altexpr>
 * <block>      -> <block> | <statement>
 * 
 * <ifstat>     -> rsv_if lparen <expr> rparen <block> <elsestat>
 * <elsestat>   -> rsv_else <block> | ^
 * 
 * <switchstat> -> rsv_switch lparen <expr> rparen lbrac <casestat> rbrac
 * <casestat>   -> rsv_case <caselabel> colon <subprogram> <casestat>
 *               | rsv_default colon <subprogram>
 * <caselabel>  -> <literal>
 * 
 * <altexpr>    -> <expr> | ^
 * <expr>       -> <assexpr>
 * <assexpr>    -> <orexpr> <asstail>
 * <asstail>    -> assign <assexpr> | ^
 * 
 * <orexpr>     -> <andexpr> <ortail>
 * <ortail>     -> or <andexpr> <ortail> | ^
 * <andexpr>    -> <cmpexpr> <andtail>
 * <andtail>    -> and <cmpexpr> <andtail> | ^
 * <cmpexpr>    -> <aloexpr> <cmptail>
 * <cmptail>    -> <cmps> <aloexpr> <cmptail> | ^
 * <cmps>       -> gt | ge | ls | le | equ | nequ
 * 
 * <aloexpr>    -> <item> <alotail>
 * <alotail>    -> <adds> <item> <alotails> | ^
 * <adds>       -> add | sub
 * 
 * <item>       -> <factor> <itemtail>
 * <itemtail>   -> <muls> <factor> <itemtail> | ^
 * <muls>       -> mul | div | mod
 * 
 * <factor>     -> <lops> <factor> | <val>
 * <val>        -> <elem> <rop>
 * <elem>       -> ident <idexpr> | lparen <expr> rparen | <literal>
 * <idexpr>     -> lbrack <expr> rbrack
 *               | lparen <realarg> rparen
 *               | ^
 * <realarg>    -> <arg> <arglist> | ^
 * <arglist>    -> comma <arg> <arglist> | ^
 * <arg>        -> <expr>
 * <lop>        -> not | sub | lea | mul | incr | decr
 * <literal>    -> number | string | char
 * <rop>        -> incr | desc | ^
 */
