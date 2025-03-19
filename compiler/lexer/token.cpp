#include <sstream>

#include "token.h"

using namespace std;


const char *tokenName[] = {
    "error",
    "EOF",
    "IDENT",    // 标识符
    "int", "char", "void",
    "extern",
    "NUM", "CHAR", "STR",
    "!", "&",
    "+", "-", "*", "/", "%",
    "++", "--",
    ">", ">=", "<", "<=", "==", "!=",
    "&&", "||",
    "(", ")",
    "[", "]",
    "{", "}",
    ",", ":", ";",
    "=",
    "if", "else",
    "switch", "case", "default",
    "while", "do", "for",
    "break", "continue", "return"
};


Token::Token(Tag t) : tag(t) {}

string Token::toString() const {
    return tokenName[cast_int(tag)];
}

Token::~Token() {}


// =============================================================================

Id::Id(string_view n) : Token(Tag::ID), name(n) {}

string Id::toString() const {
    return Token::toString() + " " + name;
}


// =============================================================================

Str::Str(std::string_view  s) : Token(Tag::STR), str(s) {}

string Str::toString() const {
    return string("[") + Token::toString() + "]: " + str;
}


// =============================================================================

Num::Num(int v) : Token(Tag::NUM), val(v) {};

string Num::toString() const {
    stringstream ss;
    ss << val;
    return string("[") + Token::toString() + "]: " + ss.str();
}


// =============================================================================

Char::Char(char c) : Token(Tag::CH), ch(c) {}

string Char::toString() const {
    stringstream ss;
    ss << ch;
    return string("[") + Token::toString() + "]: " + ss.str();
}
