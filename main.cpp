#include <unistd.h>
#include <sys/stat.h>

#include <vector>
#include <cstring>
#include <iostream>
#include <algorithm>

#include "compiler/error.h"
#include "compiler/compiler.h"
#include "assembler/ass.h"
#include "linker/link.h"

using namespace std;


int main(int argc, char *argv[]) {
    vector<string> srcfiles;

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
        else if (!strcmp(opt, "-opt")) {
            Args::opt = true;
        }
        else if (!strcmp(opt, "-h")) {
            Args::showHelp = true;
        }
        else {
            srcfiles.emplace_back(opt);
        }
    }

    if (Args::showHelp) {
        cout << "CIT: Compile In Time (SysYlang)" << endl;
        cout << "命令格式: cit 源文件[源文件] [选项]\n"
                "选项: \n"
                "\t-opt\t\t# 执行优化\n"
                "\t-char\t\t# 显示文件字符\n"
                "\t-token\t\t# 显示词法记号\n"
                "\t-symbol\t\t# 显示符号表信息\n"
                "\t-ir\t\t# 显示中间代码\n"
                "\t-or\t\t# 显示优化后的中间代码\n"
                "\t-block\t\t# 显示基本块和流图关系\n"
                "\t-h\t\t# 显示帮助信息\n";
        return 0;
    }

    if (srcfiles.size() == 0 && !Args::showHelp) {
        cout << "命令格式错误! (使用 -h 选项查看帮助)" << endl;
        return 255;
    }

    Compiler compiler;
    for (const auto &file : srcfiles) {
        compiler.compile(file);
        int error = Error::getErrorNum();
        int warn = Error::getWarnNum();
        cout << "Compile Done: Error=" << error << ", Warn=" << warn << "." << endl;
    }

    transform(srcfiles.cbegin(), srcfiles.cend(), srcfiles.begin(),
              [](std::string_view file)
              {
                  int pos = file.rfind(".c");
                  return file.substr(0, pos);
              });

    for (const auto &file : srcfiles) {
        assemble(file);
    }

    // assemble("test/test1.c");

    transform(srcfiles.cbegin(), srcfiles.cend(), srcfiles.begin(),
              [](const string &file)
              {
                  return (file + ".o");
              });

    string dest = "a.out";
    link(srcfiles, dest);

    mode_t mode;
    struct stat filestat;
    if (stat(dest.c_str(), &filestat) == -1) {
        perror("stat");
        return 1;
    }

    mode = filestat.st_mode | S_IXUSR;
    if (chmod(dest.c_str(), mode) == -1) {
        perror("chmod");
        return 1;
    }

    return 0;
}
