#include <cstdint>

#include "ass.h"
#include "elf.h"
#include "elf_file.h"
#include "semantic.h"

#define PREFIX  (0x66)


static uint8_t i_2opcode[] = {
//        8 位操作数        |      32 位操作数
//  r,r  r,rm|rm,r r,im    | r,r  r,rm|rm,r r,im
    0x88, 0x8a, 0x88, 0xb0, 0x89, 0x8b, 0x89, 0xb8, // mov
    0x38, 0x3a, 0x38, 0x80, 0x39, 0x3b, 0x39, 0x81, // cmp
    0x28, 0x2a, 0x28, 0x80, 0x29, 0x2b, 0x29, 0x81, // sub
    0x00, 0x02, 0x00, 0x80, 0x01, 0x03, 0x01, 0x81, // add
    0x22, 0x22, 0x20, 0x80, 0x23, 0x23, 0x21, 0x81, // and
    0x0a, 0x0a, 0x08, 0x80, 0x0b, 0x0b, 0x09, 0x81, // or
    0x00, 0x00, 0x00, 0x00, 0x00, 0x8d, 0x00, 0x00  // lea
};


static uint16_t i_1opcode[] = {
    0xe8, 0xcd, 0xf7, 0xf7, 0xf7, 0x40, 0x48, 0xe9, // call, int, imul, idiv, neg, inc, dec, jmp <rel32>
    0x84, 0x85, 0x8f, 0x8c, 0x8d, 0x8e, 0x86,       // je, jne, jg, jl, jge, jle, jna
    0x94, 0x95, 0x9f, 0x9d, 0x9c, 0x9e,             // sete, setne, setge, setl, setle
    0x50,       // push
    0x58        // pop
};


static uint8_t i_0opcode[] = {
    0xc3    // ret
};

int inLen = 0;


/**
 * 输出 ModRM 字节
 * mod(2) | reg(3) | rm(3)
 */
void writeModRM() {
    if (modrm.mod != -1) {
        uint8_t mrm = (uint8_t)(((modrm.mod & 0x00000003) << 6) + ((modrm.reg & 0x0000007) << 3) + (modrm.rm & 0x00000007));
        writeBytes(mrm, 1);
    }
}


/**
 * 输出 SIB 字节
 * scale(2) | index(3) | base(3)
 */
void writeSIB() {
    if (sib.scale != -1) {
        uint8_t _sib = (uint8_t)(((sib.scale & 0x00000003) << 6) + ((sib.index & 0x00000007) << 3) + (sib.base & 0x00000007));
        writeBytes(_sib, 1);
    }
}


/**
 * 按照小端顺序 (little endian) 输出指定长度数据
 * len=1: 输出第 4 字节
 * len=2: 输出第 3,4 字节
 * len=4: 输出第 1,2,3,4 字节
 */
void writeBytes(int value, int len) {
    lb_record::curAddr += len;
    if (scanLop == 2) {
        fwrite(&value, len, 1, fout);
        inLen += len;
    }
}


bool processRel(int type) {
    if (scanLop == 1 || relLb == nullptr) {
        relLb = nullptr;
        return false;
    }
    bool flag = false;
    if (type == R_386_32) {     // 绝对重定位
        if (!relLb->isEqu) {    // 只要是地址符号就必须重定位, 宏除外 (这里判断与否都可以, relLb 非 nullptr 都是非 equ 符号)
            obj.addRel(curSeg, lb_record::curAddr, relLb->lbName, type);
            flag = true;
        }
    }
    else if (type == R_386_PC32) {  // 相对重定位
        if (relLb->externed) {      // 对于跳转, 内部的不需要重定位, 外部的需要重定位
            obj.addRel(curSeg, lb_record::curAddr, relLb->lbName, type);
            flag = true;
        }
    }
    relLb = nullptr;
    return flag;
}


void gen2op(Symbol opt, int des_t, int src_t, int len) {
    if (len == 2) {
        writeBytes(PREFIX, 1);
    }

    int index = -1;
    if (src_t == cast_int(OP_TYPE::IMMD)) {
        index = 3;
    }
    else {
        index = (des_t - 2) * 2 + src_t - 2;
    }

    index = (cast_int(opt) - cast_int(Symbol::I_MOV)) * 8 + (1 - len % 2) * 4 + index;
    uint8_t opcode = i_2opcode[index];
    switch(modrm.mod) {
    case -1:    // reg32 <- imm32
        if (opt == Symbol::I_MOV) {
            opcode += (uint8_t)(modrm.reg);
            writeBytes(opcode, 1);
        }
        else {
            const int reg_codes[] = {7, 5, 0, 4, 1};
            modrm.mod = 3;
            modrm.rm = modrm.reg;
            int ret = cast_int(opt) - cast_int(Symbol::I_CMP);
            if (ret >= 0 && ret <= 4) {
                modrm.reg = reg_codes[ret];
            }
            else {
                printf("opcode err [line: %d]\n", lineNum);
                break;
            }
            writeBytes(opcode, 1);
            writeModRM();
        }

        // 可能的重定位位置 mov eax,@buffer, 也有可能是 mov eax,@buffer_len, 就不许要重定位, 因为是宏
        processRel(R_386_32);
        writeBytes(instr.imm32, len);   // 一定要按照长度输出立即数
        break;

    case 0:     // [reg],reg reg,[reg]
        writeBytes(opcode, 1);
        writeModRM();
        if (modrm.rm == 5) {        // [disp32]
            processRel(R_386_32);   // 可能是 mov eax,[@buffer], 后边 disp8 和 disp32 不会出现类似情况
            instr.writeDisp();      // 地址肯定是 4 字节长度
        }
        else if (modrm.rm == 4) {   // SIB
            writeSIB();
        }
        break;
    case 1:     // [reg+disp8],reg reg,[reg+disp8]
        writeBytes(opcode, 1);
        writeModRM();
        if (modrm.rm == 4) {
            writeSIB();
        }
        instr.writeDisp();
        break;
    case 2:     // [reg+disp32],reg reg,[reg+disp32]
        writeBytes(opcode, 1);
        writeModRM();
        if (modrm.rm == 4) {
            writeSIB();
        }
        instr.writeDisp();
        break;
    case 3:     // reg,reg
        writeBytes(opcode, 1);
        writeModRM();
        break;
    }
}

void gen1op(Symbol opt, int opr_t, int len) {
    uint8_t exchar;
    uint16_t opcode = i_1opcode[cast_int(opt) - cast_int(Symbol::I_CALL)];
    if (opt == Symbol::I_CALL || (opt >= Symbol::I_JMP && opt <= Symbol::I_JNA)) {
        if (opt != Symbol::I_CALL && opt != Symbol::I_JMP) {
            writeBytes(0x0f, 1);
        }
        writeBytes(opcode, 1);
        int addr = processRel(R_386_PC32) ? lb_record::curAddr : instr.imm32;
        int pc = lb_record::curAddr + 4;
        writeBytes(addr - pc, 4);
    }
    else if (opt >= Symbol::I_SETE && opt <= Symbol::I_SETLE) {
        modrm.mod = 3;
        modrm.rm = modrm.reg;
        modrm.reg = 0;

        writeBytes(0x0f, 1);
        writeBytes(opcode, 1);
        writeModRM();
    }
    else if (opt == Symbol::I_INT) {
        writeBytes(opcode, 1);
        writeBytes(instr.imm32, 1);
    }
    else if (opt == Symbol::I_PUSH) {
        if (opr_t == cast_int(OP_TYPE::IMMD)) {
            opcode = 0x68;
            writeBytes(opcode, 1);
            writeBytes(instr.imm32, 4);
        }
        else {
            if (len == 2) {
                writeBytes(PREFIX, 1);
            }
            opcode += (uint8_t)(modrm.reg);
            writeBytes(opcode, 1);
        }
    }
    else if (opt == Symbol::I_INC) {
        if (len == 1) {
            opcode = 0xfe;
            writeBytes(opcode, 1);
            exchar = 0xc0;
            exchar += (uint8_t)(modrm.reg);
            writeBytes(exchar, 1);
        }
        else {
            if (len == 2) {
                writeBytes(PREFIX, 1);
            }
            opcode += (uint8_t)(modrm.reg);
            writeBytes(opcode, 1);
        }
    }
    else if (opt == Symbol::I_DEC) {
        if (len == 1) {
            opcode = 0xfe;
            writeBytes(opcode, 1);
            exchar = 0xc8;
            exchar += (uint8_t)(modrm.reg);
            writeBytes(exchar, 1);
        }
        else {
            if (len == 2) {
                writeBytes(PREFIX, 1);
            }
            opcode += (uint8_t)(modrm.reg);
            writeBytes(opcode, 1);
        }
    }
    else if (opt == Symbol::I_NEG) {
        if (len == 1) {
            opcode = 0xf6;
        }
        if (len == 2) {
            writeBytes(PREFIX, 1);
        }
        exchar = 0xd8;
        exchar += (uint8_t)(modrm.reg);
        writeBytes(opcode, 1);
        writeBytes(exchar, 1);
    }
    else if (opt == Symbol::I_POP) {
        if (len == 2) {
            writeBytes(PREFIX, 1);
        }
        opcode += (uint8_t)(modrm.reg);
        writeBytes(opcode, 1);
    }
    else if (opt == Symbol::I_IMUL || opt == Symbol::I_IDIV) {
        if (len == 2) {
            writeBytes(PREFIX, 1);
        }
        writeBytes(opcode, 1);
        if (opt == Symbol::I_IMUL) {
            exchar = 0xe8;
        }
        else {
            exchar = 0xf8;
        }
        exchar += (uint8_t)(modrm.reg);
        writeBytes(exchar, 1);
    }
}


void gen0op([[maybe_unused]] Symbol opt) {
    uint8_t opcode = i_0opcode[0];
    writeBytes(opcode, 1);
}
