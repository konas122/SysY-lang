#ifndef __LEXER_KEYWORDS__
#define __LEXER_KEYWORDS__

#include <unordered_map>

#include "common.h"
#include "lexer/scanner.h"


class Keywords
{
    std::unordered_map<std::string, Tag> keywords;

public:
    Keywords();

    const Keywords &operator=(const Keywords &&rhs) = delete;

    Tag getTag(const std::string &name);
};

#endif
