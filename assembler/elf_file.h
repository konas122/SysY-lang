#ifndef __ASSEMBLER_ELF_FILE_H__
#define __ASSEMBLER_ELF_FILE_H__

#include "ass.h"
#include "elf.h"

#include <vector>
#include <string>
#include <unordered_map>

using namespace std;

struct lb_record;


//重定位信息
struct RelInfo
{
    string tarSeg; // 重定位目标段
    int offset;    // 重定位位置的偏移
    string lbName; // 重定位符号的名称
    int type;      // 重定位类型 0-R_386_32: 1-R_386_PC32
    RelInfo(string seg, int addr, string lb, int t);
};


// elf 文件类, 包含 elf 文件的重要内容, 处理 elf 文件
class Elf_file
{
public:
    // elf 文件重要数据结构
    Elf32_Ehdr ehdr;                                // 文件头
    unordered_map<string, Elf32_Shdr *> shdrTab;    // 段表
    vector<string> shdrNames;                       // 段表名和索引的映射关系, 方便符号查询自己的段信息
    unordered_map<string, Elf32_Sym *> symTab;      // 符号表
    vector<string> symNames;                        // 符号名与符号表项索引的映射关系, 对于重定位表生成重要
    vector<RelInfo *> relTab;

    // 辅助数据
    char *shstrtab;     // 段表字符串表数据
    int shstrtabSize;   // 段表字符串表长
    char *strtab;       // 字符串表数据
    int strtabSize;     // 字符串表长
    vector<Elf32_Rel *> relTextTab, relDataTab;

public:
	Elf_file();
    int getSegIndex(string segName);        // 获取指定段名在段表下标
    int getSymIndex(string symName);        // 获取指定符号名在符号表下标
    void addShdr(string sh_name, int size);
    void addShdr(string sh_name, Elf32_Word sh_type, Elf32_Word sh_flags, Elf32_Addr sh_addr, Elf32_Off sh_offset,
                 Elf32_Word sh_size, Elf32_Word sh_link, Elf32_Word sh_info, Elf32_Word sh_addralign,
                 Elf32_Word sh_entsize);    // 添加一个段表项
    void addSym(lb_record *lb);
    RelInfo *addRel(string seg, int addr, string lb, int type); // 添加一个重定位项, 相同段的重定位项连续 (一般是先是.rel.text后.rel.data)
    void padSeg(string first, string second);                   // 填充段间的空隙
    void assmObj();                                             // 组装文件
    void writeElfTail();                                        // 输出文件尾部
	void writeElf();
	void printAll();	
	~Elf_file();
};

#endif
