#include "error.h"

#include "lexer.h"

using namespace std;

#define LEXERROR(code) Error::lexError(code)


Keywords Lexer::keywords;

Lexer::Lexer(Scanner &sc) : scanner(sc) {
    token = nullptr;
    ch = ' ';
}

Lexer::~Lexer() {
    if (!token) {
        delete token;
    }
}

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

Token *Lexer::tokenize() {
    while (ch != -1) {
        Token *t = nullptr;

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
                t = new Id(name);
            }
            else {
                t = new Token(tag);
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
                        LEXERROR(static_cast<int>(LexError::STR_NO_R_QUTION));
                        t = new Token(Tag::ERR);
                        break;
                    }
                    else {
                        str += ch;
                    }
                }
                else if (ch == '\n' || ch == -1) {
                    LEXERROR(static_cast<int>(LexError::STR_NO_R_QUTION));
                    t = new Token(Tag::ERR);
                    break;
                }
                else {
                    str += ch;
                }
            }
            if (!t) {
                t = new Str(str);
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
                if (ch == 'x') {    // HEX
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
                        LEXERROR(static_cast<int>(LexError::NUM_HEX_TYPE)); // 0x 后无数据
                        t = new Token(Tag::ERR);
                    }
                }
                else if (ch == 'b') {   // BIN
                    scan();
                    if (ch >= '0' && ch <= '1') {
                        do {
                            val = val * 2 + ch - '0';
                            scan();
                        } while (ch >= '0' && ch <= '1');
                    }
                    else {
                        LEXERROR(static_cast<int>(LexError::NUM_BIN_TYPE));
                        t = new Token(Tag::ERR);
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
                t = new Num(val);
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
                    LEXERROR(static_cast<int>(LexError::CHAR_NO_R_QUTION));
                    t=new Token(Tag::ERR);
                }
                else {
                    c = ch; // 没有转义
                }
            }
            else if (ch == '\n' || ch == -1) {
                LEXERROR(static_cast<int>(LexError::CHAR_NO_R_QUTION));
                t = new Token(Tag::ERR);
            }
            else if (ch == '\'') {
                LEXERROR(static_cast<int>(LexError::CHAR_NO_DATA));
                t = new Token(Tag::ERR);
                scan(); // 读掉引号
            }
            else {
                c = ch; // 正常字符
            }
            if (!t) {
                if (scan('\'')) {   // 匹配右侧引号, 读掉引号
                    t=new Char(c);
                }
                else {
                    LEXERROR(static_cast<int>(LexError::CHAR_NO_R_QUTION));
                    t = new Token(Tag::ERR);
                }
            }
        }
        else {
            switch(ch) {
            case '#':   // 忽略行 (忽略宏定义)
                while (ch != '\n' && ch != -1) {
                    scan();
                }
                t = new Token(Tag::ERR);
                break;
            case '+':
                t = new Token(scan('+') ? Tag::INC : Tag::ADD);
                break;
            case '-':
                t = new Token(scan('-') ? Tag::DEC : Tag::SUB);
                break;
            case '*':
                t = new Token(Tag::MUL);
                scan();
                break;
            case '/':
                scan();
                if (ch == '/') {        // 单行注释
                    while (ch != '\n' && ch != -1) {
                        scan();
                    }
                    t = new Token(Tag::ERR);
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
                        LEXERROR(static_cast<int>(LexError::COMMENT_NO_END));
                    }
                    t = new Token(Tag::ERR);
                }
                else {
                    t = new Token(Tag::DIV);
                }
                break;
            case '%':
                t = new Token(Tag::MOD);
                scan();
                break;
            case '>':
                t = new Token(scan('=') ? Tag::GE : Tag::GT);
                break;
            case '<':
                t = new Token(scan('=') ? Tag::LE : Tag::LT);
                break;
            case '=':
                t = new Token(scan('=') ? Tag::EQU : Tag::ASSIGN);
                break;
            case '&':
                t = new Token(scan('&') ? Tag::AND : Tag::LEA);
                break;
            case '|':
                t = new Token(scan('|') ? Tag::OR : Tag::ERR);
                if (t->tag == Tag::ERR) {
                    LEXERROR(static_cast<int>(LexError::OR_NO_PAIR));
                }
                break;
            case '!':
                t = new Token(scan('=') ? Tag::NEQU : Tag::NOT);
                break;
            case ',':
                t = new Token(Tag::COMMA);
                scan();
                break;
            case ':':
                t = new Token(Tag::COLON);
                scan();
                break;
            case ';':
                t = new Token(Tag::SEMICON);
                scan();
                break;
            case '(':
                t = new Token(Tag::LPAREN);
                scan();
                break;
            case ')':
                t = new Token(Tag::RPAREN);
                scan();
                break;
            case '[':
                t = new Token(Tag::LBRACK);
                scan();
                break;
            case ']':
                t = new Token(Tag::RBRACK);
                scan();
                break;
            case '{':
                t = new Token(Tag::LBRACE);
                scan();
                break;
            case '}':
                t = new Token(Tag::RBRACE);
                scan();
                break;
            case -1:
                scan();
                break;
            default:
                t = new Token(Tag::ERR);
                LEXERROR(static_cast<int>(LexError::TOKEN_NO_EXIST));
                scan();
            }
        }

        if (token) {
            delete token;
        }
        token = t;
        if (token && token->tag != Tag::ERR) {
            return token;
        }
        else {
            continue;
        }
    }

    // EOF
    if (token) {
        delete token;
    }
    return token = new Token(Tag::END);
}
