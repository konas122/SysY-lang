#include "error.h"
#include "compiler.h"

#include "lexer/scanner.h"

using namespace std;


Scanner::Scanner(const string& name) {
    const char *fname = name.c_str();
    file = fopen(fname, "r");
    if (!file) {
        printf(FATAL "Failed to open file %s\n", fname);
        Error::errorNum++;
    }
    fileName = name;
    lineLen = 0;
    readPos = -1;
    lastch = 0;
    lineNum = 1;
    colNum = 0;
}

Scanner::~Scanner() {
    if (file) {
        PDEBUG(WARN "The file is not scanned completely!\n");
        Error::warnNum++;
        fclose(file);
        file = nullptr;
    }
}

void Scanner::showChar(char ch) {
    if (ch == -1) {
        printf("EOF");
    }
    else if (ch == '\n') {
        printf("\\n");
    }
    else if(ch=='\t') {
        printf("\\t");
    }
    else if (ch == ' ') {
        printf("<blank>");
    }
    else {
        printf("%c", ch);
    }
    printf("\t\t<%d>\n", ch);
}

int Scanner::scan() {
    if (!file) {
        return -1;
    }

    if (readPos == lineLen - 1) {
        lineLen = fread(line, 1, BUFLEN, file);
        if (lineLen == 0) {
            lineLen = 1;
            line[0] = -1;
        }
        readPos = -1;
    }

    readPos++;
    char ch = line[readPos];
    if (lastch == '\n') {
        lineNum++;
        colNum = 0;
    }
    if (ch == -1) {
        fclose(file);
        file = nullptr;
    }
    else if (ch != '\n') {
        if (ch == '\t') {
            colNum += 4;
        }
        else {
            colNum++;
        }
    }

    lastch = ch;
    if (Args::showChar) {
        showChar(ch);
    }

    return ch;
}

string& Scanner::getFile() {
    return fileName;
}

int Scanner::getLine() {
    return lineNum;
}

int Scanner::getCol() {
    return colNum;
}
