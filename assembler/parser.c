#include "elf.h"
#include "ass.h"
#include "semantic.h"
#include "elf_file.h"


void lbtail(const string &lbName);
void inst();
void basetail(const string &lbName, int times);
int len();
void values(const string &lbName, int times, int len);
void type(list<int> &cont, int len);
void valtail(list<int> &cont, int len);
void opr(int &regNum, int &type, int &len);
int reg();
void mem();
void addr();
void regaddr(Symbol basereg, const int type);
void regaddrtail(Symbol basereg, const int type, Symbol sign);
void off();


// 指导 nextToken 是否继续取符号
int wait = 0;
#define BACK (wait=1);

enum Symbol token;
string curSeg;      // 当前段名称
int dataLen = 0;    // 有效数据长度

int scanLop = 1;    // 记录扫描的次数, 第一遍扫描计算所有符号的地址或者值, 第二编扫描, 生成指令的二进制内容
lb_record *relLb = nullptr; // 记录指令中可能需要重定位的标签 (使用了符号)


int nextToken() {
    if (wait == 1) {
        wait = 0;   // 还原
        return 0;
    }
    while (1) {
        int flag = getSym();
        if (sym == Symbol::Null || sym == Symbol::EXCEP) {  // 无效符号掠过
            if (flag == -1) {
                token = Symbol::Null;
                if (scanLop == 1) {     // 准备第二编扫描
                    table.switchSeg();  // 第一次扫描最后记录以下最后一个段信息
                    fclose(fin);
                    fin = fopen((finName + ".s").c_str(), "r"); // 输入文件
                    oldCh = ch;
                    ch = ' ';
                    lineLen = 0;
                    chAtLine = 0;
                    lineNum = 0;
                    scanLop++;
                    continue;
                }
                return -1;
            }
        }
        else {
            token = sym;
            return 0;
        }
    }
}


void match(Symbol s) {
    if (nextToken() == 0) {
        if (token != s) {
            printf("语法符号匹配出错 [line: %d]\n", lineNum);
        }
    }
}


/**
 * <program> ->  ident <lbtail> <program>
 *            |  A_SEC ident <program>
 *            |  A_GLB ident <program>
 *            |  <inst> <program>
 */
void program() {
    if (nextToken() == -1 || token == Symbol::Null) {
        table.exportSyms();
        obj.printAll();
        return;
    }
    string lbName;
    switch (token) {
    case Symbol::IDENT:
        lbName += id;
        lbtail(lbName);
        break;
    case Symbol::A_SEC:
        match(Symbol::IDENT);
        table.switchSeg();
        break;
    case Symbol::A_GLB:
        match(Symbol::IDENT);

        if (scanLop == 2) {
            string name = id;
            lb_record *lr = table.getlb(name);
            lr->global = true;
        }
        break;
    default:
        BACK;
        inst();
    }
    program();
}


/**
 * <lbtail> -> A_TIMES NUMBER <basetail>
 *          |  A_EQU NUMBER
 *          |  COLON
 *          |  <basetail>
 */
void lbtail(const string &lbName) {
    nextToken();
    switch(token) {
    case Symbol::A_TIMES:
        match(Symbol::NUMBER);
        basetail(lbName, num);
        break;
    case Symbol::A_EQU:
        match(Symbol::NUMBER);
        {
            lb_record *lr = new lb_record(lbName, num);
            table.addlb(lr);
        }
        break;
    case Symbol::COLON:
        {
            lb_record *lr = new lb_record(lbName, false);
            table.addlb(lr);
        }
        break;
    default:
        BACK;
        basetail(lbName, 1);
    }
}


/**
 * <basetail> -> <len> <values>
 */
void basetail(const string &lbName, int times) {
    int l = len();
    values(lbName, times, l);
}


/**
 * <len> -> A_DB | A_DW | A_DD
 */
int len() {
    nextToken();
    switch (token) {
    case Symbol::A_DB:
        return 1;
        break;
    case Symbol::A_DW:
        return 2;
        break;
    case Symbol::A_DD:
        return 4;
        break;
    default:
        printf("len err! [line: %d]\n", lineNum);
        return 0;
    }
}


/**
 * <values> -> <type> <valtail>
 */
void values(const string &lbName, int times, int len) {
    list<int> cont;
    type(cont, len);
    valtail(cont, len);
    lb_record *lb = new lb_record(lbName, times, len, cont);
    table.addlb(lb);
}


/**
 * <type> -> NUMBER | STRINGS | IDENT | <off> NUMBER
 */
void type(list<int> &cont, int len) {
    nextToken();
    switch(token) {
    case Symbol::NUMBER:
        cont.emplace_back(num);
        break;
    case Symbol::STRINGS:
        for (int i = 0;; i++) {
            if (str[i] != 0) {
                cont.emplace_back(static_cast<uint8_t>(str[i]));
            }
            else {
                break;
            }
        }
        break;
    case Symbol::IDENT:
        {
            int cont_len = cont.size();
            string name = id;
            lb_record *lr = table.getlb(name);
            // 把地址作为占位, 第二次扫描还会刷新, 为该位置生成重定位项 (equ除外)
            cont.emplace_back(lr->addr);
            // 第二次扫描记录重定位项, 处理数据段重定位项
            if (scanLop == 2 && !lr->isEqu) {
                obj.addRel(curSeg, lb_record::curAddr + cont_len * len, name, R_386_32);
            }
        }
        break;
    default:
        printf("type err! [line: %d]\n", lineNum);
        break;
    }
}


/**
 * <valtail> -> COMMA <type> <valtail> | ^
 */
void valtail(list<int> &cont, int len) {
    nextToken();
    switch(token) {
    case Symbol::COMMA:
        type(cont, len);
        valtail(cont, len);
        break;
    default:
        BACK;
        return;
    }
}


SIB sib;
Inst instr;
ModRM modrm;

/**
 * <inst> -> I_MOV...I_LEA <opr> COMMA <opr> | I_CALL...I_POP <opr> | I_RET
 */
void inst() {
    instr.init();
    int i_len = 0;
    nextToken();
    if (token >= Symbol::I_MOV && token <= Symbol::I_LEA) {
        Symbol t = token;
        int d_type = 0, s_type = 0;
        int regNum = 0;
        opr(regNum, d_type, i_len);
        match(Symbol::COMMA);
        opr(regNum, s_type, i_len);
        gen2op(t, d_type, s_type, i_len);
    }
    else if (token >= Symbol::I_CALL && token <= Symbol::I_POP) {
        Symbol t = token;
        int r_type = 0, regNum = 0;
        opr(regNum, r_type, i_len);
        gen1op(t, r_type, i_len);
    }
    else if (token == Symbol::I_RET) {
        Symbol t = token;
        gen0op(t);
    }
    else {
        printf("opcode err [line: %d]\n", lineNum);
    }
}


/**
 * <opr> -> NUMBER | <reg> | <mem> | IDENT
 */
void opr(int &regNum, int &type, int &len) {
    string name;
    lb_record *lr;
    nextToken();

    switch(token) {
    case Symbol::NUMBER:
        type = cast_int(OP_TYPE::IMMD);
        instr.imm32 = num;
        break;
    case Symbol::IDENT:
        type = cast_int(OP_TYPE::IMMD);
        name += id;
        lr = table.getlb(name);
        instr.imm32 = lr->addr;
        if (scanLop == 2 && !lr->isEqu) {
            relLb = lr;
        }
        break;
    case Symbol::LBRAC:
        type = cast_int(OP_TYPE::MEMR);
        BACK;
        mem();
        break;
    case Symbol::SUBS: // 负立即数
        type = cast_int(OP_TYPE::IMMD);
        match(Symbol::NUMBER);
        instr.imm32 = -num;
        break;
    default:
        type = cast_int(OP_TYPE::REGS);
        BACK;
        len = reg();
        if (regNum != 0) {  // 双 reg, 将原来 reg 写入 rm 作为目的操作数, 本次写入 reg
            modrm.mod = 3;  // 双寄存器模式
            modrm.rm = modrm.reg;   // 因为统一采用 opcode rm,r 的指令格式, 比如 mov rm32,r32 就使用 0x89, 若是使用 opcode r,rm 形式则不需要
        }
        modrm.reg = (cast_int(token) - cast_int(Symbol::BR_AL)) % 8;
        regNum++;
    }
}


/**
 * <reg> -> BR_AL...BR_CH | DR_EAX...DR_EDI
 */
int reg() {
    nextToken();
    if (token >= Symbol::BR_AL && token <= Symbol::BR_BH) {
        return 1;
    }
    else if (token >= Symbol::WR_AX && token <= Symbol::WR_DI) {
        return 2;
    }
    else if (token >= Symbol::DR_EAX && token <= Symbol::DR_EDI) {
        return 4;
    }
    else {
        printf("reg err! [line: %d]\n", lineNum);
        return 0;
    }
}


/**
 * <mem> -> LBARC <addr> RBRAC
 */
void mem() {
    match(Symbol::LBRAC);
    addr();
    match(Symbol::RBRAC);
}


/**
 * <addr> -> NUMBER | IDENT | <reg> <regaddr>
 */
void addr() {
    string name;
    lb_record *lr;
    nextToken();
    switch (token) {
    case Symbol::NUMBER:    // 直接寻址 00 xxx 101 disp32
        modrm.mod = 0;
        modrm.rm = 5;
        instr.setDisp(num, 4);
        break;
    case Symbol::IDENT:     // 直接寻址
        modrm.mod = 0;
        modrm.rm = 5;
        name += id;
        lr = table.getlb(name);
        instr.setDisp(lr->addr, 4);
        if (scanLop == 2 && !lr->isEqu) {
            relLb = lr;     // 记录符号
        }
        break;
    default:    // 寄存器寻址
        BACK;
        int r_type = reg();
        regaddr(token, r_type);
    }
}


/**
 * <regaddr> -> <off> <regaddrtail> | ^
 */
void regaddr(Symbol baseReg, const int type) {
    nextToken();
    if (token == Symbol::ADDI || token == Symbol::SUBS) {
        BACK;
        off();
        regaddrtail(baseReg, type, token);
    }
    else {
        if (baseReg == Symbol::DR_ESP) {
            modrm.mod = 0;
            modrm.rm = 4;   // 引导 SIB
            sib.scale = 0;
            sib.index = 4;
            sib.base = 4;
        }
        else if (baseReg == Symbol::DR_EBP) {
            modrm.mod = 1;
            modrm.rm = 5;
            instr.setDisp(0, 1);
        }
        else {
            modrm.mod = 0;
            modrm.rm = cast_int(baseReg) - cast_int(Symbol::BR_AL) - (1 - type % 4) * 8;
        }
        BACK;
        return;
    }
}


/**
 * <off> -> ADDI | SUBS
 */
void off() {
    nextToken();
    if (!(token == Symbol::ADDI || token == Symbol::SUBS)) {
        printf("addr err![line:%d]\n", lineNum);
    }
}


/**
 * <regaddrtail> -> NUMBER | <reg>
 */
void regaddrtail(Symbol baseReg, const int type, Symbol sign) {
    nextToken();
    switch (token) {
    case Symbol::NUMBER: // 寄存器基址寻址 01/10 xxx rrr disp8/disp32
        if (sign == Symbol::SUBS)
            num = -num;
        if (num >= -128 && num < 128) { // disp8
            modrm.mod = 1;
            instr.setDisp(num, 1);
        }
        else {
            modrm.mod = 2;
            instr.setDisp(num, 4);
        }
        modrm.rm = cast_int(baseReg) - cast_int(Symbol::BR_AL) - (1 - type % 4) * 8;

        if (baseReg == Symbol::DR_ESP) {    // sib
            modrm.rm = 4;   // 引导 SIB
            sib.scale = 0;
            sib.index = 4;
            sib.base = 4;
        }
        break;
    default:
        BACK;
        int typei = reg();
        modrm.mod = 0;
        modrm.rm = 4;
        sib.scale = 0;
        sib.index = cast_int(token) - cast_int(Symbol::BR_AL) - (1 - typei % 4) * 8;
        sib.base = cast_int(baseReg) - cast_int(Symbol::BR_AL) - (1 - type % 4) * 8;
    }
}

/**
 *  <program>   ->  IDENT <lbtail> <program>
 *              |   <inst> <program>
 *              |   A_SEC IDENT <program>
 *              |   A_GLB IDENT <program>
 * 
 *  <lbtail>    ->  A_TIMES NUMBER <basetail>
 *              |   <basetail>
 *              |   A_EQU NUMBER
 *              |   COLON
 *  <basetail>  ->  <len> <values>
 *  <len>       ->  A_DB | A_DW | A_DD
 *  <values>    ->  <type> <valtail>
 *  <type>      ->  NUMBER | STRINGS | IDENT | <off> NUMBER
 *  <valtail>   ->  COMMA <type> <valtail> | ^
 * 
 *  <inst>      ->  I_MOV...I_LEA <opr> COMMA <opr> | I_CALL...I_POP <opr> | I_RET
 *  <opr>       ->  NUMBER | <reg> | <mem> | IDENT
 *  <reg>       ->  BR_AL...BR_CH | DR_EAX...DR_EDI
 *  <mem>       ->  LBARC <addr> RBRAC
 *  <addr>      ->  NUMBER | IDENT | <reg> <regaddr>
 *  <regaddr>   ->  <off> <regaddrtail> | ^
 *  <off>       ->  ADDI | SUBS
 *  <regaddrtail>   ->  NUMBER | <reg>
 */
