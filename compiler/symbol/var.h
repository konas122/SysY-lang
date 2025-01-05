#ifndef __SYMBOL_VAR_H__
#define __SYMBOL_VAR_H__

#include "common.h"
#include "lexer/token.h"


class Var
{
    // 特殊标记
    bool literal;               // 是否字面量, 字面量可以初始化定义的变量
    std::vector<int> scopePath; // 作用域路径

    // 基本声明形式
    bool externed;      // extern 声明或定义
    Tag type;           // 变量类型
    std::string name;   // 变量名称
    bool isPtr;         // 是否是指针
    bool isArray;       // 是否是数组
    int arraySize;      // 数组长度

    // 初始值部分
    bool isLeft;   // 是否可以作为左值
    Var *initData; // 缓存初值数据, 延迟处置处理
    bool inited;   // 是否初始化数据, 字面量一定是初始化了的特殊变量
    union
    {
        int intVal;
        char charVal;
    };
    std::string ptrVal; // 初始化字符指针常量字符串的名称
    std::string strVal; // 字符串常量的值
    Var *ptr;           // 指向当前变量指针变量

    // 附加信息
    int size;   // 变量的大小
    int offset; // 局部变量, 参数变量的栈帧偏移, 默认值 0 为无效值 (表示全局变量)

    void setExtern(bool ext);           // 设置 extern
    void setType(Tag t);                // 设置类型
    void setPtr(bool ptr);              // 设置指针
    void setName(const std::string& n); // 设置名字
    void setArray(int len);             // 设定数组
    void clear();                       // 清除关键字段信息

public:
    static Var *getStep(Var *v);
    static Var *getVoid();
    static Var *getTrue();

    Var(const std::vector<int> &sp, bool ext, Tag t, bool ptr, const std::string &name, Var *init = nullptr);   // 变量
    Var(const std::vector<int> &sp, bool ext, Tag t, const std::string &name, int len);                         // 数组
    Var(Token *lt);                                     // 设定字面量
    Var(int val);                                       // 整数变量
    Var(const std::vector<int> &sp, Tag t, bool ptr);   // 临时变量
    Var(const std::vector<int> &sp, const Var *v);      // 拷贝变量
    Var();                                              // void 变量

    bool setInit();     // 设定初始化, 由调用者决定初始化方式和顺序
    Var *getInitData(); // 获取初始化变量数据

    std::vector<int> &getPath();    // 获取 scopePath

    bool getExtern() const; // 获取 extern
    Tag getType() const;    // 获取类型
    bool isChar() const;    // 判断是否是字符变量
    bool isCharPtr() const; // 判断字符指针
    bool getPtr() const;    // 获取指针
    bool getArray() const;  // 获取数组
    std::string getName() const;    // 获取名字
    std::string getPtrVal() const;  // 获取指针变量
    std::string getRawStr() const;  // 获取原始字符串值
    std::string getStrVal() const;  // 获取字符串常量内容
    Var *getPointer();          // 获取指针
    void setPointer(Var *p);    // 设置指针变量

    void setLeft(bool lf);      // 设置变量的左值属性
    bool getLeft() const;       // 获取左值属性
    void setOffset(int off);    // 设置栈帧偏移

    int getOffset() const;  // 获取栈帧偏移
    int getSize() const;    // 获取变量大小
    void toString() const;  // 输出信息
    void value() const;     // 输出变量的中间代码形式
    bool isVoid() const;    // 是 void 唯一静态存储区变量 getVoid() 使用
    bool isBase() const;    // 是基本类型
    bool isRef() const;     // 是引用类型
    bool isLiteral() const; // 是基本类型常量 (字符串除外), 没有存储在符号表, 需要单独内存管理

    // 数据流分析接口
    int index;  // 列表索引
    bool live;  // 记录变量的活跃性
    bool unInit() const;    // 是否初始化
    bool notConst() const;  // 是否是常量
    int getVal() const;     // 获取常量值

    // 寄存器分配信息
    int regId = -1;     // 分配的寄存器编号, -1 表示在内存, 偏移地址为 offset
    bool inMem = true;  // 被取地址的变量的标记, 不分配寄存器
};

#endif
