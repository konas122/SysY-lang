#include "lexer/keywords.h"

using namespace std;


Keywords::Keywords() {
    keywords["int"] = Tag::KW_INT;
    keywords["char"] = Tag::KW_CHAR;
    keywords["void"] = Tag::KW_VOID;
    keywords["extern"] = Tag::KW_EXTERN;
    keywords["if"] = Tag::KW_IF;
    keywords["else"] = Tag::KW_ELSE;
    keywords["switch"] = Tag::KW_SWITCH;
    keywords["case"] = Tag::KW_CASE;
    keywords["default"] = Tag::KW_DEFAULT;
    keywords["while"] = Tag::KW_WHILE;
    keywords["do"] = Tag::KW_DO;
    keywords["for"] = Tag::KW_FOR;
    keywords["break"] = Tag::KW_BREAK;
    keywords["continue"] = Tag::KW_CONTINUE;
    keywords["return"] = Tag::KW_RETURN;
}

Tag Keywords::getTag(const string& name) {
    return keywords.count(name) ? keywords[name] : Tag::ID;
}
