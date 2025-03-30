#ifndef __COMPILER_COMPILER_H__
#define __COMPILER_COMPILER_H__

#include "common.h"


class Args
{
public:
    static bool showChar;
    static bool showToken;
    static bool showSym;
    static bool showIr;
    static bool showOr;
    static bool showBlock;
    static bool showHelp;
    static bool opt;
};


class Compiler
{
public:
    Compiler() = default;
    ~Compiler() = default;

    const Compiler &operator=(const Compiler &&rhs) = delete;

    void compile(const std::string &file);
};

#endif
