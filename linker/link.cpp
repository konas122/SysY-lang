#include "linker.h"

bool showLink = false;


void link(const std::vector<std::string> &files, const std::string &filename) {
    Linker linker;
    for (const auto &file : files) {
        linker.addElf(file.c_str());
    }
    linker.link(filename.c_str());
}
