#ifndef __LINKER_LINK_H__
#define __LINKER_LINK_H__

#include <vector>
#include <string>

extern bool showLink;

void link(const std::vector<std::string> &files, const std::string &filename);

#endif
