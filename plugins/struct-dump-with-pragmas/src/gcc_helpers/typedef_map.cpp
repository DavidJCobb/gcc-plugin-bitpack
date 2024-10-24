#include "gcc_helpers/typedef_map.h"

#include <gcc-plugin.h> 
#include <tree.h>

// for access to global_namespace
#include <cp/cp-tree.h>
#include <cp/name-lookup.h>
__typeof__ (cp_global_trees) cp_global_trees __attribute__ ((weak));
__typeof__ (cp_namespace_decls) cp_namespace_decls __attribute__ ((weak));

#include "gcc_helpers/extract_string_constant_from_tree.h"
#include "gcc_helpers/for_each_in_list_tree.h"
#include "gcc_helpers/type_name.h"

namespace gcc_helpers {
   typedef_map::~typedef_map() {
      for(auto& pair : this->_types)
         delete pair.second;
      this->_types.clear();
   }
   
   void typedef_map::_connect_all() {
      for(auto& pair : this->_types) {
         auto& item = *pair.second;
         if (item.alias_of.name.empty())
            continue;
         
         auto it = this->_types.find(item.alias_of.name);
         if (it != this->_types.end())
            item.alias_of.info = it->second;
      }
   }
   void typedef_map::build() {
      if (global_namespace == NULL_TREE)
         return;
      
      auto all_decls = cp_namespace_decls(global_namespace);
      if (all_decls == NULL_TREE)
         return;
      
      for_each_in_list_tree(all_decls, [this](tree decl) {
         if (TREE_CODE(decl) == TREE_LIST) { // overload
            return;
         }
         if (TREE_CODE(decl) != TYPE_DECL) {
            return;
         }
         //
         // Typedef.
         //
         auto aliased = TREE_TYPE(decl);
         auto name    = extract_string_constant_from_tree(DECL_NAME(decl));
         if (name.empty())
            return;
         
         auto aliased_name = type_name(aliased);
         if (aliased_name.empty())
            return;
         
         auto*& item_ptr = this->_types[name];
         if (!item_ptr) {
            item_ptr = new type_info;
         }
         auto& item = *item_ptr;
         if (!item.alias_of.name.empty()) {
            if (item.alias_of.name != aliased_name) {
               item.is_conflicted = true;
            }
            return;
         }
         item.alias_of.name = aliased_name;
      });
      
      this->_connect_all();
   }
   const typedef_map::type_info* typedef_map::query(const std::string_view name) {
      auto it = this->_types.find(std::string(name));
      if (it != this->_types.end())
         return it->second;
      return nullptr;
   }
}                      