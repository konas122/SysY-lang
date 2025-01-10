#include "ass.h"
#include <string.h>

void checkReserved();


#define maxLen 80   // 文件行的最大长度
char ch = ' ';      // 当前字符
char oldCh = ' ';   // 上一个字符
int lineNum = 0;    // 行号
char line[maxLen];
int chAtLine = 0;   // 当前字符列位置
int lineLen = 0;    // 当前行的长度

int getChar() {
    if (chAtLine >= lineLen) {  // 超出索引, 行读完,>=防止出现强制读取的bug
        chAtLine = 0;   // 字符, 行, 重新初始化
        lineLen = 0;
        lineNum++;      // 行号增加
        ch = ' ';
        while (ch != 10) {  // 检测行行结束
            if (fscanf(fin, "%c", &ch) == EOF) {
                line[lineLen] = 0; // 文件结束
                break;
            }
            line[lineLen] = ch; // 循环读取一行的字符
            lineLen++;
            if (lineLen == maxLen) {    // 单行程序过长
                // 不继续读就可以, 不用报错
                break;
            }
        }
    }
    // 正常读取
    oldCh = ch;
    ch = line[chAtLine];
    chAtLine++;
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
        int numCount = 0;   // 为数字的长度计数
        num = 0;            // 数值迭代器
        int reallen = 0;    // 实际数字长度
        int f;              // getChar 返回标记

        if (ch != '0') {                        // DEC
            do {
                reallen++;
                if (numCount < numLen) {    // 数字过长部分掠去
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


#define reservedNum 57
char reservedTable[reservedNum][idLen] =
{
    "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh",
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di",
    "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    "mov", "cmp", "sub", "add", "and", "or", "lea",         // 2p
    "call", "int", "imul", "idiv", "neg", "inc", "dec",     // 1p
    "jmp", "je", "jne", "sete", "setne", "setg", "setge",
    "setl", "setle", "push" ,"pop",
    "ret",                                                  // 0p
    "section", "global", "equ", "times", "db", "dw", "dd"
};


static Symbol reservedSymbol[reservedNum] =
{
    Symbol::BR_AL, Symbol::BR_CL, Symbol::BR_DL, Symbol::BR_BL, Symbol::BR_AH, Symbol::BR_CH, Symbol::BR_DH, Symbol::BR_BH,
    Symbol::WR_AX, Symbol::WR_CX, Symbol::WR_DX, Symbol::WR_BX, Symbol::WR_SP, Symbol::WR_BP, Symbol::WR_SI, Symbol::WR_DI,
    Symbol::DR_EAX, Symbol::DR_ECX, Symbol::DR_EDX, Symbol::DR_EBX, Symbol::DR_ESP, Symbol::DR_EBP, Symbol::DR_ESI, Symbol::DR_EDI,
    Symbol::I_MOV, Symbol::I_CMP, Symbol::I_SUB, Symbol::I_ADD, Symbol::I_AND, Symbol::I_OR, Symbol::I_LEA, // 2P
    Symbol::I_CALL, Symbol::I_INT, Symbol::I_IMUL, Symbol::I_IDIV,  // 1P
    Symbol::I_NEG, Symbol::I_INC, Symbol::I_DEC,
    Symbol::I_JMP, Symbol::I_JE, Symbol::I_JNE,
    Symbol::I_SETE, Symbol::I_SETNE, Symbol::I_SETG, Symbol::I_SETGE, Symbol::I_SETL, Symbol::I_SETLE,
    Symbol::I_PUSH, Symbol::I_POP,
    Symbol::I_RET,  // 0P
    Symbol::A_SEC, Symbol::A_GLB, Symbol::A_EQU, Symbol::A_TIMES, Symbol::A_DB, Symbol::A_DW, Symbol::A_DD
};


void checkReserved() {
    for (int k = 0; k < reservedNum; k++) {
        if (strcmp(id, reservedTable[k]) == 0) {
            sym = reservedSymbol[k];
            return;
        }
    }
    sym = Symbol::IDENT; // 搜索失败, 是标识符
}
