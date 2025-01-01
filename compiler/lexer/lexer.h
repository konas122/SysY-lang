#ifndef __COMPILER_LEXER_H__
#define __COMPILER_LEXER_H__

#include "lexer/token.h"
#include "lexer/keywords.h"


class Lexer
{
    static Keywords keywords;

    Scanner &scanner;
    Token *token;
    char ch;

    bool scan(char need = 0);

public:
    Lexer(Scanner &sc);
    ~Lexer();

    Lexer(const Lexer &rhs) = delete;
    Lexer &operator=(const Lexer &rhs) = delete;

    Token *tokenize();
};

#endif
