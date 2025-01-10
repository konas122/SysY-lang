#include "ass.h"
#include "semantic.h"


FILE *fin = nullptr;
FILE *fout = nullptr;

bool showAss = false;

string finName;


void assemble(const string &filename) {
    int pos = filename.rfind(".c");
    finName = filename.substr(0, pos);

    fin = fopen((finName + ".s").c_str(), "r");
    fout = fopen((finName + ".t").c_str(), "w");
    program();
    obj.writeElf();
    fclose(fin);
    fclose(fout);
    remove((finName+".t").c_str());
}
