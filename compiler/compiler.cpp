#include <memory>

#include "error.h"
#include "ir/genir.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "symbol/symtab.h"

#include "compiler.h"


bool Args::showChar = false;
bool Args::showToken = false;
bool Args::showSym = false;
bool Args::showIr = false;
bool Args::showOr = false;
bool Args::showBlock = false;
bool Args::showHelp = false;
bool Args::opt = false;


void Compiler::compile(const std::string &file) {
    // 准备
    Scanner scanner(file);                      // 扫描器
    Error error(&scanner);                      // 错误处理
    Lexer lexer(scanner);                       // 词法分析器
    auto symtab = std::make_shared<SymTab>();   // 符号表
    auto ir = GenIR::create(symtab);            // 中间代码生成器
    Parser parser(lexer, symtab, ir);           // 语法分析器

    // 分析
    parser.analyse();

    // 报错
    if (Error::getErrorNum() + Error::getWarnNum()) {
        return;
    }

    // 中间结果
    if (Args::showSym) {
        symtab->toString();
    }
    if (Args::showIr) {
        symtab->printInterCode();
    }

    // 优化
    symtab->optimize();
    if (Args::showOr) {
        symtab->printOptCode();
    }

    // 生成汇编代码
    symtab->genAsm(file);
}
