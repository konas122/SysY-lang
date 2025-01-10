#include "elf_file.h"
#include "semantic.h"


Table table;    // 符号表全局对象
int lb_record::curAddr = 0x00000000;

lb_record::lb_record(string n, bool ex) {
    lbName = n;
    addr = lb_record::curAddr;
    externed = ex;
    isEqu = false;
    segName = curSeg;
    times = 0;
    len = 0;
    if (ex) {
        addr = 0;
        segName = "";
    }
}

lb_record::lb_record(string n, int a) { // L equ 1
    lbName = n;
    segName = curSeg;
    addr = a;
    isEqu = true;
    externed = false;
    times = 0;
    len = 0;
}

lb_record::lb_record(string n, int t, int l, list<int> &c) {    // L times 10 dw 10,"1234"
    lbName = n;
    addr = lb_record::curAddr;
    segName = curSeg;
    isEqu = false;
    times = t;
    cont = c;
    len = l;

    externed = false;
    lb_record::curAddr += t * l * cont.size();  // 修改地址
}

void lb_record::write() {
    for (int i = 0; i < times; i++) {
        for (auto c : cont) {
            writeBytes(c, len);
        }
    }
}


int Table::hasName(string name) {
    return (lb_map.find(name) != lb_map.end());
}

void Table::switchSeg() {
    if (scanLop == 1) {
        dataLen += (4 - dataLen % 4) % 4;
        obj.addShdr(curSeg, lb_record::curAddr);    // 新建一个段
        if (curSeg!=".bss") {
            dataLen += lb_record::curAddr;
        }
    }
    curSeg = id;    // 切换下一个段名
    lb_record::curAddr = 0;
}

void Table::exportSyms() {
    for (auto &pair : lb_map) {
        lb_record *lr = pair.second;
        if (!lr->isEqu) {
            obj.addSym(lr);
        }
    }
}

void Table::write() {
    for (auto defLb : defLbs) {
        defLb->write();
    }
    if (showAss) {
        // 只输出定义符号
        cout << "------------定义符号------------" << endl;
        for (auto defLb : defLbs) {
            cout << defLb->lbName << endl;
        }
    }
}

Table::~Table() {
    for (auto lb_i = lb_map.begin(); lb_i != lb_map.end(); lb_i++) {
        delete lb_i->second;
    }
    lb_map.clear();
}

void Table::addlb(lb_record *p_lb) {
    if (scanLop != 1) { // 只在第一遍添加新符号
        delete p_lb;    // 不添加
        return;
    }

    if (hasName(p_lb->lbName)) {    // 符号存在
        // 本地符号覆盖外部符号
        if (lb_map[p_lb->lbName]->externed == true && p_lb->externed == false) {
            delete lb_map[p_lb->lbName];
            lb_map[p_lb->lbName] = p_lb;
        }
        // else情况不会出现, 符号已经定义就不可能找不到该符号而产生未定义符号
    }
    else {
        lb_map[p_lb->lbName] = p_lb;
    }

    // 包含数据段内容的符号: 数据段内除了不含数据 (times==0) 的符号, 外部符号段名为空
    if (p_lb->times != 0 && p_lb->segName == ".data") {
        defLbs.emplace_back(p_lb);
    }
}

lb_record *Table::getlb(string name) {
    lb_record *ret;
    if (hasName(name)) {
        ret = lb_map[name];
    }
    else {
        // 未知符号, 添加到符号表 (仅仅添加了一次, 第一次扫描添加的)
        lb_record *p_lb = lb_map[name] = new lb_record(name, true);
        ret = p_lb;
    }

    return ret;
}


ModRM::ModRM() {
    init();
}

void ModRM::init() {
    mod = -1;
    reg = 0;
    rm = 0;
}

SIB::SIB() {
    init();
}

void SIB::init() {
    scale = -1;
    index = 0;
    base = 0;
}

Inst::Inst() {
    init();
}

void Inst::init() {
    opcode = 0;
    disp = 0;
    dispLen = 0;
    imm32 = 0;
    modrm.init();
    sib.init();
}

// 设置 disp, 自动检测disp长度 (符号), 及时是无符号地址值也无妨
void Inst::setDisp(int d, int len) {
    dispLen = len;
    disp = d;
}

// 按照记录的 disp 长度输出
void Inst::writeDisp() {
    if (dispLen) {
        writeBytes(disp, dispLen);
        dispLen = 0;    // 还原
    }
}
