#include <cstring>
#include <cassert>

#include "semantic.h"
#include "elf_file.h"

using namespace ASS;

Elf_file obj;


RelInfo::RelInfo(const string &seg, int addr, const string &lb, int t)
    : tarSeg(seg), offset(addr), lbName(lb), type(t)
{ }


Elf_file::Elf_file() {
    addShdr("", 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int Elf_file::getSegIndex(const string &segName) {
    int index = 0;
    for (size_t i = 0; i < shdrNames.size(); ++i) {
        if (shdrNames[i] == segName) {
            break;
        }
        ++index;
    }
    return index;
}

int Elf_file::getSymIndex(const string &symName) {
    int index = 0;
    for (size_t i = 0; i < symNames.size(); ++i) {
        if (symNames[i] == symName) {
            break;
        }
        ++index;
    }
    return index;
}

void Elf_file::addShdr(const string &sh_name, int size) {
    int off = 52 + dataLen;
    if (sh_name == ".text") {
        addShdr(sh_name, SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, 0, off, size, 0, 0, 4, 0);
    }
    else if (sh_name == ".data") {
        addShdr(sh_name, SHT_PROGBITS, SHF_ALLOC | SHF_WRITE, 0, off, size, 0, 0, 4, 0);
    }
    else if (sh_name == ".bss") {
        addShdr(sh_name, SHT_NOBITS, SHF_ALLOC | SHF_WRITE, 0, off, size, 0, 0, 4, 0);
    }
}

void Elf_file::addShdr(const string &sh_name,
    Elf32_Word sh_type, Elf32_Word sh_flags, Elf32_Addr sh_addr, Elf32_Off sh_offset,
    Elf32_Word sh_size, Elf32_Word sh_link, Elf32_Word sh_info, Elf32_Word sh_addralign,
    Elf32_Word sh_entsize)  // 添加一个段表项
{
    auto sh = make_unique<Elf32_Shdr>();
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
    shdrTab[sh_name] = std::move(sh);
    shdrNames.emplace_back(sh_name);
}

void Elf_file::addSym(std::shared_ptr<lb_record> lb) {
    bool glb = lb->global;

    if (lb->segName == "") {
        glb = lb->externed;
    }

    auto symbol = make_unique<Elf32_Sym>();
    symbol->st_name = 0;
    symbol->st_value = lb->addr;    // 符号段偏移, 外部符号地址为 0
    symbol->st_size = lb->times * lb->len * lb->cont.size();    // 函数无法通过目前的设计确定, 而且不必关心
    if (glb) {                                                  // 统一记作无类型符号, 和链接器协议保持一致
        symbol->st_info = ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE);
    }
    else {
        symbol->st_info = ELF32_ST_INFO(STB_LOCAL, STT_NOTYPE); // 局部符号, 避免名字冲突
    }
    symbol->st_other = 0;
    if (lb->externed) {
        symbol->st_shndx = STN_UNDEF;
    }
    else {
        symbol->st_shndx = getSegIndex(lb->segName);
    }
    symTab[lb->lbName] = std::move(symbol);
    symNames.emplace_back(lb->lbName);
}


void Elf_file::addRel(const string &seg, int addr, const string &lb, int type) {
    auto rel = make_unique<RelInfo>(seg, addr, lb, type);
    relTab.emplace_back(std::move(rel));
}

void Elf_file::assmObj() {
    vector<string> AllSegNames = shdrNames;
    AllSegNames.emplace_back(".shstrtab");
    AllSegNames.emplace_back(".symtab");
    AllSegNames.emplace_back(".strtab");
    AllSegNames.emplace_back(".rel.text");
    AllSegNames.emplace_back(".rel.data");

    const char magic[] = {
        0x7f, 0x45, 0x4c, 0x46,
        0x01, 0x01, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00};
    memcpy(&ehdr.e_ident, magic, sizeof(magic));

    ehdr.e_type = ET_REL;
    ehdr.e_machine = EM_386;
    ehdr.e_version = EV_CURRENT;
    ehdr.e_entry = 0;
    ehdr.e_phoff = 0;
    ehdr.e_shoff = 0;
    ehdr.e_flags = 0;
    ehdr.e_ehsize = sizeof(Elf32_Ehdr);
    ehdr.e_phentsize = 0;
    ehdr.e_phnum = 0;
    ehdr.e_shentsize = sizeof(Elf32_Shdr);
    ehdr.e_shnum = AllSegNames.size();
    ehdr.e_shstrndx = 4;

    // 填充 .shstrtab 数据
    int curOff = 52 + dataLen;  // header + (.text+pad+.data+pad) 数据偏移, .shstrtab 偏移
    shstrtabSize = 51;
    shstrtab = make_unique<char[]>(shstrtabSize);
    char *tmpStr = shstrtab.get();
    int idx = 0;
    // 段表串名与索引映射
    unordered_map<string, int> shstrIndex;
    shstrIndex[".rel.text"] = idx;
    strncpy(tmpStr + idx, ".rel.text", 10);
    shstrIndex[".text"] = idx + 4;
    idx += 10;
    shstrIndex[""] = idx - 1;
    shstrIndex[".rel.data"] = idx;
    strncpy(tmpStr + idx, ".rel.data", 10);
    shstrIndex[".data"] = idx + 4;
    idx += 10;
    shstrIndex[".bss"] = idx;
    strncpy(tmpStr + idx, ".bss", 5);
    idx += 5;
    shstrIndex[".shstrtab"] = idx;
    strncpy(tmpStr + idx, ".shstrtab", 10);
    idx += 10;
    shstrIndex[".symtab"] = idx;
    strncpy(tmpStr + idx, ".symtab", 8);
    idx += 8;
    shstrIndex[".strtab"] = idx;
    strncpy(tmpStr + idx, ".strtab", 8);

    assert(idx + 8 <= shstrtabSize);

    // 添加 .shstrtab
    addShdr(".shstrtab", SHT_STRTAB, 0, 0, curOff, shstrtabSize, SHN_UNDEF, 0, 1, 0);

    // 定位段表
    curOff += shstrtabSize;
    ehdr.e_shoff = curOff;

    // 添加符号表
    curOff += 9 * 40;   // 8 个段 + 空段, 段表字符串表偏移, 符号表表偏移

    // .symtab, sh_link 代表 .strtab 索引, 默认在 .symtab 之后, sh_info 不能确定
    addShdr(".symtab", SHT_SYMTAB, 0, 0, curOff, symNames.size() * 16, 0, 0, 1, 16);
    shdrTab[".symtab"]->sh_link = getSegIndex(".symtab") + 1;   // .strtab 默认在 .symtab 之后

    // 添加 .strtab
    strtabSize = 0;
    for (const auto &symName : symNames) { // 遍历所有符号
        strtabSize += symName.length() + 1;
    }
    curOff += symNames.size() * 16;                                                 // .strtab 偏移
    addShdr(".strtab", SHT_STRTAB, 0, 0, curOff, strtabSize, SHN_UNDEF, 0, 1, 0);   // .strtab

    // 填充 strtab 数据
    strtab = make_unique<char[]>(strtabSize);
    tmpStr = strtab.get();

    // 串表与符号表名字更新
    for (size_t i = 0, idx = 0; i < symNames.size(); ++i) {
        symTab[symNames[i]]->st_name = idx;
        int len = symNames[i].length() + 1;
        strncpy(tmpStr + idx, symNames[i].c_str(), len);
        idx += len;
    }
    assert(idx <= strtabSize);

    // 处理重定位表
    for (size_t i = 0; i < relTab.size(); i++) {
        auto rel = make_unique<Elf32_Rel>();
        rel->r_offset = relTab[i]->offset;
        rel->r_info = ELF32_R_INFO((Elf32_Word)getSymIndex(relTab[i]->lbName), relTab[i]->type);
        if (relTab[i]->tarSeg == ".text") {
            relTextTab.emplace_back(std::move(rel));
        }
        else if (relTab[i]->tarSeg == ".data") {
            relDataTab.emplace_back(std::move(rel));
        }
        else {
            rel.reset();
        }
    }

    // 添加 .rel.text
    curOff += strtabSize;
    addShdr(".rel.text", SHT_REL, 0, 0, curOff, relTextTab.size() * 8, getSegIndex(".symtab"), getSegIndex(".text"), 1, 8); // .rel.text

    // 添加 .rel.data
    curOff += relTextTab.size() * 8;
    addShdr(".rel.data", SHT_REL, 0, 0, curOff, relDataTab.size() * 8, getSegIndex(".symtab"), getSegIndex(".data"), 1, 8); // .rel.data

    // 更新段表 name
    for (const auto &name : AllSegNames) {
        shdrTab[name]->sh_name = shstrIndex[name];
    }
}

void Elf_file::writeElf() {
    fclose(fout);
    fout = fopen((finName + ".o").c_str(), "w");
    assmObj();  // 组装文件
    fwrite(&ehdr, ehdr.e_ehsize, 1, fout);

    // 输出 .text
    fclose(fin);
    fin = fopen((finName + ".t").c_str(), "r"); // 临时输出文件, 供代码段使用
    char buffer[1024] = {0};
    int f = -1;

    while (f != 0) {
        f = fread(buffer, 1, 1024, fin);
        fwrite(buffer, 1, f, fout);
    }

    padSeg(".text", ".data");
    table.write();              // 输出 .data 段
    padSeg(".data", ".bss");    // .bss不用输出, 对齐即可

    // .shstrtab, 段表, 符号表, .strtab, .rel.text, .rel.data
    obj.writeElfTail();
}

// 填充段间的空隙
void Elf_file::padSeg(const string &first, const string &second) {
    const char pad[1] = {0};
    int padNum = shdrTab[second]->sh_offset - (shdrTab[first]->sh_offset + shdrTab[first]->sh_size);
    if (padNum <= 0) {
        return;
    }
    while (padNum--) {
        fwrite(pad, 1, 1, fout);    // 填充
    }
}

void Elf_file::writeElfTail() {
    fwrite(shstrtab.get(), shstrtabSize, 1, fout);    // .shstrtab

    for (const auto &shdrName : shdrNames) {    // 段表
        const Elf32_Shdr *sh = shdrTab[shdrName].get();
        fwrite(sh, ehdr.e_shentsize, 1, fout);
    }

    for (const auto &symName : symNames) {      // 符号表
        const Elf32_Sym *tmpSym = symTab[symName].get();
        fwrite(tmpSym, sizeof(Elf32_Sym), 1, fout);
    }

    fwrite(strtab.get(), strtabSize, 1, fout);  // .strtab

    for (auto& rel : relTextTab) {              // .rel.text
        fwrite(rel.get(), sizeof(Elf32_Rel), 1, fout);
        rel.reset();
    }

    for (auto& rel : relDataTab) {              // .rel.data
        fwrite(rel.get(), sizeof(Elf32_Rel), 1, fout);
        rel.reset();
    }
}

Elf_file::~Elf_file() {
    // 清空段表
    shdrTab.clear();
    shdrNames.clear();

    // 清空符号表
    symTab.clear();

    // 清空重定位表
    relTab.clear();
}

void Elf_file::printAll() {
    if (!showAss)
        return;
    cout << "------------段信息------------\n";
    for (auto i = shdrTab.cbegin(); i != shdrTab.cend(); ++i) {
        if (i->first == "") {
            continue;
        }
        cout << i->first << ":" << i->second->sh_size << endl;
    }
    cout << "------------符号信息------------\n";
    for (auto i = symTab.cbegin(); i != symTab.cend(); ++i) {
        if (i->first == "") {
            continue;
        }
        cout << i->first << ": ";
        if (i->second->st_shndx == 0) {
            cout << "外部";
        }
        if (ELF32_ST_BIND(i->second->st_info) == STB_GLOBAL) {
            cout << "全局";
        }
        else if (ELF32_ST_BIND(i->second->st_info) == STB_LOCAL) {
            cout << "局部";
        }
        cout << endl;
    }
    cout << "------------重定位信息------------\n";
    for (auto i = relTab.cbegin(); i != relTab.cend(); ++i) {
        cout << (*i)->tarSeg << ":" << (*i)->offset << "<-" << (*i)->lbName << endl;
    }
}
