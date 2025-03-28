#ifndef __COMPILER_DFG_H__
#define __COMPILER_DFG_H__

#include <memory>

#include "common.h"
#include "optimize/set.h"

class Block;
class InterInst;
class InterCode;


// 基本块类
class Block : public std::enable_shared_from_this<Block>
{
public:
    // 基本信息和关系
    std::list<std::shared_ptr<InterInst>> insts;    // 基本块的指令序列
    std::list<std::shared_ptr<Block>> prevs;        // 基本块的前驱序列
    std::list<std::shared_ptr<Block>> succs;        // 基本块的后继序列

    bool visited = false;   // 访问标记
    bool canReach = true;   // 块可达标记, 不可达块不作为数据流处理对象

    // 数据流分析信息
    std::vector<double> inVals;  // 常量传播输入值集合
    std::vector<double> outVals; // 常量传播输出值集合
    RedundInfo info;    // 冗余删除数据流信息
    CopyInfo copyInfo;  // 复写传播数据流信息
    LiveInfo liveInfo;  // 活跃变量数据流信息

    void toString() const;

    static std::shared_ptr<Block> create(const std::vector<std::shared_ptr<InterInst>> &codes);

    Block &operator=(const Block &&) = delete;

private:
    void init(const std::vector<std::shared_ptr<InterInst>> &codes);
};


// =============================================================================

// 数据流图类
class DFG
{
    void createBlocks();    // 创建基本块
    void linkBlocks();      // 链接基本块

    void resetVisit();              // 重置访问标记
    bool reachable(std::shared_ptr<Block> block);   // 测试块是否可达
    void release(std::shared_ptr<Block> block);     // 如果块不可达, 则删除所有后继, 并继续处理所有后继

public:
    std::vector<std::shared_ptr<InterInst>> &codeList;  // 中间代码序列
    std::vector<std::shared_ptr<Block>> blocks;         // 流图的所有基本块
    std::vector<std::shared_ptr<InterInst>> newCode;    // 优化生成的中间代码序列, 内存管理

    explicit DFG(InterCode &code);
    ~DFG();
    void delLink(std::shared_ptr<Block> begin, std::shared_ptr<Block> end); // 删除块间联系, 如果块不可达, 则删除所有后继联系

    // 核心实现
    void toCode(std::list<std::shared_ptr<InterInst>> &opt);    // 导出数据流图为中间代码

    void toString() const;
};

#endif
