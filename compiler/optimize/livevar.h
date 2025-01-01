#ifndef __OPTIMIZE_LIVEVAR_H__
#define __OPTIMIZE_LIVEVAR_H__


class LiveVar
{
public:
    void analyse();
    void elimateDeadCode(int stop);
};

#endif
