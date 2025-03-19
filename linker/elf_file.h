#ifndef __LINKER_ELF_FILE_H__
#define __LINKER_ELF_FILE_H__

#include "elf.h"
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>

#define cast_int(x) static_cast<int>(x)

#define __link_namespace_start  namespace LINK {
#define __link_namespace_end    }


struct RelItem
{
    std::string segName;
    std::string relName;
    Elf32_Rel *rel;

    RelItem(std::string_view sname, Elf32_Rel *r, std::string_view rname);
    ~RelItem();
};


__link_namespace_start

class Elf_file
{
public:
    // elf 文件重要数据结构
    Elf32_Ehdr ehdr;                    // 文件头
    std::vector<Elf32_Phdr *> phdrTab;  // 程序头表

    std::unordered_map<std::string, Elf32_Shdr *> shdrTab;  // 段表
    std::vector<std::string> shdrNames;                     // 段表名和索引的映射关系, 方便符号查询自己的段信息
    std::unordered_map<std::string, Elf32_Sym *> symTab;    // 符号表
    std::vector<std::string> symNames;                      // 符号名与符号表项索引的映射关系, 对于重定位表生成重要
    std::vector<RelItem *> relTab;                          // 重定位表

    // 辅助数据
    std::string elf_dir;        // 处理 elf 文件的目录
    char *shstrtab = nullptr;   // 段表字符串表数据
    uint8_t shstrtabSize = 0;   // 段表字符串表长
    char *strtab = nullptr;     // 字符串表数据
    uint8_t strtabSize = 0;     // 字符串表长

public:
    Elf_file() = default;
    void readElf(const char *dir);                              // 读入 elf
    void getData(char *buf, Elf32_Off offset, Elf32_Word size); // 读取数据
    int getSegIndex(std::string_view segName);                  // 获取指定段名在段表下标
    int getSymIndex(std::string_view symName);                  // 获取指定符号名在符号表下标
    void addPhdr(Elf32_Word type, Elf32_Off off,                // 添加程序头表项
                 Elf32_Addr vaddr, Elf32_Word filesz,
                 Elf32_Word memsz, Elf32_Word flags, Elf32_Word align);
    void addShdr(const std::string &sh_name, Elf32_Word sh_type,// 添加一个段表项
                 Elf32_Word sh_flags, Elf32_Addr sh_addr,
                 Elf32_Off sh_offset, Elf32_Word sh_size,
                 Elf32_Word sh_link, Elf32_Word sh_info,
                 Elf32_Word sh_addralign, Elf32_Word sh_entsize);
    void addSym(const std::string &st_name, const Elf32_Sym *); // 添加一个符号表项
    void writeElf(const char *dir, int flag);                   // 输出 Elf 文件
    ~Elf_file();
};

__link_namespace_end

#endif
