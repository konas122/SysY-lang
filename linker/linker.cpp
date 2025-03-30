#include <cstdio>
#include <cassert>
#include <cstdint>
#include <cstring>

#include "linker.h"

using namespace std;
using namespace LINK;

extern bool showLink;


LinkBlock::LinkBlock(unique_ptr<char[]>&& d, uint32_t off, uint32_t s)
    : data(std::move(d)), offset(off), size(s)
{}

LinkBlock::~LinkBlock()
{}


// =============================================================================

SegList::~SegList() {
    ownerList.clear();
    blocks.clear();
}

/**
 * @param name: 段名
 * @param off: 文件偏移地址
 * @param base: 加载基址
 */
void SegList::allocAddr(const string &name, uint32_t &base, uint32_t &off) {
    begin = off;
    if (name != ".bss") {
        base += (MEM_ALIGN - base % MEM_ALIGN) % MEM_ALIGN;
    }

    int align = DISC_ALIGN;
    if (name == ".text") {
        align = 16;
    }
    off += (align - off % align) % align;
    base = base - base % MEM_ALIGN + off % MEM_ALIGN;   // 使虚址和偏移按照 4k 模同余

    baseAddr = base;
    offset = off;
    size = 0;

    for (auto & owner : ownerList) {
        size += (DISC_ALIGN - size % DISC_ALIGN) % DISC_ALIGN;
        auto seg = owner->shdrTab[name];

        if (name != ".bss") {
            auto buf = make_unique<char[]>(seg->sh_size);
            owner->getData(buf.get(), seg->sh_offset, seg->sh_size);
            auto block = make_shared<LinkBlock>(std::move(buf), size, seg->sh_size);
            blocks.emplace_back(block);
        }
        seg->sh_addr = base + size;
        size += seg->sh_size;
    }
    base += size;
    if (name != ".bss") {
        off += size;
    }
}

/**
 *  @param relAddr: 重定位虚拟地址
 *  @param type: 重定位类型
 *  @param symAddr: 重定位符号的虚拟地址
 */
void SegList::relocAddr(uint32_t relAddr, uint8_t type, uint32_t symAddr) {
    uint32_t relOffset = relAddr - baseAddr;    // 同类合并段的数据偏移
    // 查找修正地址所在位置
    shared_ptr<LinkBlock> b;
    for (auto block : blocks) {
        if (block->offset <= relOffset && block->offset + block->size > relOffset) {
            b = block;
            break;
        }
    }
    assert(b != nullptr && "pointer b == nullptr");
    // 处理字节为 b->data[relOffset-b->offset]
    char *pAddr = reinterpret_cast<char *>(b->data.get() + relOffset - b->offset);

    uint32_t addr = 0;
    std::memcpy(&addr, pAddr, sizeof(uint32_t));        // 相当于 `addr=*pAddr`, 但要避免未定义行为

    if (type == R_386_32) { // 绝对地址修正
        if (showLink) {
            printf("绝对地址修正: 原地址=%08x\t", addr);
        }
        std::memcpy(pAddr, &symAddr, sizeof(uint32_t)); // 相当于 `*pAddr=symAddr`, 但要避免未定义行为
        std::memcpy(&addr, pAddr, sizeof(uint32_t));    // 相当于 `addr=*pAddr`, 但要避免未定义行为
        if (showLink) {
            printf("修正后地址=%08x\n", addr);
        }
    }
    else if (type == R_386_PC32) {  // 相对地之修正
        if (showLink) {
            printf("相对地址修正: 原地址=%08x\t", addr);
        }
        addr = symAddr - relAddr + addr;
        std::memcpy(pAddr, &addr, sizeof(uint32_t));    // 相当于 `*pAddr=symAddr`, 但要避免未定义行为
        if (showLink) {
            printf("修正后地址=%08x\n", *pAddr);
        }
    }
}


// =============================================================================

Linker::Linker() {
    segNames.emplace_back(".text");
    segNames.emplace_back(".data");
    segNames.emplace_back(".bss");
    for (const auto &segName : segNames) {
        segLists[segName] = make_shared<SegList>();
    }
}

void Linker::addElf(const char *dir) {
    shared_ptr<Elf_file> elf = make_shared<Elf_file>();
    elf->readElf(dir);
    elfs.emplace_back(elf);
}

void Linker::collectInfo() {
    for (auto elf : elfs) {
        for (const auto &segName : segNames) {
            if (elf->shdrTab.count(segName)) {
                segLists[segName]->ownerList.emplace_back(elf);
            }
        }

        for (const auto &pair : elf->symTab) {
            auto symLink = make_shared<SymLink>();
            symLink->name = pair.first;

            if (pair.second->st_shndx == STN_UNDEF) {
                symLink->recv = elf;        // 记录引用文件
                symLink->prov = nullptr;    // 标记未定义
                symLinks.emplace_back(symLink);
            }
            else {
                symLink->prov = elf;        // 记录定义文件
                symLink->recv = nullptr;    // 标示该定义符号尚未被任何文件引用
                symDef.emplace_back(symLink);
            }
        }
    }
}

bool Linker::symValid() {
    bool flag = true;
    startOwner = nullptr;
    for (size_t i = 0; i < symDef.size(); ++i) {    // 遍历定义的符号, 寻找重定义信息, 和入口信息
        if (ELF32_ST_BIND(symDef[i]->prov->symTab[symDef[i]->name]->st_info) != STB_GLOBAL) {   // 只考虑全局符号
            continue;
        }
        if (symDef[i]->name == START) { // 记录程序入口文件
            startOwner = symDef[i]->prov;
        }

        for (size_t j = i + 1; j < symDef.size(); ++j) {
            if (ELF32_ST_BIND(symDef[j]->prov->symTab[symDef[j]->name]->st_info) != STB_GLOBAL) {
                continue;
            }
            if (symDef[i]->name == symDef[j]->name) {
                printf("符号名 %s 在文件 %s 和文件 %s 中发生链接冲突\n", symDef[i]->name.c_str(), symDef[i]->prov->elf_dir.c_str(), symDef[j]->prov->elf_dir.c_str());
                flag = false;
            }
        }
    }

    if (startOwner == nullptr) {
        printf("链接器找不到程序入口 %s\n", START);
        flag = false;
    }

    for (size_t i = 0; i < symLinks.size(); ++i) {      // 遍历未定义符号
        for (size_t j = 0; j < symDef.size(); ++j) {    // 遍历定义的符号
            if (ELF32_ST_BIND(symDef[j]->prov->symTab[symDef[j]->name]->st_info) != STB_GLOBAL) {   // 只考虑全局符号
                continue;
            }
            if (symLinks[i]->name == symDef[j]->name) {
                symLinks[i]->prov = symDef[j]->prov;
                symDef[j]->recv = symDef[j]->prov;
            }
        }
        if (symLinks[i]->prov == NULL) {
            uint8_t info = symLinks[i]->recv->symTab[symLinks[i]->name]->st_info;
            string type;
            if (ELF32_ST_TYPE(info) == STT_OBJECT) {
                type = "变量";
            }
            else if (ELF32_ST_TYPE(info) == STT_FUNC) {
                type = "函数";
            }
            else {
                type = "符号";
            }
            printf("文件 %s 的 %s 名 %s 未定义.\n", symLinks[i]->recv->elf_dir.c_str(), type.c_str(), symLinks[i]->name.c_str());
            flag = false;
        }
    }
    return flag;
}

void Linker::allocAddr() {
    uint32_t curAddr = BASE_ADDR;                   // 当前加载基址
    uint32_t curOff = 52 + 32 * segNames.size();    // 默认文件偏移, PHT 保留 .bss 段
    if (showLink) {
        printf("----------地址分配----------\n");
    }
    for (const auto &segName : segNames) {  // 按照类型分配地址, 不紧邻 .data 与 .bss 段
        segLists[segName]->allocAddr(segName, curAddr, curOff); // 自动分配
        if (showLink) {
            printf("%s\taddr=%08x\toff=%08x\tsize=%08x(%d)\n", segName.c_str(),
                   segLists[segName]->baseAddr, segLists[segName]->offset,
                   segLists[segName]->size, cast_int(segLists[segName]->size));
        }
    }
}

void Linker::symParser() {
    // 扫描所有定义符号, 原地计算虚拟地址
    if (showLink) {
        printf("----------定义符号解析----------\n");
    }

    for (size_t i = 0; i < symDef.size(); ++i) {
        auto sym = symDef[i]->prov->symTab[symDef[i]->name];        // 定义的符号信息
        string segName = symDef[i]->prov->shdrNames[sym->st_shndx]; // 段名
        sym->st_value = sym->st_value +                             // 偏移
                        symDef[i]->prov->shdrTab[segName]->sh_addr; // 段基址
        if (showLink) {
            printf("%s\t%08x\t%s\n", symDef[i]->name.c_str(), sym->st_value, symDef[i]->prov->elf_dir.c_str());
        }
    }
    // 扫描所有符号引用, 绑定虚拟地址
    if (showLink) {
        printf("----------未定义符号解析----------\n");
    }
    for (auto symLink : symLinks) {
        const auto provsym = symLink->prov->symTab[symLink->name];  // 被引用的符号信息
        auto recvsym = symLink->recv->symTab[symLink->name];        // 被引用的符号信息
        recvsym->st_value = provsym->st_value;                      // 被引用符号已经解析了
        if (showLink) {
            printf("%s\t%08x\t%s\n", symLink->name.c_str(), recvsym->st_value, symLink->recv->elf_dir.c_str());
        }
    }
}

void Linker::relocate() {
    // 重定位项符号必然在符号表中, 且地址已经解析完毕
    if (showLink) {
        printf("--------------重定位----------------\n");
    }
    for (const auto &elf : elfs) {
        for (auto rel : elf->relTab) {  // 遍历重定位项
            const auto sym = elf->symTab[rel->relName];                                     // 重定位符号信息
            uint32_t symAddr = sym->st_value;                                               // 解析后的符号段偏移为虚拟地址
            uint32_t relAddr = elf->shdrTab[rel->segName]->sh_addr + rel->rel->r_offset;    // 重定位地址
            // 重定位操作
            if (showLink) {
                printf("%s\trelAddr=%08x\tsymAddr=%08x\n", rel->relName.c_str(), relAddr, symAddr);
            }
            segLists[rel->segName]->relocAddr(relAddr, ELF32_R_TYPE(rel->rel->r_info), symAddr);
        }
    }
}

void Linker::assemExe() {
    {
        int *p_id = reinterpret_cast<int *>(exe.ehdr.e_ident);
        assert(reinterpret_cast<uintptr_t>(p_id) % alignof(int) == 0);

        *p_id = 0x464c457f;
        p_id++;
        *p_id = 0x010101;
        p_id++;
        *p_id = 0;
        p_id++;
        *p_id = 0;
    }
    exe.ehdr.e_type = ET_EXEC;
    exe.ehdr.e_machine = EM_386;
    exe.ehdr.e_version = EV_CURRENT;
    exe.ehdr.e_flags = 0;
    exe.ehdr.e_ehsize = 52;

    uint32_t curOff = 52 + 32 * segNames.size();
    exe.addShdr("", 0, 0, 0, 0, 0, 0, 0, 0, 0); // 空段表项
    int shstrtabSize = 26;

    for (const auto& segName : segNames) {
        shstrtabSize += segName.length() + 1;

        // 生成程序头表
        Elf32_Word flags = PF_W | PF_R;                 // 读写
        Elf32_Word filesz = segLists[segName]->size;    // 占用磁盘大小
        if (segName == ".text") {
            flags = PF_X | PF_R;    // .text 段可读可执行
        }
        if (segName == ".bss") {
            filesz = 0;             // .bss 段不占磁盘空间
        }
        exe.addPhdr(PT_LOAD, segLists[segName]->offset, segLists[segName]->baseAddr,
                    filesz, segLists[segName]->size, flags, MEM_ALIGN);     // 添加程序头表项
        curOff = segLists[segName]->offset; // 修正当前偏移, 循环结束后保留的是 .bss 的基址

        // 生成段表项
        Elf32_Word sh_type = SHT_PROGBITS;
        Elf32_Word sh_flags = SHF_ALLOC | SHF_WRITE;
        Elf32_Word sh_align = 4;    // 4B
        if (segName == ".bss") {
            sh_type = SHT_NOBITS;
        }
        if (segName == ".text") {
            sh_flags = SHF_ALLOC | SHF_EXECINSTR;
            sh_align = 16;          // 16B
        }
        exe.addShdr(segName, sh_type, sh_flags, segLists[segName]->baseAddr, segLists[segName]->offset,
                    segLists[segName]->size, SHN_UNDEF, 0, sh_align, 0);    // 添加一个段表项, 暂时按照 4 字节对齐
    }
    exe.ehdr.e_phoff = 52;
    exe.ehdr.e_phentsize = 32;
    exe.ehdr.e_phnum = segNames.size();

    // 填充 shstrtab 数据
    exe.shstrtab = make_unique<char[]>(shstrtabSize);
    exe.shstrtabSize = shstrtabSize;
    char *str = exe.shstrtab.get();
    int index = 0;
    // 段表串名与索引映射
    unordered_map<string, int> shstrIndex;
    shstrIndex[".shstrtab"] = index;
    strncpy(str + index, ".shstrtab", 10);
    index += 10;
    shstrIndex[".symtab"] = index;
    strncpy(str + index, ".symtab", 8);
    index += 8;
    shstrIndex[".strtab"] = index;
    strncpy(str + index, ".strtab", 8);
    index += 8;
    shstrIndex[""] = index - 1;
    for (const auto &segName : segNames) {
        shstrIndex[segName] = index;
        int len = segName.length() + 1;
        strncpy(str + index, segName.c_str(), len);
        index += len;
    }

    // 生成 .shstrtab
    exe.addShdr(".shstrtab", SHT_STRTAB, 0, 0, curOff, shstrtabSize, SHN_UNDEF, 0, 1, 0);   // .shstrtab
    exe.ehdr.e_shstrndx = exe.getSegIndex(".shstrtab");
    curOff += shstrtabSize;                 // 段表偏移
    exe.ehdr.e_shoff = curOff;
    exe.ehdr.e_shentsize = 40;
    exe.ehdr.e_shnum = 4 + segNames.size(); // 段表数

    // 生成符号表项
    curOff += 40 * (4 + segNames.size());   // 符号表偏移
    exe.addShdr(".symtab", SHT_SYMTAB, 0, 0, curOff, (1 + symDef.size()) * 16, 0, 0, 1, 16);
    exe.shdrTab[".symtab"]->sh_link = exe.getSegIndex(".symtab") + 1; // .strtab 默认在 .symtab 之后
    int strtabSize = 0;         // 字符串表大小
    exe.addSym("", nullptr);    // 空符号表项
    for (const auto &symlink : symDef) {
        string name = symlink->name;
        strtabSize += name.length() + 1;
        auto sym = symlink->prov->symTab[name];
        sym->st_shndx = exe.getSegIndex(symlink->prov->shdrNames[sym->st_shndx]);   // 重定位后可以修改了
        exe.addSym(name, sym);
    }

    // 记录程序入口
    exe.ehdr.e_entry = exe.symTab[START]->st_value; // 程序入口地址
    curOff += (1 + symDef.size()) * 16;             // .strtab 偏移
    exe.addShdr(".strtab", SHT_STRTAB, 0, 0, curOff, strtabSize, SHN_UNDEF, 0, 1, 0);
    // 填充 strtab 数据
    exe.strtab = make_unique<char[]>(strtabSize);
    exe.strtabSize = strtabSize;
    str = exe.strtab.get();
    index = 0;
    // 串表与索引映射
    unordered_map<string, int> strIndex;
    strIndex[""] = strtabSize - 1;
    for (const auto &sym : symDef) {
        strIndex[sym->name] = index;
        int len = sym->name.length() + 1;
        strncpy(str + index, sym->name.c_str(), len);
        index += len;
    }

    // 更新符号表 name
    for (auto i = exe.symTab.cbegin(); i != exe.symTab.cend(); ++i) {
        i->second->st_name = strIndex[i->first];
    }
    // 更新段表 name
    for (auto i = exe.shdrTab.cbegin(); i != exe.shdrTab.cend(); ++i) {
        i->second->sh_name = shstrIndex[i->first];
    }
}

void Linker::exportElf(const char *dir) {
    exe.writeElf(dir, 1);
    FILE *fp = fopen(dir, "a+");
    const char pad[1] = {0};
    for (const auto &segName : segNames) {
        const auto sl = segLists[segName];
        int padnum = sl->offset - sl->begin;
        while (padnum--) {
            fwrite(pad, 1, 1, fp);
        }

        if (segName != ".bss") {
            shared_ptr<LinkBlock> old = nullptr;
            const char instPad[1] = {(char)0x90};
            for (auto block : sl->blocks) {
                if (old != nullptr) {
                    padnum = block->offset - (old->offset + old->size);
                    while (padnum--) {
                        fwrite(instPad, 1, 1, fp);
                    }
                }
                old = block;
                fwrite(block->data.get(), block->size, 1, fp);
            }
        }
    }
    fclose(fp);
    exe.writeElf(dir, 2);   // 输出链接后的 elf 后半段
}

bool Linker::link(const char *dir) {
    collectInfo();      // 搜集段/符号信息
    if (!symValid()) {  // 符号引用验证
        return false;
    }
    allocAddr();    // 分配地址空间
    symParser();    // 符号地址解析
    relocate();     // 重定位
    assemExe();     // 组装文件 exe
    exportElf(dir); // 输出 elf 可执行文件
    return true;
}

Linker::~Linker() {
    // 清空合并段序列
    segLists.clear();

    // 清空符号引用序列
    symLinks.clear();

    // 清空符号定义序列
    symDef.clear();

    // 清空目标文件
    elfs.clear();
}
