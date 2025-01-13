#ifndef __ASSEMBLER_ASS_H__
#define __ASSEMBLER_ASS_H__

#include <cstdio>
#include <iostream>

using namespace std;


enum class Symbol : int
{
    Null, IDENT, EXCEP, NUMBER, STRINGS,    // 空, 标识符, 异常字符, 数字, 串
    ADDI, SUBS, // 加, 减
    COMMA, LBRAC, RBRAC, COLON,
    BR_AL, BR_CL, BR_DL, BR_BL, BR_AH, BR_CH, BR_DH, BR_BH,
    WR_AX, WR_CX, WR_DX, WR_BX, WR_SP, WR_BP, WR_SI, WR_DI,
    DR_EAX, DR_ECX, DR_EDX, DR_EBX, DR_ESP, DR_EBP, DR_ESI, DR_EDI,
    I_MOV, I_CMP, I_SUB, I_ADD, I_AND, I_OR, I_LEA, // 2P
    I_CALL, I_INT,  // 1P
    I_IMUL, I_IDIV,
    I_NEG, I_INC, I_DEC,
    I_JMP, I_JE, I_JNE, I_JG, I_JL, I_JGE, I_JLE, I_JNA,
    I_SETE, I_SETNE, I_SETG, I_SETGE, I_SETL, I_SETLE,
    I_PUSH, I_POP,
    I_RET,          // 0P
    A_SEC, A_GLB, A_EQU, A_TIMES, A_DB, A_DW, A_DD,  // assamble instructions
};


enum class OP_TYPE : int
{
    IMMD = 1,
    REGS,
    MEMR,
};


#define idLen 30      // 标识符的最大长度 30
#define numLen 9      // 数字的最大位数 9
#define stringLen 255 // 字符串的最大长度 255


#define cast_int(x) static_cast<int>(x)

#define GET_CHAR         \
    if (-1 == getChar()) \
    {                    \
        return -1;       \
    }

extern FILE *fin;       // 全局变量, 文件输入指针
extern FILE *fout;      // 全局变量, 文件输出指针
extern enum Symbol sym; // 当前符号, getSym()->给语法分析使用
extern string finName;  // 文件名
extern char reservedTable[][idLen];
extern char str[];      // 记录当前 string, 给 erorr 处理
extern char id[];       // 记录当前 ident
extern int num;         // 记录当前 num
extern bool showAss;    // 显示汇编信息

class Table;
struct lb_record;

extern Table table;
extern string curSeg;       // 当前段名称
extern int dataLen;         // 有效数据长度

extern int inLen;           // 已经输出的指令长度, 调试用
extern lb_record *relLb;    // 记录指令中可能需要重定位的标签
struct ModRM;
extern ModRM modrm;
struct SIB;
extern SIB sib;
struct Inst;
extern Inst instr;

extern char ch;
extern char oldCh;
extern int chAtLine;
extern int lineLen;

extern int lineNum; // 记录行数
extern int err;     // 记录错误

extern enum Symbol token;   // 记录有效的符号
extern int scanLop;

int getSym();
void program();
void assemble(const string &filename);
void writeBytes(int value, int len);    // 按照小端顺序输出任意不大于 4 字节长度数据
void gen2op(Symbol opt, int des_t, int src_t, int len);
void gen1op(Symbol opt, int opr_t, int len);
void gen0op(Symbol opt);

#endif
