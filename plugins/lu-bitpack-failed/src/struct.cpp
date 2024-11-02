#include "struct.h"
#include <diagnostic.h> // warning(), error(), etc. (transitively included from diagnostic-core.h)
#include "gcc_helpers/for_each_in_list_tree.h"
#include "gcc_helpers/type_name.h"
#include "gcc_helpers/type_size_in_bytes.h"

void Struct::from_gcc_tree(tree type) {
   this->name = gcc_helpers::type_name(type);
   {
      auto opt = gcc_helpers::type_size_in_bytes(type);
      if (opt.has_value())
         this->bytecount = opt.value();
   }
   
   tree fields = TYPE_FIELDS(type);
   gcc_helpers::for_each_in_list_tree(fields, [this](tree field) {
      auto member = DataMember::from_tree(field);
      if (member.has_value()) {
         this->members.push_back(std::move(*member));
      }
   });
}