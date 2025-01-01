#ifndef __COMPILER_PLATFORM_H__
#define __COMPILER_PLATFORM_H__


class Plat
{
public:
    static const int regNum = 8;
    static const char *regName[regNum];
    static const int stackBase = 12;
    static const int stackBase_protect = 12 + regNum * 4;
};

#endif
