#ifndef __LEXER_SCANNER_H__
#define __LEXER_SCANNER_H__

#include "common.h"


class Scanner
{
    FILE *file;
    std::string fileName;

    static constexpr int BUFLEN = 1024;
    char line[BUFLEN] = {0};

    int lineLen;    // 当前行的长度
    int readPos;    // 读取的位置
    char lastch;    // 上一个字符, 主要用于判断换行位置

    int lineNum;
    int colNum;

    void showChar(const char ch) const;

public:
    const Scanner &operator=(const Scanner &&rhs) = delete;

    explicit Scanner(const std::string &name);
    ~Scanner();

    int scan();

    std::string& getFile();
    int getLine() const;
    int getCol() const;
};


#endif
