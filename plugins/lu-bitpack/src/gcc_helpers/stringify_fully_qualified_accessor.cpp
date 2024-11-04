#include "gcc_helpers/stringify_fully_qualified_accessor.h"
#include <charconv>
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/type/base.h"
namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}
#include <diagnostic.h>
#include <print-tree.h>

namespace {
   template<typename T>
   std::string _to_chars(T value) {
      char buffer[64] = { 0 };
      auto result = std::to_chars((char*)buffer, (char*)(&buffer + sizeof(buffer) - 1), value);
      if (result.ec == std::errc{}) {
         *result.ptr = '\0';
      } else {
         buffer[0] = '?';
         buffer[1] = '\0';
      }
      return buffer;
   }
   
   std::string _traverse(tree node) {
      std::string v;
      switch (TREE_CODE(node)) {
         case PARM_DECL:
         case VAR_DECL:
            {
               auto id_node = DECL_NAME(node);
               if (id_node != NULL_TREE && TREE_CODE(id_node) == IDENTIFIER_NODE) {
                  return IDENTIFIER_POINTER(id_node);
               }
               return "@unnamed";
            }
            break;
            
         // Enumeration member
         case CONST_DECL:
            v = gw::type::base::from_untyped(TREE_TYPE(node)).pretty_print();
            v += "::";
            {
               auto id_node = DECL_NAME(node);
               if (id_node != NULL_TREE && TREE_CODE(id_node) == IDENTIFIER_NODE) {
                  v += IDENTIFIER_POINTER(id_node);
               } else {
                  v += "@unnamed";
               }
            }
            return v;
         
         case ADDR_EXPR:
            v = '&';
            v += _traverse(TREE_OPERAND(node, 0));
            break;
         case ARRAY_REF:
            v = _traverse(TREE_OPERAND(node, 0));
            v += '[';
            v += _traverse(TREE_OPERAND(node, 1));
            v += ']';
            break;
         case COMPONENT_REF:
            {
               auto record = TREE_OPERAND(node, 0);
               auto field  = TREE_OPERAND(node, 1);
               
               auto field_name = gw::decl::field::from_untyped(field).name();
               
               v = _traverse(record);
               if (!field_name.empty()) {
                  v += '.';
                  v += field_name;
               }
               return v;
            }
            break;
         case INDIRECT_REF:
            v = "(*";
            v += _traverse(TREE_OPERAND(node, 0));
            v += ')';
            break;
         case INTEGER_CST:
            if (TYPE_UNSIGNED(TREE_TYPE(node))) {
               return _to_chars((unsigned HOST_WIDE_INT) TREE_INT_CST_LOW(node));
            }
            return _to_chars((HOST_WIDE_INT) TREE_INT_CST_LOW(node));
         case REAL_CST:
            {
               char buffer[64];
               real_to_decimal(buffer, &TREE_REAL_CST(node), sizeof(buffer), 0, 1);
               return std::string(buffer);
            }
            break;
            
         default:
            return "???";
      }
      return v;
   }
}

namespace gcc_helpers {
   extern std::string stringify_fully_qualified_accessor(gcc_wrappers::value v) {
      return _traverse(v.as_untyped());
   }
}