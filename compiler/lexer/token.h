#ifndef __LEXER_TOKEN_H__
#define __LEXER_TOKEN_H__

#include "common.h"

extern const char *tokenName[];

/*******************************************************************************
                                   词法记号
*******************************************************************************/

class Token
{
public:
    Tag tag;
    Token(Tag t);
    virtual std::string toString() const;
    virtual ~Token ();
};


/*******************************************************************************
                                   标识符
*******************************************************************************/

class Id: public Token
{
public:
    std::string name;
    Id(const std::string &n);
    virtual std::string toString() const override;
};


/*******************************************************************************
                                   字符串
*******************************************************************************/

class Str: public Token
{
public:
    std::string str;
    Str(const std::string &s);
    virtual std::string toString() const override;
};


/*******************************************************************************
                                   数字
*******************************************************************************/

class Num: public Token
{
public:
    int val;
    Num(int v);
    virtual std::string toString() const override;
};


/*******************************************************************************
                                   字符
*******************************************************************************/

class Char: public Token
{
public:
    char ch;
    Char(char c);
    virtual std::string toString() const override;
};

#endif
