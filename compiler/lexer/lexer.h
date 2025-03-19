#ifndef __COMPILER_LEXER_H__
#define __COMPILER_LEXER_H__

#include <memory>
#include "lexer/token.h"
#include "lexer/keywords.h"


class Lexer
{
    static Keywords keywords;

    Scanner &scanner;
    std::unique_ptr<Token> token;
    char ch;

    bool scan(char need = 0);

public:
    explicit Lexer(Scanner &sc);
    ~Lexer();

    Lexer(const Lexer &rhs) = delete;
    Lexer &operator=(const Lexer &rhs) = delete;

    std::unique_ptr<Token> tokenize();
};

#endif
