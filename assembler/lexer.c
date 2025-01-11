#include "ass.h"
#include <string.h>

void checkReserved();


#define maxLen 1024 // 文件行的最大长度
char ch = ' ';      // 当前字符
char oldCh = ' ';   // 上一个字符
int lineNum = 0;    // 行号
char buffer[maxLen];
int chAtLine = 0;   // 当前字符列位置
int lineLen = 0;    // 当前行的长度


int getChar() {
    static int len = 0;
    static int readPos = 0;

    if (readPos >= len) {
        readPos = 0;
        len = fread(buffer, 1, maxLen, fin);
        if (len <= 0) {
            ch = 0;
            return -1;
        }
    }

    // 正常读取
    oldCh = ch;
    ch = buffer[readPos];
    readPos++;

    lineLen++;
    chAtLine++;

    if (ch == '\n') {
        lineNum++;
        lineLen = chAtLine = 0;
    }

    if (ch == 0) {
        return -1;
    }
    else {
        return 0;
    }
}


int err = 0;                    // 记录错误
enum Symbol sym = Symbol::Null; // 当前符号
char id[idLen + 1];             // 存放标识符
int num = 0;                    // 存放的数字
char str[stringLen + 1];        // 存放字符串
char letter = 0;                // 存放字符


int getSym() {
    while (ch == ' ' || ch == 10 || ch == 9)  { // 忽略空格, 换行, TAB
        getChar();
    }

    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' || ch == '@' || ch == '.')  {
        int idCount = 0;    // 为标识符的长度计数
        int reallen = 0;    // 实际标识符长度
        int f;              // getChar 返回标记

        // 取出标识符
        do {
            reallen++;
            if (idCount < idLen) {  // 标识符过长部分掠去
                id[idCount] = ch;
                idCount++;
            }
            f = getChar();
        } while ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' || ch == '@' || ch == '.' || (ch >= '0' && ch <= '9'));

        id[idCount] = 0;        // 结尾
        if (reallen > idLen) {  // 标识符过长
            // lexerror(id2long,0);
        }
        checkReserved();
        return f;
    }
    else if (ch >= '0' && ch <= '9') {  // 数字, 默认正数
        sym = Symbol::NUMBER;
        int numCount = 0;       // 为数字的长度计数
        num = 0;                // 数值迭代器
        [[maybe_unused]] int f; // getChar 返回标记

        if (ch != '0') {                        // DEC
            do {
                if (numCount < numLen) {        // 数字过长部分掠去
                    num = 10 * num + ch - '0';
                    numCount++;
                }
                f = getChar();
            } while (ch >= '0' && ch <= '9');
            return f;
        }
        else {
            f = getChar();
            if (ch == 'x' || ch == 'X') {       // HEX
                f = getChar();
                if (f == -1) {
                    return -1;
                }
                if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f')) {
                    do {
                        num = num * 16 + ch;
                        if (ch >= '0' && ch <= '9') {
                            num -= '0';
                        }
                        else if (ch >= 'A' && ch <= 'F') {
                            num += 10 - 'A';
                        }
                        else if (ch >= 'a' && ch <= 'f') {
                            num += 10 - 'a';
                        }
                        f = getChar();
                    } while ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f'));
                }
                else {
                    printf("lex error: [line %d]\n", lineNum);
                }
            }
            else if (ch == 'b' || ch == 'B') {  // BIN
                f = getChar();
                if (ch >= '0' && ch <= '1') {
                    do {
                        num = num * 2 + ch - '0';
                        f = getChar();
                    } while (ch >= '0' && ch <= '1');
                }
                else {
                    printf("lex error: [line %d]\n", lineNum);
                }
            }
            else if (ch >= '0' && ch <= '7') {  // OCT
                do {
                    num = num * 8 + ch - '0';
                    f = getChar();
                } while (ch >= '0' && ch <= '7');
            }
        }
    }
    else {
        int strCount = 0;   // 为字符串的长度计数
        int f = 0;          // getChar 返回标记
        int reallen;        // 记录串的实际长度
        switch (ch) {
        case '+':
            sym = Symbol::ADDI;
            GET_CHAR
            break;
        case '-':
            sym = Symbol::SUBS;
            GET_CHAR
            break;
        case ':':
            sym = Symbol::COLON;
            GET_CHAR
            break;
        case ';':
            sym = Symbol::Null;
            GET_CHAR
            while (ch != '\n') {    // 只要行不结束
                GET_CHAR
            }
            GET_CHAR
            break;
        case ',':
            sym = Symbol::COMMA;
            GET_CHAR
            break;
        case '"':
            sym = Symbol::Null;
            // GET_CHAR
            f = getChar();
            if (f == -1) {  // 文件结束
                // lexerror(strwrong,0);
                return -1;
            }
            reallen = 0;
            while (ch != '"') {
                reallen++;
                if (strCount < stringLen) { // 字符串过长部分掠去
                    str[strCount] = ch;
                    strCount++;
                }
                // GET_CHAR
                f = getChar();
                if (f == -1) {  // 文件结束
                    // lexerror(strwrong,0);
                    return -1;
                }
            }
            str[strCount] = 0;          // 结尾
            if (reallen > stringLen) {  // string 太长
                // lexerror(str2long,0);
            }
            sym = Symbol::STRINGS;
            GET_CHAR
            break;
        case '[':
            sym = Symbol::LBRAC;
            GET_CHAR
            break;
        case ']':
            sym = Symbol::RBRAC;
            GET_CHAR
            break;
        case 0: // 不在这个位置达到文件末尾, 会暂时不处理, 等到下一次调用时候在这里返回
            sym = Symbol::Null;
            return -1;
            break;
        default:
            sym = Symbol::EXCEP;
            // lexerror(excpchar,ch);
            printf("不能解析的词法符号 [line: %d]\n", lineNum);
            err++;
            GET_CHAR
        }
    }
    return 0;
}


#define reservedNum 62


static std::unordered_map<std::string, Symbol> keywords = {
    {"al", Symbol::BR_AL}, {"cl", Symbol::BR_CL},
    {"dl", Symbol::BR_DL}, {"bl", Symbol::BR_BL},
    {"ah", Symbol::BR_AH}, {"ch", Symbol::BR_CH},
    {"dh", Symbol::BR_DH}, {"bh", Symbol::BR_BH},

    {"ax", Symbol::WR_AX}, {"cx", Symbol::WR_CX},
    {"dx", Symbol::WR_DX}, {"bx", Symbol::WR_BX},
    {"sp", Symbol::WR_SP}, {"bp", Symbol::WR_BP},
    {"si", Symbol::WR_SI}, {"di", Symbol::WR_DI},

    {"eax", Symbol::DR_EAX}, {"ecx", Symbol::DR_ECX},
    {"edx", Symbol::DR_EDX}, {"ebx", Symbol::DR_EBX},
    {"esp", Symbol::DR_ESP}, {"ebp", Symbol::DR_EBP},
    {"esi", Symbol::DR_ESI}, {"edi", Symbol::DR_EDI},

    {"mov", Symbol::I_MOV}, {"cmp", Symbol::I_CMP},
    {"sub", Symbol::I_SUB}, {"add", Symbol::I_ADD},
    {"and", Symbol::I_AND}, {"or", Symbol::I_OR},
    {"lea", Symbol::I_LEA},
    
    {"call", Symbol::I_CALL}, {"int", Symbol::I_INT},
    {"imul", Symbol::I_IMUL}, {"idiv", Symbol::I_IDIV},
    {"neg", Symbol::I_NEG},
    {"inc", Symbol::I_INC}, {"dec", Symbol::I_DEC},
    {"jmp", Symbol::I_JMP}, {"je", Symbol::I_JE},
    {"jne", Symbol::I_JNE}, {"jg", Symbol::I_JG},
    {"jl", Symbol::I_JL}, {"jge", Symbol::I_JGE},
    {"jle", Symbol::I_JLE}, {"jna", Symbol::I_JNA},
    {"sete", Symbol::I_SETE}, {"setne", Symbol::I_SETNE},
    {"setg", Symbol::I_SETG}, {"setge", Symbol::I_SETGE},
    {"setl", Symbol::I_SETL}, {"setle", Symbol::I_SETLE},
    {"push", Symbol::I_PUSH}, {"pop", Symbol::I_POP},

    {"ret", Symbol::I_RET},

    {"section", Symbol::A_SEC}, {"global", Symbol::A_GLB},
    {"equ", Symbol::A_EQU}, {"times", Symbol::A_TIMES},
    {"db", Symbol::A_DB}, {"dw", Symbol::A_DW}, {"dd", Symbol::A_DD},
};


void checkReserved() {
    if (keywords.count(id)) {
        sym = keywords[id];
        return;
    }
    sym = Symbol::IDENT; // 搜索失败, 是标识符
}
