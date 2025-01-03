#include <cassert>

#include "set.h"

using namespace std;


Set::Set() {
    count = 0;
}

Set::Set(int size, bool val) {
    init(size, val);
}

void Set::init(int size, bool val) {
    count = size;
    size = size / 32 + (size % 32 != 0);
    uint32_t v = val ? 0xffffffff : 0;
    for (int i = 0; i < size; ++i) {
        bmList.emplace_back(v);
    }
}

Set Set::operator&(const Set &val) {
    assert(bmList.size() == val.bmList.size());

    Set ret(count, 0);
    for (size_t i = 0; i < bmList.size(); ++i) {
        ret.bmList[i] = bmList[i] & val.bmList[i];
    }
    return ret;
}

Set Set::operator|(const Set &val) {
    assert(bmList.size() == val.bmList.size());

    Set ret(count, 0);
    for (size_t i = 0; i < bmList.size(); i++) {
        ret.bmList[i] = bmList[i] | val.bmList[i];
    }
    return ret;
}

Set Set::operator-(const Set &val) {
    assert(bmList.size() == val.bmList.size());

    Set ret(count, 0);
    for (size_t i = 0; i < bmList.size(); i++) {
        ret.bmList[i] = bmList[i] & ~val.bmList[i];
    }
    return ret;
}

Set Set::operator^(const Set &val) {
    assert(bmList.size() == val.bmList.size());

    Set ret(count, 0);
    for (size_t i = 0; i < bmList.size(); i++) {
        ret.bmList[i] = bmList[i] ^ val.bmList[i];
    }
    return ret;
}

Set Set::operator~() {
    Set ret(count, 0);
    for (size_t i = 0; i < bmList.size(); i++) {
        ret.bmList[i] = ~bmList[i];
    }
    return ret;
}

bool Set::operator==(const Set &val) {
    if (count != val.count) {
        return false;
    }
    for (size_t i = 0; i < bmList.size(); i++) {
        if (bmList[i] != val.bmList[i])
            return false;
    }
    return true;
}

bool Set::operator!=(const Set &val) {
    if (count != val.count) {
        return true;
    }
    for (size_t i = 0; i < bmList.size(); i++) {
        if (bmList[i] != val.bmList[i])
            return true;
    }
    return false;
}

bool Set::get(int i) const {
    if (i < 0 || i >= count) {
        return false;
    }
    return !!(bmList[i / 32] & (1 << (i % 32)));
}

void Set::set(int i) {
    if (i < 0 || i >= count) {
        return;
    }
    bmList[i / 32] |= (1 << (i % 32));
}

void Set::reset(int i) {
    if (i < 0 || i >= count) {
        return;
    }
    bmList[i / 32] &= ~(1 << (i % 32));
}

int Set::max() {
    for (int i = bmList.size() - 1; i >= 0; --i) {
        uint32_t n = 0x80000000;
        int index = 31;
        while (n) {
            if (bmList[i] & n)
                break;
            --index;
            n >>= 1;
        }
        if (index >= 0) {
            return i * 32 + index;
        }
    }
    return -1;
}

void Set::p() {
    int num = count;    // 计数器

    // if (bmList.size() == 0) {
    //     while (num--) {
    //         printf("- ");
    //     }
    // }

    for (size_t i = 0; i < bmList.size(); ++i) {
        uint32_t n = 0x1;
        while (n) {
            printf("%d ", !!(bmList[i] & n));
            if (!--num) {
                break;
            }
            n <<= 1;
        }
        if (!--num) {
            break;
        }
    }
    fflush(stdout);
}
