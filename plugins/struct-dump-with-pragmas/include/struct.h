#pragma once
#include <string>
#include <string_view>
#include <vector>
#include "data_member.h"

#include <gcc-plugin.h> 
#include <tree.h>

struct Struct {
   std::string name;
   std::vector<DataMember> members;
   
   size_t bytecount = 0;
   
   void from_gcc_tree(tree);
};