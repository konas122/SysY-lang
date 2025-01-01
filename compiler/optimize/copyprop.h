#ifndef __OPTIMIZE_COPYPROP_H__
#define __OPTIMIZE_COPYPROP_H__

class DFG;


// 复写传播数据流分析框架
class CopyPropagation
{
public:
    CopyPropagation(DFG *g);
    void propagate();   // 执行复写传播
};

#endif
