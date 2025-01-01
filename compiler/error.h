#ifndef __COMPILER_ERROR_H__
#define __COMPILER_ERROR_H__

#include "lexer/token.h"
#include "lexer/scanner.h"

#define FATAL   "<fatal>: "
#define ERROR   "<error>: "
#define WARN    "<warn>: "

#ifdef DEBUG
#define PDEBUG(fmt, args...) printf(fmt, ##args)
#else
#define PDEBUG(fmt, args...)
#endif


class Error
{
    static Scanner *scanner;

public:
    Error(Scanner *sc);

    static int errorNum;
    static int warnNum;

    static int getErrorNum();
    static int getWarnNum();

    static void lexError(int code);
    static void synError(int code, Token *t);
    static void semError(int code, std::string name = "");
    static void semWarn(int code, std::string name = "");
};

#endif
