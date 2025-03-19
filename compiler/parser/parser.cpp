#include <cassert>

#include "error.h"
#include "compiler.h"

#include "ir/genir.h"
#include "lexer/token.h"
#include "lexer/lexer.h"
#include "symbol/fun.h"
#include "symbol/var.h"
#include "symbol/symtab.h"

#include "parser.h"

using namespace std;


#define SYNERROR(code, t) Error::synError(code, t)

#define _(T) || look->tag == T
#define F(C) look->tag == C

// 类型
#define TYPE_FIRST F(Tag::KW_INT) _(Tag::KW_CHAR) _(Tag::KW_VOID)
// 表达式
#define EXPR_FIRST F(Tag::LPAREN) _(Tag::NUM) _(Tag::CH) _(Tag::STR) _(Tag::ID) _(Tag::NOT) _(Tag::SUB) _(Tag::LEA) _(Tag::MUL) _(Tag::INC) _(Tag::DEC)
// 左值运算
#define LVAL_OPR F(Tag::ASSIGN) _(Tag::OR) _(Tag::AND) _(Tag::GT) _(Tag::GE) _(Tag::LT) _(Tag::LE) \
                    _(Tag::EQU) _(Tag::NEQU) _(Tag::ADD) _(Tag::SUB) _(Tag::MUL) _(Tag::DIV) _(Tag::MOD) _(Tag::INC) _(Tag::DEC)
// 右值运算
#define RVAL_OPR F(Tag::OR) _(Tag::AND) _(Tag::GT) _(Tag::GE) _(Tag::LT) _(Tag::LE) _(Tag::EQU) \
                    _(Tag::NEQU) _(Tag::ADD) _(Tag::SUB) _(Tag::MUL) _(Tag::DIV) _(Tag::MOD)
// 语句
#define STATEMENT_FIRST (EXPR_FIRST) _(Tag::SEMICON) _(Tag::KW_WHILE) _(Tag::KW_FOR) _(Tag::KW_DO) _(Tag::KW_IF) \
    _(Tag::KW_SWITCH) _(Tag::KW_RETURN) _(Tag::KW_BREAK) _(Tag::KW_CONTINUE)


Parser::Parser(Lexer& lex, SymTab &tab, GenIR &inter)
    : lexer(lex), symtab(tab), ir(inter)
{}

void Parser::analyse() {
    move();
    program();
}

void Parser::move() {
    look = lexer.tokenize();
    if (Args::showToken) {
        cout << look->toString() << endl;
    }
}

bool Parser::match(Tag need) {
    if (look->tag == need) {
        move();
        return true;
    }
    return false;
}

void Parser::recovery(bool cond, SynError lost, SynError wrong) {
    if (cond) {
        SYNERROR(cast_int(lost), look.get());
    }
    else {
        SYNERROR(cast_int(wrong), look.get());
        move();
    }
}

/**
 *  <program> -> <segment> <program> | ^
 */
void Parser::program() {
    if (F(Tag::END)) {
        return;
    }
    segment();
    program();
}

/**
 * <segment> -> rsv_extern <type> <def> | <type> <def>
 */
void Parser::segment() {
    bool ext = match(Tag::KW_EXTERN);
    Tag t = type();
    def(ext, t);
}

/**
 * <type> -> rsv_int | rsv_char | rsv_bool | rsv_void
 */
Tag Parser::type() {
    Tag tmp = Tag::KW_INT;
    if (TYPE_FIRST) {
        tmp = look->tag;
        move();
    }
    else {
        recovery(F(Tag::ID) _(Tag::MUL), SynError::TYPE_LOST, SynError::TYPE_WRONG);
    }
    return tmp;
}

/**
 * <defdata> -> ident <varrdef> | mul ident <init>
 */
Var *Parser::defdata(bool ext, Tag t) {
    string name;
    if (F(Tag::ID)) {
        name = static_cast<Id *>(look.get())->name;
        move();
        return varrdef(ext, t, false, name);
    }
    else if (match(Tag::MUL)) {
        if (F(Tag::ID)) {
            name = static_cast<Id *>(look.get())->name;
            move();
        }
        else {
            recovery(F(Tag::SEMICON) _(Tag::COMMA) _(Tag::ASSIGN), SynError::ID_LOST, SynError::ID_WRONG);
        }
        return init(ext, t, true, name);
    }
    recovery(F(Tag::SEMICON) _(Tag::COMMA) _(Tag::ASSIGN)_(Tag::LBRACK), SynError::ID_LOST, SynError::ID_WRONG);
    return varrdef(ext, t, false, name);
}

/**
 * <deflist> -> comma <defdata> <deflist> | semicon
 */
void Parser::deflist(bool ext, Tag t) {
    if (match(Tag::COMMA)) {
        symtab.addVar(defdata(ext, t));
        deflist(ext, t);
    }
    else if (match(Tag::SEMICON)) {
        return;
    }
    else {
        if (F(Tag::ID)_(Tag::MUL)) {
            recovery(1, SynError::COMMA_LOST, SynError::COMMA_WRONG);
            symtab.addVar(defdata(ext, t));
            deflist(ext, t);
        }
        else {
            recovery(TYPE_FIRST || STATEMENT_FIRST || F(Tag::KW_EXTERN) _(Tag::RBRACE),
                     SynError::SEMICON_LOST, SynError::SEMICON_WRONG);
        }
    }
}

/**
 * <varrdef> -> lbrack num rbrack | <init>
 */
Var *Parser::varrdef(bool ext, Tag t, bool ptr, const string &name) {
    if (match(Tag::LBRACK)) {
        int len = 0;
        if (F(Tag::NUM)) {
            len = static_cast<Num *>(look.get())->val;
            move();
        }
        else {
            recovery(F(Tag::RBRACK), SynError::NUM_LOST, SynError::NUM_WRONG);
        }

        if (!match(Tag::RBRACK)) {
            recovery(F(Tag::COMMA) _(Tag::SEMICON), SynError::RBRACK_LOST, SynError::RBRACK_WRONG);
        }

        return new Var(symtab.getScopePath(), ext, t, name, len);
    }
    return init(ext, t, ptr, name);
}

/**
 * <init> -> assign <expr> | ^
 */
Var *Parser::init(bool ext, Tag t, bool ptr, const string &name) {
    Var *initVal = nullptr;
    if (match(Tag::ASSIGN)) {
        initVal = expr();
    }
    return new Var(symtab.getScopePath(), ext, t, ptr, name, initVal);
}

/**
 * <def> -> mul ident <init> <deflist> | ident <idtail>
 */
void Parser::def(bool ext, Tag t) {
    string name;
    if (match(Tag::MUL)) {
        if (F(Tag::ID)) {
            name = static_cast<Id *>(look.get())->name;
            move();
        }
        else {
            recovery(F(Tag::SEMICON) _(Tag::COMMA) _(Tag::ASSIGN), SynError::ID_LOST, SynError::ID_WRONG);
        }
        symtab.addVar(init(ext, t, true, name));
        deflist(ext, t);
    }
    else {
        if (F(Tag::ID)) {
            name = static_cast<Id *>(look.get())->name;
            move();
        }
        else {
            recovery(F(Tag::SEMICON) _(Tag::COMMA) _(Tag::ASSIGN) _(Tag::LPAREN) _(Tag::LBRACK), SynError::ID_LOST, SynError::ID_WRONG);
        }
        idtail(ext, t, false, name);
    }
}

/**
 * <idtail> -> <varrdef> <deflist> | lparen <para> rparen <funtail>
 */
void Parser::idtail(bool ext, Tag t, [[maybe_unused]] bool ptr, const string &name) {
    if (match(Tag::LPAREN)) {   // 函数
        symtab.enter();
        vector<Var *> paraList;
        para(paraList);
        if (!match(Tag::RPAREN)) {
            recovery(F(Tag::LBRACK) _(Tag::SEMICON), SynError::RPAREN_LOST, SynError::RPAREN_WRONG);
        }
        Fun *fun = new Fun(ext, t, name, paraList);
        funtail(fun);
        symtab.leave();
    }
    else {
        symtab.addVar(varrdef(ext, t, false, name));
        deflist(ext, t);
    }
}

/**
 * <paradatatail> -> lbrack rbrack | lbrack num rbrack | ^
 */
Var *Parser::paradatatail(Tag t, string &name) {
    if (match(Tag::LBRACK)) {
        int len = 1;
        if (F(Tag::NUM)) {
            len = static_cast<Num *>(look.get())->val;
            move();
        }
        if (!match(Tag::RBRACK)) {
            recovery(F(Tag::COMMA) _(Tag::RPAREN), SynError::RBRACK_LOST, SynError::RBRACK_WRONG);
        }
        return new Var(symtab.getScopePath(), false, t, name, len);
    }
    return new Var(symtab.getScopePath(), false, t, false, name);
}

/**
 * <paradata> -> mul ident | ident <paradatatail>
 */
Var *Parser::paradata(Tag t) {
    string name;
    if (match(Tag::MUL)) {
        if (F(Tag::ID)) {
            name = static_cast<Id *>(look.get())->name;
            move();
        }
        else {
            recovery(F(Tag::COMMA) _(Tag::RPAREN), SynError::ID_LOST, SynError::ID_WRONG);
        }
        return new Var(symtab.getScopePath(), false, t, true, name);
    }
    else if (F(Tag::ID)) {
        name = static_cast<Id *>(look.get())->name;
        move();
        return paradatatail(t, name);
    }
    recovery(F(Tag::COMMA) _(Tag::RPAREN) _(Tag::LBRACK), SynError::ID_LOST, SynError::ID_WRONG);
    return new Var(symtab.getScopePath(), false, t, false, name);
}

/**
 * <para> -> <type> <paradata> <paralist> | ^
 */
void Parser::para(vector<Var *>& list) {
    if (F(Tag::RPAREN)) {
        return;
    }
    Tag t = type();
    Var *v = paradata(t);
    symtab.addVar(v);
    list.emplace_back(v);
    paralist(list);
}

/**
 * <paralist> -> comma <type> <paradata> <paralist> | ^
 */
void Parser::paralist(vector<Var *>&list) {
    if (match(Tag::COMMA)) {
        Tag t = type();
        Var *v = paradata(t);
        symtab.addVar(v);
        list.emplace_back(v);
        paralist(list);
    }
}

/**
 * <funtail> -> <block> | semicon
 */
void Parser::funtail(Fun *f) {
    if (match(Tag::SEMICON)) {
        symtab.decFun(f);
    }
    else {
        symtab.defFun(f);
        block();
        symtab.endDefFun();
    }
}

/**
 * <block> -> lbrac <subprogram> rbrac
 */
void Parser::block() {
    if (!match(Tag::LBRACE)) {
        recovery(TYPE_FIRST || STATEMENT_FIRST || F(Tag::RBRACE), SynError::LBRACE_LOST, SynError::LBRACE_WRONG);
    }
    subprogram();
    if (!match(Tag::RBRACE)) {
        recovery(TYPE_FIRST || STATEMENT_FIRST || F(Tag::KW_EXTERN) _(Tag::KW_ELSE) _(Tag::KW_CASE) _(Tag::KW_DEFAULT),
                 SynError::RBRACE_LOST, SynError::RBRACE_WRONG);
    }
}

/**
 * <subprogram> -> <localdef> <subprogram> | <statements> <subprogram> | ^
 */
void Parser::subprogram() {
    if (TYPE_FIRST) {
        localdef();
        subprogram();
    }
    else if (STATEMENT_FIRST) {
        statement();
        subprogram();
    }
}

/**
 * <localdef> -> <type> <defdata> <deflist>
 */
void Parser::localdef() {
    Tag t = type();
    symtab.addVar(defdata(false, t));
    deflist(false, t);
}

/**
 * <statement> -> <altexpr> semicon
 *              | <whilestat> | <forstat> | <dowhilestat>
 *              | <ifstat> | <switchstat>
 *              | rsv_break semicon
 *              | rsv_continue semicon
 *              | rsv_return <altexpr> semicon
 */
void Parser::statement() {
    switch (look->tag) {
    case Tag::KW_WHILE:
        whilestat();
        break;

    case Tag::KW_FOR:
        forstat();
        break;

    case Tag::KW_DO:
        dowhilestat();
        break;

    case Tag::KW_IF:
        ifstat();
        break;
    case Tag::KW_SWITCH:
        switchstat();
        break;

    case Tag::KW_BREAK:
        ir.genBreak();              // 产生 break 语句
        move();
        if (!match(Tag::SEMICON)) {
            recovery(TYPE_FIRST || STATEMENT_FIRST || F(Tag::RBRACE), SynError::SEMICON_LOST, SynError::SEMICON_WRONG);
        }
        break;

    case Tag::KW_CONTINUE:
        ir.genContinue();           // 产生 continue 语句
        move();
        if (!match(Tag::SEMICON)) {
            recovery(TYPE_FIRST || STATEMENT_FIRST || F(Tag::RBRACE), SynError::SEMICON_LOST, SynError::SEMICON_WRONG);
        }
        break;

    case Tag::KW_RETURN:
        move();
        ir.genReturn(altexpr());    // 产生 return 语句
        if (!match(Tag::SEMICON)) {
            recovery(TYPE_FIRST || STATEMENT_FIRST || F(Tag::RBRACE), SynError::SEMICON_LOST, SynError::SEMICON_WRONG);
        }
        break;

    default:
        altexpr();
        if (!match(Tag::SEMICON)) {
            recovery(TYPE_FIRST || STATEMENT_FIRST || F(Tag::RBRACE), SynError::SEMICON_LOST, SynError::SEMICON_WRONG);
        }
    }
}

/**
 * <whilestat> -> rsv_while lparen <altexpr> rparen <block>
 * <block> -> <block> | <statement>
 */
void Parser::whilestat() {
    symtab.enter();
    InterInst *_while, *_exit;      // 标签

    ir.genWhileHead(_while, _exit); // while 循环头部
    match(Tag::KW_WHILE);

    if (!match(Tag::LPAREN)) {
        recovery(EXPR_FIRST || F(Tag::RPAREN), SynError::LPAREN_LOST, SynError::LPAREN_WRONG);
    }
    Var *cond = altexpr();
    ir.genWhileCond(cond, _exit);   // while 条件
    if (!match(Tag::RPAREN)) {
        recovery(F(Tag::LBRACE), SynError::RPAREN_LOST, SynError::RPAREN_WRONG);
    }

    if (F(Tag::LBRACE)) {
        block();
    }
    else {
        statement();
    }

    ir.genWhileTail(_while, _exit); // while 尾部
    symtab.leave();
}


/**
 * <dowhilestat> -> rsv_do <block> rsv_while lparen <altexpr> rparen semicon
 * <block> -> <block> | <statement>
*/
void Parser::dowhilestat() {
    symtab.enter();
    InterInst *_do, *_exit;         // 标签
    ir.genDoWhileHead(_do, _exit);  // do-while 头部

    match(Tag::KW_DO);
    if (F(Tag::LBRACE)) {
        block();
    }
    else {
        statement();
    }

    if (!match(Tag::KW_WHILE)) {
        recovery(F(Tag::LPAREN), SynError::WHILE_LOST, SynError::WHILE_WRONG);
    }
    if (!match(Tag::LPAREN)) {
        recovery(EXPR_FIRST || F(Tag::RPAREN), SynError::LPAREN_LOST, SynError::LPAREN_WRONG);
    }

    symtab.leave();
    Var *cond = altexpr();
    if (!match(Tag::RPAREN)) {
        recovery(F(Tag::SEMICON), SynError::RPAREN_LOST, SynError::RPAREN_WRONG);
    }

    if (!match(Tag::SEMICON)) {
        recovery(TYPE_FIRST || STATEMENT_FIRST || F(Tag::RBRACE), SynError::SEMICON_LOST, SynError::SEMICON_WRONG);
    }
    ir.genDoWhileTail(cond, _do, _exit);    // do-while 尾部
}

/**
 * <forstat> -> rsv_for lparen <forinit> semicon <altexpr> semicon <altexpr> rparen <block>
 * <block> -> <block> | <statement>
*/
void Parser::forstat() {
    symtab.enter();
    InterInst *_for, *_exit, *_step, *_block;   // 标签

    match(Tag::KW_FOR);
    if (!match(Tag::LPAREN)) {
        recovery(TYPE_FIRST || EXPR_FIRST || F(Tag::SEMICON), SynError::LPAREN_LOST, SynError::LPAREN_WRONG);
    }

    forinit();                  // 初始语句
    ir.genForHead(_for, _exit); // for 循环头部
    Var *cond = altexpr();      // 循环条件
    ir.genForCondBegin(cond, _step, _block, _exit); // for 循环条件开始部分

    if (!match(Tag::SEMICON)) {
        recovery(EXPR_FIRST, SynError::SEMICON_LOST, SynError::SEMICON_WRONG);
    }
    altexpr();

    if (!match(Tag::RPAREN)) {
        recovery(F(Tag::LBRACE), SynError::RPAREN_LOST, SynError::RPAREN_WRONG);
    }
    ir.genForCondEnd(_for, _block); // for 循环条件结束部分
    if (F(Tag::LBRACE)) {
        block();
    }
    else {
        statement();
    }

    ir.genForTail(_step, _exit);    // for 循环尾部
    symtab.leave();
}

/**
 * <forinit> -> <localdef> | <altexpr>
*/
void Parser::forinit() {
    if (TYPE_FIRST) {
        localdef();
    }
    else {
        altexpr();
        if (!match(Tag::SEMICON)) {
            recovery(EXPR_FIRST, SynError::SEMICON_LOST, SynError::SEMICON_WRONG);
        }
    }
}

/**
 * <ifstat> -> rsv_if lparen <expr> rparen <block> <elsestat>
 */
void Parser::ifstat() {
    symtab.enter();
    InterInst *_else, *_exit;

    match(Tag::KW_IF);
    if (!match(Tag::LPAREN)) {
        recovery(EXPR_FIRST, SynError::LPAREN_LOST, SynError::LPAREN_WRONG);
    }

    Var *cond = expr();
    ir.genIfHead(cond, _else);

    if (!match(Tag::RPAREN)) {
        recovery(F(Tag::LBRACE), SynError::RPAREN_LOST, SynError::RPAREN_WRONG);
    }

    if (F(Tag::LBRACE)) {
        block();
    }
    else {
        statement();
    }
    symtab.leave();

    ir.genElseHead(_else, _exit);
    if (F(Tag::KW_ELSE)) {
        elsestat();
    }
    ir.genElseTail(_exit);
}

/**
 * <elsestat> -> rsv_else <block> | ^
 */
void Parser::elsestat() {
    if (match(Tag::KW_ELSE)) {
        symtab.enter();

        if (F(Tag::LBRACE)) {
            block();
        }
        else {
            statement();
        }

        symtab.leave();
    }
}

/**
 * <switchstat> -> rsv_switch lparen <expr> rparen lbrac <casestat> rbrac
 */
void Parser::switchstat() {
    symtab.enter();

    InterInst *_exit;
    ir.genSwitchHead(_exit);

    match(Tag::KW_SWITCH);
    if (!match(Tag::LPAREN)) {
        recovery(EXPR_FIRST, SynError::LPAREN_LOST, SynError::LPAREN_WRONG);
    }

    Var *cond = expr();
    if (cond->isRef()) {    // switch(*p), switch(a[0])
        cond = ir.genAssign(cond);
    }
    if (!match(Tag::RPAREN)) {
        recovery(F(Tag::LBRACE), SynError::RPAREN_LOST, SynError::RPAREN_WRONG);
    }
    if (!match(Tag::LBRACE)) {
        recovery(F(Tag::KW_CASE) _(Tag::KW_DEFAULT), SynError::LBRACE_LOST, SynError::LBRACE_WRONG);
    }
    casestat(cond);
    if (!match(Tag::RBRACE)) {
        recovery(TYPE_FIRST || STATEMENT_FIRST, SynError::RBRACE_LOST, SynError::RBRACE_WRONG);
    }
    ir.genSwitchTail(_exit);

    symtab.leave();
}

/**
 * <casestat> -> rsv_case <caselabel> colon <subprogram> <casestat>
 *             | rsv_default colon <subprogram>
 */
void Parser::casestat(Var *cond) {
    if (match(Tag::KW_CASE)) {
        InterInst *_case_exit;
        Var *lb = caselabel();
        ir.genCaseHead(cond, lb, _case_exit);

        if (!match(Tag::COLON)) {
            recovery(TYPE_FIRST || STATEMENT_FIRST, SynError::COLON_LOST, SynError::COLON_WRONG);
        }

        symtab.enter();
        subprogram();
        symtab.leave();

        ir.genCaseTail(_case_exit);
        casestat(cond);
    }
    else if (match(Tag::KW_DEFAULT)) {
        if (!match(Tag::COLON)) {
            recovery(TYPE_FIRST || STATEMENT_FIRST, SynError::COLON_LOST, SynError::COLON_WRONG);
        }

        symtab.enter();
        subprogram();
        symtab.leave();
    }
}

/**
 * <caselabel> -> <literal>
 */
Var *Parser::caselabel() {
    return literal();
}

/**
 * <altexpr> -> <expr> | ^
 */
Var *Parser::altexpr() {
    if (EXPR_FIRST) {
        return expr();
    }
    return Var::getVoid();  // 返回特殊 VOID 变量
}

/**
 * <expr> -> <assexpr>
 */
Var *Parser::expr() {
    return assexpr();
}

/**
 * <assexpr> -> <orexpr> <asstail>
 */
Var *Parser::assexpr() {
    Var *lval = orexpr();
    return asstail(lval);
}

/**
 * <asstail> -> assign <assexpr> | ^
 */
Var *Parser::asstail(Var *lval) {
    if (match(Tag::ASSIGN)) {
        Var *rval = assexpr();
        Var *result = ir.genTwoOp(lval, Tag::ASSIGN, rval);
        return asstail(result);
    }
    return lval;
}

/**
 * <orexpr> -> <andexpr> <ortail>
 */
Var *Parser::orexpr() {
    Var *lval = andexpr();
    return ortail(lval);
}

/**
 * <ortail> -> or <andexpr> <ortail> | ^
 */
Var *Parser::ortail(Var *lval) {
    if (match(Tag::OR)) {
        Var *rval = andexpr();
        Var *result = ir.genTwoOp(lval, Tag::OR, rval);
        return ortail(result);
    }
    return lval;
}

/**
 * <andexpr> -> <cmpexpr> <andtail>
 */
Var *Parser::andexpr() {
    Var *lval = cmpexpr();
    return andtail(lval);
}

/**
 * <andtail> -> and <cmpexpr> <andtail> | ^
 */
Var *Parser::andtail(Var *lval) {
    if (match(Tag::AND)) {
        Var *rval = cmpexpr();
        Var *result = ir.genTwoOp(lval, Tag::AND, rval);
        return andtail(result);
    }
    return lval;
}

/**
 * <cmpexpr> -> <aloexpr> <cmptail>
 */
Var *Parser::cmpexpr() {
    Var *lval = aloexpr();
    return cmptail(lval);
}

/**
 * <cmptail> -> <cmps> <aloexpr> <cmptail> | ^
 */
Var *Parser::cmptail(Var *lval) {
    if (F(Tag::GT) _(Tag::GE) _(Tag::LT) _(Tag::LE) _(Tag::EQU) _(Tag::NEQU)) {
        Tag opt = cmps();
        Var *rval = aloexpr();
        Var *result = ir.genTwoOp(lval, opt, rval);
        return cmptail(result);
    }
    return lval;
}

/**
 * <cmps> -> gt | ge | ls | le | equ | nequ
 */
Tag Parser::cmps() {
    Tag opt = look->tag;
    move();
    return opt;
}

/**
 * <aloexpr> -> <item> <alotail>
 */
Var *Parser::aloexpr() {
    Var *lval = item();
    return alotail(lval);
}

/**
 * <alotail> -> <adds> <item> <alotails> | ^
 */
Var *Parser::alotail(Var *lval) {
    if (F(Tag::ADD) _(Tag::SUB)) {
        Tag opt = adds();
        Var *rval = item();
        Var *result = ir.genTwoOp(lval, opt, rval);
        return alotail(result);
    }
    return lval;
}

/**
 * <adds> -> add | sub
 */
Tag Parser::adds() {
    Tag opt = look->tag;
    move();
    return opt;
}

/**
 * <item> -> <factor> <itemtail>
 */
Var *Parser::item() {
    Var *lval = factor();
    return itemtail(lval);
}

/**
 * <itemtail> -> <muls> <factor> <itemtail> | ^
 */
Var *Parser::itemtail(Var *lval) {
    if (F(Tag::MUL) _(Tag::DIV)_(Tag::MOD)) {
        Tag opt = muls();
        Var *rval = factor();
        Var *result = ir.genTwoOp(lval, opt, rval);
        return itemtail(result);
    }
    return lval;
}

/**
 * <muls> -> mul | div | mod
 */
Tag Parser::muls() {
    assert(F(Tag::MUL) _(Tag::DIV) _(Tag::MOD));
    Tag opt = look->tag;
    move();
    return opt;
}

/**
 * <factor> -> <lops> <factor> | <val>
 */
Var *Parser::factor() {
    if (F(Tag::NOT)_(Tag::SUB)_(Tag::LEA)_(Tag::MUL)_(Tag::INC)_(Tag::DEC)) {
        Tag opt = lop();
        Var *v = factor();
        return ir.genOneOpLeft(opt, v);
    }
    return val();
}

/**
 * <lop> -> not | sub | lea | mul | incr | decr
 */
Tag Parser::lop() {
    assert(F(Tag::NOT)_(Tag::SUB) _(Tag::LEA) _(Tag::MUL) _(Tag::INC) _(Tag::DEC));
    Tag opt = look->tag;
    move();
    return opt;
}

/**
 * <val> -> <elem> <rop>
 */
Var *Parser::val() {
    Var *v = elem();
    if (F(Tag::INC)_(Tag::DEC)) {
        Tag opt = rop();
        v = ir.genOneOpRight(v, opt);
    }
    return v;
}

/**
 * <rop> -> incr | desc | ^
 */
Tag Parser::rop() {
    assert(F(Tag::INC) _(Tag::DEC));
    Tag opt = look->tag;
    move();
    return opt;
}

/**
 * <elem> -> ident <idexpr> | lparen <expr> rparen | <literal>
 */
Var *Parser::elem() {
    Var *v = nullptr;

    if (F(Tag::ID)) {
        string name = static_cast<Id *>(look.get())->name;
        move();
        v = idexpr(name);
    }
    else if (match(Tag::LPAREN)) {
        v = expr();
        if (!match(Tag::RPAREN)) {
            recovery(LVAL_OPR, SynError::RPAREN_LOST, SynError::RPAREN_WRONG);
        }
    }
    else {
        v = literal();
    }

    return v;
}

/**
 * <literal> -> number | string | char
 */
Var *Parser::literal() {
    Var *v = nullptr;

    if (F(Tag::NUM)_(Tag::STR)_(Tag::CH)) {
        v = new Var(look.get());
        if (F(Tag::STR)) {
            symtab.addStr(v);
        }
        else {
            symtab.addVar(v);
        }
        move();
    }
    else {
        recovery(RVAL_OPR, SynError::LITERAL_LOST, SynError::LITERAL_WRONG);
    }

    return v;
}

/**
 * <idexpr> -> lbrack <expr> rbrack
 *           | lparen <realarg> rparen
 *           | ^
 */
Var *Parser::idexpr(const string &name) {
    Var *v = nullptr;

    if (match(Tag::LBRACK)) {
        Var *index = expr();
        if (!match(Tag::RBRACK)) {
            recovery(LVAL_OPR, SynError::LBRACK_LOST, SynError::LBRACK_WRONG);
        }
        Var *array = symtab.getVar(name);
        v = ir.genArray(array, index);
    }
    else if (match(Tag::LPAREN)) {
        vector<Var *> args;
        realarg(args);
        if (!match(Tag::RPAREN)) {
            recovery(RVAL_OPR, SynError::RPAREN_LOST, SynError::RPAREN_WRONG);
        }
        Fun *function = symtab.getFun(name, args);
        v = ir.genCall(function, args);
    }
    else {
        v = symtab.getVar(name);
    }

    return v;
}

/**
 * <realarg> -> <arg> <arglist> | ^
 */
void Parser::realarg(vector<Var *> &args) {
    if (EXPR_FIRST) {
        args.emplace_back(arg());
        arglist(args);
    }
}

/**
 * <arglist> -> comma <arg> <arglist> | ^
 */
void Parser::arglist(vector<Var *> &args) {
    if (match(Tag::COMMA)) {
        args.emplace_back(arg());
        arglist(args);
    }
}

/**
 * <arg> -> <expr>
 */
Var *Parser::arg() {
    return expr();
}
