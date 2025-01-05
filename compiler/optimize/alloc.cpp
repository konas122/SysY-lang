#include <algorithm>

#include "platform.h"
#include "symbol/var.h"
#include "symbol/fun.h"
#include "ir/interInst.h"

#include "optimize/livevar.h"

#include "alloc.h"

using namespace std;


Node::Node(Var *v, const Set &E) : var(v)
{
    exColors = E;
}

void Node::addLink(Node *node) {
    auto pos = lower_bound(links.begin(), links.end(), node);
    if (pos == links.end() || *pos != node) {
        links.insert(pos, node);
        degree++;
    }
}

void Node::addExColor(int color) {
    if (degree == -1) {
        return;          // 已经着色节点, 不再处理
    }
    exColors.set(color); // 添加排除色
    degree--;            // 度减少
}

void Node::paint(const Set &colorBox) {
    Set availColors = colorBox - exColors;  // 可用的颜色集合
    for (int i = 0; i < availColors.count; ++i) {
        if (availColors.get(i)) {
            color = i;
            var->regId = color;
            degree = -1;
            for (size_t j = 0; j < links.size(); ++j) {
                links[j]->addExColor(color);
            }
            return;
        }
    }
    degree = -1;    // 着色失败
}

// =============================================================================

Scope::Scope(int i, int addr) : id(i), esp(addr), x(0), y(0), parent(nullptr) {}

Scope::~Scope() {
    for (const auto child : children) {
        delete child;
    }
}

// 查找 i 作用域, 不存在则产生一个子作用域, id=i, esp 继承当前父 esp
Scope *Scope::find(int i) {
    // 二分查找
    Scope *sc = new Scope(i, esp);  // 先创建子作用域, 拷贝父 esp
    auto pos = lower_bound(children.begin(), children.end(), sc, scope_less());

    if (pos == children.end() || (*pos)->id != i) { // 没有找到, 则插入
        children.insert(pos, sc);   // 有序插入
        sc->parent = this;          // 记录父节点
    }
    else {
        delete sc;  // 删除查找对象
        sc = *pos;  // 找到了
    }
    return sc;
}

// =============================================================================

CoGraph::CoGraph(list<InterInst *> &optCode, vector<Var *> &para, LiveVar *lv, Fun *f) {
    scRoot = nullptr;

    fun = f;
    this->optCode = optCode;
    this->lv = lv;

    U.init(Plat::regNum, 1);
    E.init(Plat::regNum, 0);

    // 函数参数变量
    for (auto var : para) {
        varList.emplace_back(var);
    }

    // 所有的局部变量
    for (auto inst : optCode) {
        Operator op = inst->getOp();
        if (op == Operator::OP_DEC) {
            Var *arg1 = inst->getArg1();
            varList.emplace_back(arg1);
        }
        if (op == Operator::OP_LEA) {
            Var *arg1 = inst->getArg1();
            if (arg1) {
                arg1->inMem = true; // 只能放在内存
            }
        }
    }

    const Set &liveE = lv->getE();  // 活跃变量分析时的空集合
    Set mask = liveE;
    for (size_t i = 0; i < varList.size(); ++i) {
        mask.set(varList[i]->index);
    }

    // 构建图节点
    for (size_t i = 0; i < varList.size(); ++i) {
        Node *node;
        auto var = varList[i];
        if (var->getArray() || var->inMem) {
            node = new Node(var, U);
        }
        else {
            node = new Node(var, E);
        }
        var->index = i;
        nodes.emplace_back(node);
    }

    // 构建冲突边
    Set buf = liveE;
    for (auto i = optCode.rbegin(); i != optCode.rend(); ++i) {
        Set &liveout = (*i)->liveInfo.out;
        if (liveout != buf) {   // 新的冲突关系
            buf = liveout;
            vector<Var *> coVar = lv->getCoVar(liveout & mask);
            for (size_t j = 0; j < coVar.size() - 1; ++j) {
                for (size_t k = j + 1; k < coVar.size(); ++k) {
                    nodes[coVar[j]->index]->addLink(nodes[coVar[k]->index]);    // 添加关系, 不会重复添加
                    nodes[coVar[k]->index]->addLink(nodes[coVar[j]->index]);    // 相互添加关系
                }
            }
        }
    }
}

CoGraph::~CoGraph() {
    for (auto node : nodes) {
        delete node;
    }
    delete scRoot;
}

// 选择度最大的未处理节点, 利用最大堆根据节点度堆排序
Node *CoGraph::pickNode() {
    make_heap(nodes.begin(), nodes.end(), node_less());
    Node *node = nodes.front();
    return node;
}

// 寄存器分配图着色算法, 将 regNum 个寄存器着色到图上
void CoGraph::regAlloc() {
    Set colorBox = U;
    int nodeNum = nodes.size();
    for (auto i = 0; i < nodeNum; ++i) {
        Node *node = pickNode();
        node->paint(colorBox);
    }
}

void CoGraph::printTree(Scope *root, bool tree_style) const {
    if (!tree_style) {
        printf("( <%d>: %d ", root->id, root->esp);
        for (const auto child : root->children) {
            printTree(child, false);
        }
        printf(") ");
    }
    else {                          // 树形打印
        int y = 0;                  // 从 0 行开始
        __printTree(root, 0, 0, y); // 树形打印
    }
}

void CoGraph::__printTree(Scope *root, int blk, int x, int &y) const {
    // 记录打印位置
    root->x = x;
    root->y = y;

    // 填充不连续的列
    if (root->parent) {
        vector<Scope *> &brother = root->parent->children;  // 兄弟节点
        vector<Scope *>::const_iterator pos;
        pos = lower_bound(brother.begin(), brother.end(), root, Scope::scope_less());   // 查找位置, 一定存在
        if (pos != brother.begin()) {           // 不是第一个兄弟
            const Scope *prev = (*--pos);       // 前一个兄弟
            int disp = root->y - prev->y - 1;   // 求差值
            printf("\033[s");                   // 保存光标位置

            while (disp--) {
                printf("\033[1A");  // 上移
                printf("|");        // 打印 |
                printf("\033[1D");  // 左移回复光标位置
            }
            printf("\033[u");       // 恢复光标位置
        }
    }

    printf("|——\033[33m<%d>:%d\033[0m", root->id, root->esp);
    printf("\n");
    x += (blk + 1) * 4; // 计算空的列个数
    for (const auto child : root->children) {
        ++y;            // 同级节点累加行
        int t = blk;
        while (t--) {
            printf("    ");
        }
        printf("    ");
        __printTree(child, blk + 1, x, y);
    }
}

int &CoGraph::getEsp(const vector<int> &path) {
    Scope *scope = scRoot;  // 当前作用域初始化为全局作用域
    for (size_t i = 1; i < path.size(); i++) {
        scope = scope->find(path[i]); // 向下寻找作用域, 没有会自动创建
    }
    return scope->esp;
}

// 为不能着色的变量分配栈帧地址
void CoGraph::stackAlloc() {
    // 初始化作用域序列
    scRoot = new Scope(0, 0);
    int max = 0;

    for (auto inst : optCode) {
        Operator op = inst->getOp();
        if (op == Operator::OP_DEC) {
            Var *arg1 = inst->getArg1();
            if (arg1->regId != -1) {
                continue;
            }

            // 没有分配到寄存器, 计算栈帧地址
            int &esp = getEsp(arg1->getPath());
            int size = arg1->getSize();
            size += (4 - size % 4) % 4; // 按照 4 字节的大小整数倍分配局部变量
            esp += size;
            arg1->setOffset(-esp);

            max = esp > max ? esp : max;
        }
    }

    // 设置函数的最大栈帧深度
    fun->setMaxDep(max);
}

// 分配算法
void CoGraph::alloc() {
    regAlloc();     // 寄存器分配
    stackAlloc();   // 栈帧地址分配
}
