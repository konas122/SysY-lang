#include "error.h"
#include "lexer.h"

using namespace std;

#define LEXERROR(code) Error::lexError(code)


Keywords Lexer::keywords;

Lexer::Lexer(Scanner &sc) : scanner(sc) {
    token = nullptr;
    ch = ' ';
}

Lexer::~Lexer()
{}

bool Lexer::scan(char need) {
    ch = scanner.scan();
    if (need) {
        if (ch != need) {
            return false;
        }
        ch = scanner.scan();
        return true;
    }
    return true;
}

unique_ptr<Token> Lexer::tokenize() {
    while (ch != -1) {
        unique_ptr<Token> t;

        while (ch == ' ' || ch == '\n' || ch == '\t') {
            scan();
        }

        // IDENT & Keywords
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_') {
            string name = "";
            do {
                name += ch;
                scan();
            } while ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' || (ch >= '0' && ch <= '9'));

            Tag tag = keywords.getTag(name);
            if (tag == Tag::ID) {
                t = make_unique<Id>(name);
            }
            else {
                t = make_unique<Token>(tag);
            }
        }
        // STR
        else if (ch == '"') {
            string str = "";
            while (!scan('"')) {
                if (ch == '\\') {
                    scan();
                    if (ch == 'n') {
                        str += '\n';
                    }
                    else if (ch == '\\') {
                        str += '\\';
                    }
                    else if (ch == 't') {
                        str += '\t';
                    }
                    else if (ch == '"') {
                        str += '"';
                    }
                    else if (ch == '0') {
                        str += '\0';
                    }
                    else if (ch == '\n');  // do nothing
                    else if (ch == -1){
                        LEXERROR(cast_int(LexError::STR_NO_R_QUTION));
                        t = make_unique<Token>(Tag::ERR);
                        break;
                    }
                    else {
                        str += ch;
                    }
                }
                else if (ch == '\n' || ch == -1) {
                    LEXERROR(cast_int(LexError::STR_NO_R_QUTION));
                    t = make_unique<Token>(Tag::ERR);
                    break;
                }
                else {
                    str += ch;
                }
            }
            if (!t) {
                t = make_unique<Str>(str);
            }
        }
        // NUM
        else if (ch >= '0' && ch <= '9') {
            int val = 0;
            if (ch != '0') {        // DEC
                do{
                    val = val * 10 + ch - '0';
                    scan();
                } while (ch >= '0' && ch <= '9');
            }
            else {
                scan();
                if (ch == 'x' || ch == 'X') {    // HEX
                    scan();
                    if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f')) {
                        do {
                            val = val * 16 + ch;
                            if (ch >= '0' && ch <= '9') {
                                val -= '0';
                            }
                            else if (ch >= 'A' && ch <= 'F') {
                                val += 10 - 'A';
                            }
                            else if (ch >= 'a' && ch <= 'f') {
                                val += 10 - 'a';
                            }
                            scan();
                        } while ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f'));
                    }
                    else {
                        LEXERROR(cast_int(LexError::NUM_HEX_TYPE)); // 0x 后无数据
                        t = make_unique<Token>(Tag::ERR);
                    }
                }
                else if (ch == 'b' || ch == 'B') {   // BIN
                    scan();
                    if (ch >= '0' && ch <= '1') {
                        do {
                            val = val * 2 + ch - '0';
                            scan();
                        } while (ch >= '0' && ch <= '1');
                    }
                    else {
                        LEXERROR(cast_int(LexError::NUM_BIN_TYPE));
                        t = make_unique<Token>(Tag::ERR);
                    }
                }
                else if (ch >= '0' && ch <= '7') {  // OCT
                    do {
                        val = val * 8 + ch - '0';
                        scan();
                    } while (ch >= '0' && ch <= '7');
                }
            }
            if (!t) {
                t = make_unique<Num>(val);
            }
        }
        // CHAR
        else if (ch == '\'') {
            char c;
            scan();
            if (ch == '\\') {   // 转义
                scan();
                if (ch == 'n') {
                    c = '\n';
                }
                else if (ch == '\\') {
                    c = '\\';
                }
                else if (ch == 't') {
                    c = '\t';
                }
                else if (ch == '0') {
                    c = '\0';
                }
                else if (ch == '\'') {
                    c = '\'';
                }
                else if (ch == -1 || ch == '\n') {  // 文件结束 换行
                    LEXERROR(cast_int(LexError::CHAR_NO_R_QUTION));
                    t = make_unique<Token>(Tag::ERR);
                }
                else {
                    c = ch; // 没有转义
                }
            }
            else if (ch == '\n' || ch == -1) {
                LEXERROR(cast_int(LexError::CHAR_NO_R_QUTION));
                t = make_unique<Token>(Tag::ERR);
            }
            else if (ch == '\'') {
                LEXERROR(cast_int(LexError::CHAR_NO_DATA));
                t = make_unique<Token>(Tag::ERR);
                scan(); // 读掉引号
            }
            else {
                c = ch; // 正常字符
            }
            if (!t) {
                if (scan('\'')) {   // 匹配右侧引号, 读掉引号
                    t = make_unique<Char>(c);
                }
                else {
                    LEXERROR(cast_int(LexError::CHAR_NO_R_QUTION));
                    t = make_unique<Token>(Tag::ERR);
                }
            }
        }
        else {
            switch(ch) {
            case '#':   // 忽略行 (忽略宏定义)
                while (ch != '\n' && ch != -1) {
                    scan();
                }
                t = make_unique<Token>(Tag::ERR);
                break;
            case '+':
                t = make_unique<Token>(scan('+') ? Tag::INC : Tag::ADD);
                break;
            case '-':
                t = make_unique<Token>(scan('-') ? Tag::DEC : Tag::SUB);
                break;
            case '*':
                t = make_unique<Token>(Tag::MUL);
                scan();
                break;
            case '/':
                scan();
                if (ch == '/') {        // 单行注释
                    while (ch != '\n' && ch != -1) {
                        scan();
                    }
                    t = make_unique<Token>(Tag::ERR);
                }
                else if (ch == '*') {   // 多行注释
                    while (!scan(-1)) {
                        if (ch == '*') {
                            if (scan('/')) {
                                break;
                            }
                        }
                    }
                    if (ch == -1) {     // 没正常结束注释
                        LEXERROR(cast_int(LexError::COMMENT_NO_END));
                    }
                    t = make_unique<Token>(Tag::ERR);
                }
                else {
                    t = make_unique<Token>(Tag::DIV);
                }
                break;
            case '%':
                t = make_unique<Token>(Tag::MOD);
                scan();
                break;
            case '>':
                t = make_unique<Token>(scan('=') ? Tag::GE : Tag::GT);
                break;
            case '<':
                t = make_unique<Token>(scan('=') ? Tag::LE : Tag::LT);
                break;
            case '=':
                t = make_unique<Token>(scan('=') ? Tag::EQU : Tag::ASSIGN);
                break;
            case '&':
                t = make_unique<Token>(scan('&') ? Tag::AND : Tag::LEA);
                break;
            case '|':
                t = make_unique<Token>(scan('|') ? Tag::OR : Tag::ERR);
                if (t->tag == Tag::ERR) {
                    LEXERROR(cast_int(LexError::OR_NO_PAIR));
                }
                break;
            case '!':
                t = make_unique<Token>(scan('=') ? Tag::NEQU : Tag::NOT);
                break;
            case ',':
                t = make_unique<Token>(Tag::COMMA);
                scan();
                break;
            case ':':
                t = make_unique<Token>(Tag::COLON);
                scan();
                break;
            case ';':
                t = make_unique<Token>(Tag::SEMICON);
                scan();
                break;
            case '(':
                t = make_unique<Token>(Tag::LPAREN);
                scan();
                break;
            case ')':
                t = make_unique<Token>(Tag::RPAREN);
                scan();
                break;
            case '[':
                t = make_unique<Token>(Tag::LBRACK);
                scan();
                break;
            case ']':
                t = make_unique<Token>(Tag::RBRACK);
                scan();
                break;
            case '{':
                t = make_unique<Token>(Tag::LBRACE);
                scan();
                break;
            case '}':
                t = make_unique<Token>(Tag::RBRACE);
                scan();
                break;
            case -1:
                scan();
                break;
            default:
                t = make_unique<Token>(Tag::ERR);
                LEXERROR(cast_int(LexError::TOKEN_NO_EXIST));
                scan();
            }
        }

        token = std::move(t);
        if (token && token->tag != Tag::ERR) {
            return std::move(token);
        }
        else {
            continue;
        }
    }

    // EOF
    token = make_unique<Token>(Tag::END);
    return std::move(token);
}
