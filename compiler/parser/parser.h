#ifndef __COMPILER_PARSER_H__
#define __COMPILER_PARSER_H__

#include <memory>

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
    std::shared_ptr<Var> defdata(bool ext, Tag t);
    void deflist(bool ext, Tag t);
    std::shared_ptr<Var> varrdef(bool ext, Tag t, bool ptr, std::string_view name);
    std::shared_ptr<Var> init(bool ext, Tag t, bool ptr, std::string_view name);
    void idtail(bool ext, Tag t, bool ptr, std::string_view name);

    // 函数
    std::shared_ptr<Var> paradatatail(Tag t, std::string &name);
    std::shared_ptr<Var> paradata(Tag t);
    void para(std::vector<std::shared_ptr<Var>> &list);
    void paralist(std::vector<std::shared_ptr<Var>> &list);
    void funtail(std::shared_ptr<Fun> f);
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
    void casestat(std::shared_ptr<Var> cond);
    std::shared_ptr<Var> caselabel();

    // 表达式
    std::shared_ptr<Var> altexpr();
    std::shared_ptr<Var> expr();
    std::shared_ptr<Var> assexpr();
    std::shared_ptr<Var> asstail(std::shared_ptr<Var> lval);
    std::shared_ptr<Var> orexpr();
    std::shared_ptr<Var> ortail(std::shared_ptr<Var> lval);
    std::shared_ptr<Var> andexpr();
    std::shared_ptr<Var> andtail(std::shared_ptr<Var> lval);
    std::shared_ptr<Var> cmpexpr();
    std::shared_ptr<Var> cmptail(std::shared_ptr<Var> lval);
    Tag cmps();
    std::shared_ptr<Var> aloexpr();
    std::shared_ptr<Var> alotail(std::shared_ptr<Var> lval);
    Tag adds();
    std::shared_ptr<Var> item();
    std::shared_ptr<Var> itemtail(std::shared_ptr<Var> lval);
    Tag muls();
    std::shared_ptr<Var> factor();
    Tag lop();
    std::shared_ptr<Var> val();
    Tag rop();
    std::shared_ptr<Var> elem();
    std::shared_ptr<Var> literal();
    std::shared_ptr<Var> idexpr(const std::string& name);
    void realarg(std::vector<std::shared_ptr<Var>> &args);
    void arglist(std::vector<std::shared_ptr<Var>> &args);
    std::shared_ptr<Var> arg();

    // 词法分析
    Lexer &lexer;           // 词法分析器
    std::unique_ptr<Token> look = nullptr;  // 超前查看的字符

    // 符号表
    std::shared_ptr<SymTab> symtab;

    // 中间代码生成器
    std::shared_ptr<GenIR> ir;

    // 语法分析与错误修复
    void move();                                             // 移进
    bool match(Tag t);                                       // 匹配,成功则移进
    void recovery(bool cond, SynError lost, SynError wrong); // 错误修复

public:
    const Parser &operator=(const Parser &&rhs) = delete;

    // 构造与初始化
    Parser(Lexer &lex, std::shared_ptr<SymTab> tab, std::shared_ptr<GenIR> inter);

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
