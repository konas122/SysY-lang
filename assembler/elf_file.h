#ifndef __ASSEMBLER_ELF_FILE_H__
#define __ASSEMBLER_ELF_FILE_H__

#include "ass.h"
#include "elf.h"

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

#define __ass_namespace_start   namespace ASS {
#define __ass_namespace_end     }

using namespace std;

struct lb_record;


// 重定位信息
struct RelInfo
{
    string tarSeg; // 重定位目标段
    int offset;    // 重定位位置的偏移
    string lbName; // 重定位符号的名称
    int type;      // 重定位类型 0-R_386_32: 1-R_386_PC32
    RelInfo(std::string_view seg, int addr, std::string_view lb, int t);
};


__ass_namespace_start

// elf 文件类, 包含 elf 文件的重要内容, 处理 elf 文件
class Elf_file
{
public:
    // elf 文件重要数据结构
    Elf32_Ehdr ehdr;                                        // 文件头
    unordered_map<string, unique_ptr<Elf32_Shdr>> shdrTab;  // 段表
    vector<string> shdrNames;                               // 段表名和索引的映射关系, 方便符号查询自己的段信息
    unordered_map<string, unique_ptr<Elf32_Sym>> symTab;    // 符号表
    vector<string> symNames;                                // 符号名与符号表项索引的映射关系, 对于重定位表生成重要
    vector<unique_ptr<RelInfo>> relTab;

    // 辅助数据
    unique_ptr<char[]> shstrtab;    // 段表字符串表数据
    int shstrtabSize = 0;           // 段表字符串表长
    unique_ptr<char[]> strtab;  // 字符串表数据
    int strtabSize = 0;         // 字符串表长
    vector<unique_ptr<Elf32_Rel>> relTextTab, relDataTab;

public:
    Elf_file();
    int getSegIndex(std::string_view segName);     // 获取指定段名在段表下标
    int getSymIndex(std::string_view symName);     // 获取指定符号名在符号表下标
    void addShdr(const std::string &sh_name, int size);
    void addShdr(const std::string &sh_name,
                Elf32_Word sh_type, Elf32_Word sh_flags,
                Elf32_Addr sh_addr, Elf32_Off sh_offset,
                Elf32_Word sh_size, Elf32_Word sh_link,
                Elf32_Word sh_info, Elf32_Word sh_addralign,
                Elf32_Word sh_entsize);         // 添加一个段表项
    void addSym(std::shared_ptr<lb_record> lb);
    void addRel(std::string_view seg, int addr,
                    std::string_view lb, int type);            // 添加一个重定位项, 相同段的重定位项连续 (一般是先是.rel.text后.rel.data)
    void padSeg(const std::string &first, const std::string &second); // 填充段间的空隙
    void assmObj();                                         // 组装文件
    void writeElfTail();                                    // 输出文件尾部
    void writeElf();
    void printAll();
    ~Elf_file();
};

__ass_namespace_end

extern ASS::Elf_file obj; // 输出文件

#endif
