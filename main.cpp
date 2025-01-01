#include <vector>
#include <cstring>
#include <iostream>

#include "compiler/error.h"
#include "compiler/compiler.h"

using namespace std;


int main(int argc, char *argv[]) {
    vector<char *> srcfiles;

    if (argc > 1) {
        for (int i = 1; i < argc - 1; i++) {
            srcfiles.emplace_back(argv[i]);
        }
        char *opt = argv[argc - 1]; // 最后一个参数
        if (!strcmp(opt, "-char")) {
            Args::showChar = true;
        }
        else if (!strcmp(opt, "-token")) {
            Args::showToken = true;
        }
        else if (!strcmp(opt, "-symbol")) {
            Args::showSym = true;
        }
        else if (!strcmp(opt, "-ir")) {
            Args::showIr = true;
        }
        else if (!strcmp(opt, "-or")) {
            Args::opt = true;
            Args::showOr = true;
        }
        else if (!strcmp(opt, "-block")) {
            Args::opt = true;
            Args::showBlock = true;
        }
        else if (!strcmp(opt, "-o")) {
            Args::opt = true;
        }
        else if (!strcmp(opt, "-h")) {
            Args::showHelp = true;
        }
        else {
            srcfiles.emplace_back(opt);
        }
    }

    if (srcfiles.size() == 0) {
        return 0;
    }

    Compiler compiler;
    for (auto file : srcfiles) {
        string filename(file);
        compiler.compile(filename);
    }

    return 0;
}
