#include <cstring>
#include <iostream>

#include "elf_file.h"

using namespace std;
using namespace LINK;

extern bool showLink;


RelItem::RelItem(std::string_view sname, std::shared_ptr<Elf32_Rel> r, std::string_view rname)
    : segName(sname), relName(rname), rel(r)
{}

RelItem::~RelItem()
{}


// =============================================================================

void Elf_file::getData(char *buf, Elf32_Off offset, Elf32_Word size) {
    FILE *fp = fopen(elf_dir.c_str(), "rb");
    rewind(fp);
    fseek(fp, offset, 0);
    fread(buf, size, 1, fp);
    fclose(fp);
}

void Elf_file::readElf(const char *dir) {
    elf_dir = dir;
    FILE *fp = fopen(dir, "rb");
    rewind(fp);
    fread(&ehdr, sizeof(Elf32_Ehdr), 1, fp);

    if (ehdr.e_type == ET_EXEC) {   // 可执行文件拥有程序头表
        fseek(fp, ehdr.e_phoff, 0); // 程序头表位置
        for (int i = 0; i < ehdr.e_phnum; ++i) {
            auto phdr = make_shared<Elf32_Phdr>();
            fread(phdr.get(), ehdr.e_phentsize, 1, fp);
            phdrTab.emplace_back(phdr);
        }
    }

    fseek(fp, ehdr.e_shoff + ehdr.e_shentsize * ehdr.e_shstrndx, 0);    // 段表字符串表位置
    Elf32_Shdr shstrTab;
    fread(&shstrTab, ehdr.e_shentsize, 1, fp);                          // 读取段表字符串表项

    auto shstrTabData = make_unique<char[]>(shstrTab.sh_size);
    fseek(fp, shstrTab.sh_offset, 0);               // 转移到段表字符串表内容
    fread(shstrTabData.get(), shstrTab.sh_size, 1, fp);   // 读取段表字符串表

    fseek(fp, ehdr.e_shoff, 0);                 // 段表位置
    for (int i = 0; i < ehdr.e_shnum; ++i) {    // 读取段表
        auto shdr = make_shared<Elf32_Shdr>();
        fread(shdr.get(), ehdr.e_shentsize, 1, fp); // 读取段表项
        string name(shstrTabData.get() + shdr->sh_name);
        shdrNames.emplace_back(name);               // 记录段表名位置
        if (name.empty()) {
            // 删除空段表项
        }
        else {
            shdrTab[name] = shdr;
        }
    }

    auto strTab = shdrTab[".strtab"];    // 字符串表信息
    auto strTabData = make_unique<char[]>(strTab->sh_size);
    fseek(fp, strTab->sh_offset, 0);                    // 转移到字符串表内容
    fread(strTabData.get(), strTab->sh_size, 1, fp);    // 读取字符串表

    const auto sh_symTab = shdrTab[".symtab"];   // 符号表信息
    fseek(fp, sh_symTab->sh_offset, 0);                 // 转移到符号表内容
    int symNum = sh_symTab->sh_size / 16;
    vector<shared_ptr<Elf32_Sym>> symList;
    for (int i = 0; i < symNum; ++i) {
        auto sym = make_shared<Elf32_Sym>();
        fread(sym.get(), 16, 1, fp);            // 读取符号项
        symList.emplace_back(sym);              // 添加到符号序列
        string name(strTabData.get() + sym->st_name);
        if (name.empty()) {

        }
        else {
            symTab[name] = sym;
        }
    }

    if (showLink) {
        printf("----------%s 重定位数据:----------\n", elf_dir.c_str());
    }

    for (auto &pair : shdrTab) {
        if (pair.first.find(".rel") == 0) {             // 是重定位段
            const auto sh_relTab = shdrTab[pair.first]; // 重定位表信息
            fseek(fp, sh_relTab->sh_offset, 0);         // 转移到重定位表内容
            int relNum = sh_relTab->sh_size / 8;
            for (int i = 0; i < relNum; ++i) {
                auto rel = make_shared<Elf32_Rel>();
                fread(rel.get(), 8, 1, fp);
                string name(strTabData.get() + symList[ELF32_R_SYM(rel->r_info)]->st_name);
                auto r = make_shared<RelItem>(pair.first.substr(4), rel, name);
                relTab.emplace_back(r);
                if (showLink) {
                    cout << r->segName << "\t" << r->relName << "\t" << rel->r_offset << endl;
                }
            }
        }
    }

    fclose(fp);
}

int Elf_file::getSegIndex(std::string_view segName) {
    int index = 0;
    for (const auto &shdrName : shdrNames) {
        if (shdrName == segName) {
            break;
        }
        ++index;
    }
    return index;
}

int Elf_file::getSymIndex(std::string_view name) {
    int index = 0;
    for (const auto &symName : symNames) {
        if (symName == name) {
            break;
        }
        ++index;
    }
    return index;
}

void Elf_file::addPhdr(Elf32_Word type, Elf32_Off off,
                       Elf32_Addr vaddr, Elf32_Word filesz,
                       Elf32_Word memsz, Elf32_Word flags, Elf32_Word align)
{
    auto ph = make_shared<Elf32_Phdr>();
    ph->p_type = type;
    ph->p_offset = off;
    ph->p_vaddr = ph->p_paddr = vaddr;
    ph->p_filesz = filesz;
    ph->p_memsz = memsz;
    ph->p_flags = flags;
    ph->p_align = align;
    phdrTab.emplace_back(ph);
}

void Elf_file::addShdr(const string &sh_name, Elf32_Word sh_type, Elf32_Word sh_flags,
                       Elf32_Addr sh_addr, Elf32_Off sh_offset, Elf32_Word sh_size,
                       Elf32_Word sh_link, Elf32_Word sh_info, Elf32_Word sh_addralign,
                       Elf32_Word sh_entsize)
{
    auto sh = make_shared<Elf32_Shdr>();
    sh->sh_name = 0;
    sh->sh_type = sh_type;
    sh->sh_flags = sh_flags;
    sh->sh_addr = sh_addr;
    sh->sh_offset = sh_offset;
    sh->sh_size = sh_size;
    sh->sh_link = sh_link;
    sh->sh_info = sh_info;
    sh->sh_addralign = sh_addralign;
    sh->sh_entsize = sh_entsize;
    shdrTab[sh_name] = sh;
    shdrNames.emplace_back(sh_name);
}

void Elf_file::addSym(const string &st_name, const shared_ptr<Elf32_Sym> s) {
    auto sym = symTab[st_name] = make_shared<Elf32_Sym>();
    if (st_name == "") {
        sym->st_name = 0;
        sym->st_value = 0;
        sym->st_size = 0;
        sym->st_info = 0;
        sym->st_other = 0;
        sym->st_shndx = 0;
    }
    else {
        sym->st_name = 0;
        sym->st_value = s->st_value;
        sym->st_size = s->st_size;
        sym->st_info = s->st_info;
        sym->st_other = s->st_other;
        sym->st_shndx = s->st_shndx;
    }
    symNames.emplace_back(st_name);
}

/**
 * @param dir:输出目录
 * @param flag: 1 第一次写, 文件头+PHT
 *              2 第二次写, 段表字符串表+段表+符号表+字符串表
 */
void Elf_file::writeElf(const char*dir, int flag) {
    if (flag == 1) {
        FILE *fp = fopen(dir, "w+");
        fwrite(&ehdr, ehdr.e_ehsize, 1, fp);    // elf 文件头
        if(!phdrTab.empty()) {                  // 程序头表
            for (size_t i = 0; i < phdrTab.size(); ++i) {
                fwrite(phdrTab[i].get(), ehdr.e_phentsize, 1, fp);
            }
        }
        fclose(fp);
    }
    else if (flag == 2) {
        FILE *fp = fopen(dir, "a+");
        fwrite(shstrtab.get(), shstrtabSize, 1, fp);    // .shstrtab
        for (size_t i = 0; i < shdrNames.size(); ++i) { // 段表
            const auto sh = shdrTab[shdrNames[i]];
            fwrite(sh.get(), ehdr.e_shentsize, 1, fp);
        }
        for (size_t i = 0; i < symNames.size(); ++i) {  // 符号表
            auto sym = symTab[symNames[i]];
            fwrite(sym.get(), sizeof(Elf32_Sym), 1, fp);
        }

        fwrite(strtab.get(), strtabSize, 1, fp);        // .strtab
        fclose(fp);
    }
}

Elf_file::~Elf_file() {
    // 清空程序头表
    phdrTab.clear();

    // 清空段表
    shdrTab.clear();
    shdrNames.clear();

    // 清空符号表
    symTab.clear();

    // 清空重定位表
    relTab.clear();
}
