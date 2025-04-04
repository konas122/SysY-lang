#ifndef __ASSEMBLER_SEMANTIC_H__
#define __ASSEMBLER_SEMANTIC_H__

#include "ass.h"

#include <list>
#include <memory>
#include <vector>
#include <cstdint>
#include <iostream>
#include <unordered_map>

using namespace std;


// 符号声明记录
struct lb_record
{
    static int curAddr; // 一个段内符号的偏移累加量
    string segName;     // 隶属于的段名, 三种: .text .data .bss
    string lbName;      // 符号名
    bool isEqu;         // 是否是 L equ 1
    bool global = false;
    bool externed;      // 是否是外部符号, 内容是 1 的时候表示为外部的, 此时 curAddr 不累加
    int addr;           // 符号段偏移
    int times;          // 定义重复次数
    int len;            // 符号类型长度: db-1 dw-2 dd-4
    list<int> cont;     // 符号内容数组

    lb_record(std::string_view n, int a);                      // L equ 1
    explicit lb_record(std::string_view n, bool ex = false);   // L: 或者创建外部符号 (ex=true: L dd @e_esp)
    lb_record(std::string_view n, int t, int l, const list<int> &c);   // L times 5 dw 1,"abc",L2 或者 L dd 23
    void write();                   // 输出符号内容
    ~lb_record() = default;
};


// =============================================================================

class Table
{
public:
    unordered_map<string, std::shared_ptr<lb_record>> lb_map;   // 符号声明列表
    vector<std::shared_ptr<lb_record>> defLbs;                  // 记录数据定义符号顺序
    int hasName(const string &name);
    void addlb(std::shared_ptr<lb_record> p_lb);    // 添加符号
    std::shared_ptr<lb_record> getlb(const string& name);           // 获取已经定义的符号
    void switchSeg();               // 切换下一个段, 由于一般只有 .text 和 .data, 因此可以此时创建段表项目
    void exportSyms();              // 导出所有的符号到 elf
    void write();
    ~Table();

    Table() = default;
    const Table &operator=(const Table &&rhs) = delete;
};


// =============================================================================

struct ModRM
{
    int mod;    // 0-1
    int reg;    // 2-4
    int rm;     // 5-7
    ModRM();
    void init();

    const ModRM &operator=(const ModRM &&rhs) = delete;
};


// =============================================================================

struct SIB
{
    int scale;  // 0-1
    int index;  // 2-4
    int base;   // 5-7
    SIB();
    void init();

    const SIB &operator=(const SIB &&rhs) = delete;
};


// =============================================================================

struct Inst
{
    uint8_t opcode;
    int disp;
    int imm32;
    int dispLen;    // 偏移的长度
    Inst();
    void init();
    void setDisp(int d, int len);   // 设置 disp, 自动检测 disp 长度 (符号) , 及时是无符号地址值也无妨
    void writeDisp();

    const Inst &operator=(const Inst &&rhs) = delete;
};

#endif
