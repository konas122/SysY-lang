#ifndef __OPTIMIZE_ALLOC_H__
#define __OPTIMIZE_ALLOC_H__

#include <memory>

#include "common.h"
#include "optimize/set.h"

class Var;
class Fun;
class LiveVar;
class InterInst;


struct Node
{
    std::shared_ptr<Var> var = nullptr;         // 节点对应的变量
    int degree = 0;             // 节点的度, 跟踪节点边的个数, 为图着色算法提供选择依据
    int color = -1;             // 节点颜色, 与 var->regId 对应, -1 为无效值 (无法/不能着色)
    Set exColors;               // 节点排斥的颜色, 避免相邻节点着同色. 其补集中第一个有效元素为 color
    std::vector<std::weak_ptr<Node>> links;  // 节点的连接边, 按照有序插入, 使用二分查找是否存在已有邻居

    Node(std::shared_ptr<Var> v, const Set &E);         // 创建 v 对应的节点, 初始化
    void addLink(std::weak_ptr<Node> node);           // 添加一条边, 有序插入, 度 +1. 保证二分查找
    void addExColor(int color);         // 添加一个排除色
    void paint(const Set &colorBox);    // 根据 exColors 选择第一个有效的颜色, 度 =-1
};


// =============================================================================

struct Scope : std::enable_shared_from_this<Scope>
{
    struct scope_less
    {
        bool operator()(const std::shared_ptr<Scope> left, const std::shared_ptr<Scope> right) {
            return left->id < right->id;    // 按照id确定节点大小
        }
    };

    int id;
    int esp;
    std::vector<std::shared_ptr<Scope>> children;  // 子作用域

    Scope(int i, int addr);
    ~Scope();

    std::shared_ptr<Scope> find(int i); // 查找 i 作用域, 不存在则产生一个子作用域, id=i, esp 继承当前父 esp
    // 输出信息控制, |--开始位置
    int x;          // 列位置, 实际未用到
    int y;          // 行位置
    std::weak_ptr<Scope> parent;  // 记录父亲节点
};


// =============================================================================

class CoGraph
{
    struct node_less
    {
        bool operator()(const std::shared_ptr<Node> left, const std::shared_ptr<Node> right) const {
            return left->degree <= right->degree;
        }
    };

    std::list<std::shared_ptr<InterInst>> optCode;  // 中间代码
    std::shared_ptr<LiveVar> lv;                    // 活跃变量分析对象, 使用该对象提供集合对应的变量信息
    std::vector<std::shared_ptr<Node>> nodes;      // 图节点数组, 图着色选择最大度节点时, 使用堆排序
    std::vector<std::shared_ptr<Var>> varList;     // 变量列表, 缓存所有需要分配的变量

    Set U;          // 颜色集合全集
    Set E;          // 颜色集合空集
    std::shared_ptr<Scope> scRoot;  // 根作用域指针
    std::shared_ptr<Fun> fun;       // 函数对象

    std::shared_ptr<Node> pickNode();                       // 选择度最大的未处理节点, 利用最大堆根据节点度堆排序
    void regAlloc();                                        // 寄存器分配图着色算法, 将 regNum 个寄存器着色到图上
    int &getEsp(const std::vector<int> &path);              // 根据当前变量的作用域路径获取栈帧偏移地址
    void stackAlloc();                                      // 为不能着色的变量分配栈帧地址

    void __printTree(std::shared_ptr<Scope> root, int blk, int x, int &y) const;    // 树形输出作用域
    void printTree(std::shared_ptr<Scope> root, bool tree_style = true) const;      // 封装的函数

public:
    const CoGraph &operator=(const CoGraph &&rhs) = delete;

    CoGraph(std::list<std::shared_ptr<InterInst>> &optCode, std::vector<std::shared_ptr<Var>> &para, std::shared_ptr<LiveVar> lv, std::shared_ptr<Fun> f);
    ~CoGraph();
    void alloc();
};

#endif
