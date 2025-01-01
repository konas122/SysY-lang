#include <sstream>

#include "error.h"
#include "ir/genir.h"
#include "symbol/symtab.h"

#include "var.h"

using namespace std;

#define SEMERROR(code, name) Error::semError(code, name)


Var *Var::getVoid() {
    return SymTab::voidVar;
}

Var::Var() {
    clear();
    setName("<void>");
    setLeft(false);
    intVal = 0;
    literal = false;    // 消除字面量标志
    type = Tag::KW_VOID;
    isPtr = true;       // 消除基本类型标志
}

Var *Var::getTrue() {
    return SymTab::one;
}

Var *Var::getStep(Var *v) {
    if (v->isBase()) {
        return SymTab::one;
    }
    else if (v->type == Tag::KW_CHAR) {
        return SymTab::one;
    }
    else if (v->type == Tag::KW_INT) {
        return SymTab::four;
    }
    return nullptr;
}

void Var::clear() {
    scopePath.emplace_back(-1); // 默认全局作用域
    externed = false;
    isPtr = false;
    isArray = false;
    isLeft = true;  // 默认变量是可以作为左值的
    inited = false;
    literal = false;
    size = 0;
    offset = 0;
    ptr = nullptr;  // 没有指向当前变量的指针变量
    index = -1;     // 无效索引
    initData = nullptr;
    live = false;
    regId = -1;     // 默认放在内存
    inMem = false;
}


// 临时变量
Var::Var(const vector<int> &sp, Tag t, bool ptr) {
    clear();
    scopePath = sp;
    setType(t);
    setPtr(ptr);
    setName("");
    setLeft(false);
}

// 拷贝出一个临时变量
Var::Var(const vector<int> &sp, Var *v) {
    clear();
    scopePath = sp;
    setType(v->type);
    setPtr(v->isPtr || v->isArray);
    setName("");
    setLeft(false);
}

// 变量, 指针
Var::Var(const vector<int>& sp, bool ext, Tag t, bool ptr, const string &name, Var *init) {
    clear();
    scopePath = sp;
    setExtern(ext);
    setType(t);
    setPtr(ptr);
    setName(name);
    initData = init;
}

// 数组
Var::Var(const vector<int> &sp, bool ext, Tag t, const string &name, int len) {
    clear();
    scopePath = sp;
    setExtern(ext);
    setType(t);
    setName(name);
    setArray(len);
}

// 整数变量
Var::Var(int val) {
    clear();
    setName("<int>");
    literal = true;
    setLeft(false);
    setType(Tag::KW_INT);
    intVal = val;
}

// 常量, 不涉及作用域的变化, 字符串存储在字符串表, 其他常量作为初始值 (使用完删除)
Var::Var(Token *lt) {
    clear();
    literal = true;
    setLeft(false);
    switch (lt->tag) {
    case Tag::NUM:
        setType(Tag::KW_INT);
        name = "<int>";
        intVal = static_cast<Num *>(lt)->val;
        break;
    case Tag::CH:
        setType(Tag::KW_CHAR);
        name = "<char>";
        intVal = 0;
        charVal = static_cast<Char *>(lt)->ch;
        break;
    case Tag::STR:
        setType(Tag::KW_CHAR);
        name = GenIR::genLb();
        strVal = static_cast<Str *>(lt)->str;
        setArray(strVal.size() + 1);
        break;

    default:
        break;
    }
}

void Var::setExtern(bool ext) {
    externed = ext;
    size = 0;
}

void Var::setType(Tag t) {
    type = t;
    if (type == Tag::KW_VOID) {
        SEMERROR(static_cast<int>(SemError::VOID_VAR), "");
        type = Tag::KW_INT;
    }
    if (!externed && type == Tag::KW_INT) {
        size = 4;
    }
    else if (!externed && type == Tag::KW_CHAR) {
        size = 1;
    }
}

void Var::setPtr(bool ptr) {
    if (!ptr) {
        return;
    }
    isPtr = true;
    if (!externed) {
        size = 4;
    }
}

void Var::setName(const string &n) {
    if (n == "") {
        name = GenIR::genLb();
        return;
    }
    name = n;
}

void Var::setArray(int len) {
    if (len <= 0) {
        SEMERROR(static_cast<int>(SemError::ARRAY_LEN_INVALID), name);
        return;
    }
    isArray = true;
    isLeft = false;
    arraySize = len;
    if (!externed) {
        size *= len;
    }
}

bool Var::setInit() {
    Var *init = initData;
    if (!init) {
        return false;           // 没有初始化表达式
    }
    inited = false;             // 默认初始化失败
    if (externed) {             // 声明不允许初始化
        SEMERROR(static_cast<int>(SemError::DEC_INIT_DENY), name);
    }
    else if (init->literal) {   // 使用常量初始化
        inited = true;          // 初始化成功
        if (init->isArray) {    // 初始化常量是数组, 必是字符串
            ptrVal = init->name;
        }
        else {                  // 基本类型, 拷贝数值数据
            intVal = init->intVal;
        }
    }
    else {                      // 初始值不是常量
        if (scopePath.size() == 1) {
            SEMERROR(static_cast<int>(SemError::GLB_INIT_ERR), name);
        }
        else {
            return true;        // 被初始化变量是局部变量
        }
    }
    // // 删除整数, 字符初始化常量
    // if (init->literal && !(init->isArray)) {
    //     delete init;            // 清除临时的字面变量, 字符串和临时变量不清除 (已经在符号表内)
    // }
    return false;
}

Var *Var::getInitData() {
    return initData;
}

bool Var::getExtern() {
    return externed;
}

vector<int>& Var::getPath() {
    return scopePath;
}

Tag Var::getType() {
    return type;
}

bool Var::isChar() {
    return (type == Tag::KW_CHAR) && isBase();
}

bool Var::isCharPtr() {
    return (type == Tag::KW_CHAR) && !isBase();
}

bool Var::getPtr() {
    return isPtr;
}

string Var::getName() {
    return name;
}

bool Var::getArray() {
    return isArray;
}

void Var::setPointer(Var *p) {
    ptr = p;
}

Var *Var::getPointer() {
    return ptr;
}

// 获取字符指针内容
string Var::getPtrVal() {
    return ptrVal;
}

// 获取字符串常量内容
string Var::getStrVal() {
    return strVal;
}

// 获取字符串常量原始内容, 将特殊字符转义
string Var::getRawStr() {
    stringstream ss;
    for (size_t i = 0, chpass = 0; i < strVal.size(); ++i) {
        auto ch = strVal[i];
        if (ch == '\n' || ch == '\t' || ch == '\"' || ch == '\0') {
            if (chpass == 0) {
                ss << static_cast<int>(ch) << ",";
            }
            else {
                ss << "\"," << static_cast<int>(ch) << ",";
            }
            chpass = 0;
        }
        else {
            if (chpass == 0) {
                ss << "\"" << static_cast<int>(ch);
            }
            else {
                ss << static_cast<int>(ch);
            }
            if (i == strVal.size() - 1) {
                ss << "\",";
            }
            chpass = 1;
        }
    }
    ss << "0";
    return ss.str();
}

void Var::setLeft(bool lf) {
    isLeft = lf;
}

bool Var::getLeft() {
    return isLeft;
}

// 设置栈帧偏移
void Var::setOffset(int off) {
    offset = off;
}

int Var::getOffset() {
    return offset;
}

int Var::getSize() {
    return size;
}

bool Var::isVoid() {
    return type == Tag::KW_VOID;
}

bool Var::isBase() {
    return !isArray && !isPtr;
}

bool Var::isRef() {
    return !!ptr;
}

bool Var::unInit() {
    return !inited;
}

bool Var::notConst() {
    return !literal;
}

int Var::getVal() {
    return intVal;
}

// 是基本类型常量 (字符串除外), 没有存储在符号表, 需要单独内存管理
bool Var::isLiteral() {
    return this->literal && isBase();
}

// 输出变量的中间代码形式
void Var::value() {
    if (literal) {  // 是字面量
        if (type == Tag::KW_INT) {
            printf("%d", intVal);
        }
        else if (type == Tag::KW_CHAR) {
            if (isArray) {
                printf("%s", name.c_str());
            }
            else {
                printf("%d", charVal);
            }
        }
    }
    else {
        printf("%s", name.c_str());
    }
}

void Var::toString() {
    if (externed) {
        printf("externed ");
    }
    printf("%s", tokenName[static_cast<int>(type)]);
    if (isPtr) {
        printf("*");
    }
    printf(" %s", name.c_str());
    if (isArray) {
        printf("[%d]", arraySize);
    }
    if (inited) {
        printf(" = ");
        switch (type) {
        case Tag::KW_INT:
            printf("%d", intVal);
            break;
        case Tag::KW_CHAR:
            if (isPtr) {
                printf("<%s>", ptrVal.c_str());
            }
            else {
                printf("%c", charVal);
            }
            break;

        default:
            break;
        }
    }

    printf("; size=%d scope=\"", size);
    for (size_t i = 0; i < scopePath.size(); i++) {
        printf("/%d", scopePath[i]);
    }
    printf("\" ");

    if (offset > 0) {
        printf("addr=[ebp+%d]", offset);
    }
    else if (offset < 0) {
        printf("addr=[ebp%d]", offset);
    }
    else if (name[0] != '<') {
        printf("addr=<%s>", name.c_str());
    }
    else {
        printf("value='%d'", getVal());
    }
}
