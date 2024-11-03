#include "gcc_wrappers/type.h"
#include "gcc_wrappers/decl/type_def.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"
#include <cassert>
#include <c-family/c-common.h> // c_build_qualified_type, c_type_promotes_to, identifier_global_tag, lookup_name
#include <stringpool.h> // get_identifier

// Defined in GCC source, `c-tree.h`; not included in the plug-in headers, bafflingly.
// Do not confuse this with the C++ `comptypes`, which has a different signature and 
// is unavailable when compiling C.
extern int comptypes(tree, tree);

namespace gcc_wrappers {
   WRAPPED_TREE_NODE_BOILERPLATE(type)
   
   /*static*/ type type::lookup_by_name(const char* name) {
      auto id_node = get_identifier(name);
      auto node    = lookup_name(id_node);
      //
      // The `lookup_name` function returns a `*_DECL` node with the given 
      // identifier. However, TYPE_DECLs only exist in C when the `typedef` 
      // keyword is used. To find normal struct definitions, we need to use 
      // another approach.
      //
      if (node == NULL_TREE) {
         //
         // That "other approach" is to look for a "tag."
         //
         node = identifier_global_tag(id_node);
         if (node == NULL_TREE)
            return {};
      }
      if (TREE_CODE(node) == TYPE_DECL) {
         node = TREE_TYPE(node);
      } else if (!TYPE_P(node)) {
         return {};
      }
      return from_untyped(node);
   }
   /*static*/ type type::lookup_by_name(const std::string& s) {
      return lookup_by_name(s.c_str());
   }
   
   bool type::operator==(const type other) const {
      if (this->_node == other._node)
         return true;
      
      gcc_assert(c_language == c_language_kind::clk_c);
      return comptypes(this->_node, other._node) != 0;
   }
   
   std::string type::name() const {
      tree name_tree = TYPE_NAME(this->_node);
      if (!name_tree) {
         return {}; // unnamed struct
      }
      if (TREE_CODE(name_tree) == IDENTIFIER_NODE)
         return IDENTIFIER_POINTER(name_tree);
      if (TREE_CODE(name_tree) == TYPE_DECL && DECL_NAME(name_tree))
         return IDENTIFIER_POINTER(DECL_NAME(name_tree));
      //
      // Name irretrievable?
      //
      return {};
   }
   std::string type::pretty_print() const {
      if (empty())
         return "<empty>";
      
      std::string out;
      
      //
      // Try printing function types and function pointer types.
      //
      {
         bool is_const_pointer    = false;
         bool is_function_pointer = false;
         type ft;
         if (is_function()) {
            ft = *this;
         } else if (is_pointer()) {
            auto pt = remove_pointer();
            if (pt.is_function()) {
               ft = pt;
               is_function_pointer = true;
               is_const_pointer    = ft.is_const();
            }
         }
         if (!ft.empty()) {
            out = ft.function_return_type().pretty_print();
            if (is_function_pointer) {
               out += "(*";
               if (is_const_pointer) {
                  out += " const";
               }
               out += ')';
            } else {
               out += "(&)"; // not sure if this is correct
            }
            out += '(';
            if (!ft.is_unprototyped_function()) {
               bool first   = true;
               bool varargs = true;
               auto args    = ft.function_arguments();
               args.for_each_value([&first, &out, &varargs](tree raw) {
                  if (raw == void_type_node) {
                     varargs = false;
                     return;
                  }
                  type t = type::from_untyped(raw);
                  if (first) {
                     first = false;
                  } else {
                     out += ", ";
                  }
                  out += t.pretty_print();
               });
            }
            out += ')';
            return out;
         }
      }
      
      //
      // Try printing array types.
      //
      if (is_array()) {
         out = array_value_type().pretty_print();
         out += '[';
         if (auto extent = array_extent(); extent.has_value())
            out += *extent; // TODO: stringify number
         out += ']';
         return out;
      }
      
      //
      // Try printing pointer types.
      //
      if (is_pointer()) {
         out += remove_pointer().pretty_print();
         out += '*';
         if (is_const())
            out += " const";
         return out;
      }
      
      //
      // Base case.
      //
      
      if (is_const())
         out = "const ";
      
      auto id = TYPE_NAME(this->_node);
      switch (TREE_CODE(id)) {
         case IDENTIFIER_NODE:
            out += IDENTIFIER_POINTER(id);
            break;
         case TYPE_DECL:
            out += IDENTIFIER_POINTER(DECL_NAME(id));
            break;
         default:
            out += "<unnamed>";
            break;
      }
      
      return out;
   }
   
   list_node type::attributes() const {
      return list_node(TYPE_ATTRIBUTES(this->_node));
   }
   
   type type::canonical() const {
      type out;
      out.set_from_untyped(TYPE_CANONICAL(this->_node));
      return out;
   }
   type type::main_variant() const {
      type out;
      out.set_from_untyped(TYPE_MAIN_VARIANT(this->_node));
      return out;
   }
   
   decl::type_def type::declaration() const {
      auto dn = TYPE_NAME(this->_node);
      if (dn != NULL_TREE) {
         if (TREE_CODE(dn) != TYPE_DECL)
            dn = NULL_TREE;
      }
      return decl::type_def::from_untyped(dn);
   }
   
   bool type::is_complete() const {
      return COMPLETE_TYPE_P(this->_node);
   }
   
   size_t type::size_in_bits() const {
      auto size_node = TYPE_SIZE(this->_node);
      if (TREE_CODE(size_node) != INTEGER_CST)
         return 0;
      
      // This truncates, but if you have a type that's larger than 
      // (std::numeric_limits<uint32_t>::max() / 8), then you're 
      // probably screwed anyway tbh
      return TREE_INT_CST_LOW(size_node);
   }
   size_t type::size_in_bytes() const {
      return this->size_in_bits() / 8;
   }
   
   bool type::is_arithmetic() const {
      auto code = TREE_CODE(this->_node);
      return code == INTEGER_TYPE || code == REAL_TYPE;
   }
   bool type::is_array() const {
      return TREE_CODE(this->_node) == ARRAY_TYPE;
   }
   bool type::is_boolean() const {
      return TREE_CODE(this->_node) == BOOLEAN_TYPE;
   }
   bool type::is_enum() const {
      return TREE_CODE(this->_node) == ENUMERAL_TYPE;
   }
   bool type::is_fixed_point() const {
      return TREE_CODE(this->_node) == FIXED_POINT_TYPE;
   }
   bool type::is_floating_point() const {
      return TREE_CODE(this->_node) == REAL_TYPE;
   }
   bool type::is_function() const {
      auto code = TREE_CODE(this->_node);
      return code == FUNCTION_TYPE || code == METHOD_TYPE;
   }
   bool type::is_integer() const {
      return TREE_CODE(this->_node) == INTEGER_TYPE;
   }
   bool type::is_method() const {
      return TREE_CODE(this->_node) == METHOD_TYPE;
   }
   bool type::is_record() const {
      return TREE_CODE(this->_node) == RECORD_TYPE;
   }
   bool type::is_union() const {
      return TREE_CODE(this->_node) == UNION_TYPE;
   }
   bool type::is_void() const {
      return TREE_CODE(this->_node) == VOID_TYPE;
   }
   
   bool type::is_signed() const {
      return TYPE_UNSIGNED(this->_node) == 0;
   }
   bool type::is_unsigned() const {
      return TYPE_UNSIGNED(this->_node) != 0;
   }
   bool type::is_saturating() const {
      return TYPE_SATURATING(this->_node) != 0;
   }
   
   type type::add_const() const {
      gcc_assert(!empty());
      type out;
      out.set_from_untyped(c_build_qualified_type(this->_node, TYPE_QUAL_CONST));
      return out;
   }
   type type::remove_const() const {
      if (!is_const())
         return *this;
      type out;
      out.set_from_untyped(c_build_qualified_type(this->_node, 0));
      return out;
   }
   bool type::is_const() const {
      return TREE_READONLY(this->_node);
   }
   
   bool type::is_volatile() const {
      return TYPE_VOLATILE(this->_node);
   }
   
   type type::add_pointer() const {
      gcc_assert(!empty());
      return type::from_untyped(build_pointer_type(this->_node));
   }
   type type::remove_pointer() const {
      if (!is_pointer())
         return *this;
      type out;
      out.set_from_untyped(TREE_TYPE(this->_node));
      return out;
   }
   bool type::is_pointer() const {
      return TREE_CODE(this->_node) == POINTER_TYPE;
   }
   
   type type::make_signed() const {
      type out;
      out.set_from_untyped(signed_type_for(this->_node)); // tree.h
      return out;
   }
   type type::make_unsigned() const {
      type out;
      out.set_from_untyped(unsigned_type_for(this->_node)); // tree.h
      return out;
   }
   
   type type::add_array_extent(size_t n) const {
      type out;
      out.set_from_untyped(build_array_type(
         this->_node,
         build_index_type(size_int(n))
      ));
      return out;
   }
   
   type::fixed_point_info type::get_fixed_point_info() const {
      gcc_assert(is_fixed_point());
      fixed_point_info out;
      out.bitcounts = {
         .fractional = TYPE_FBIT(this->_node),
         .integral   = TYPE_IBIT(this->_node),
         .total      = TYPE_PRECISION(this->_node),
      };
      out.is_saturating = this->is_saturating();
      out.is_signed     = this->is_signed();
      {
         auto data = TYPE_MIN_VALUE(this->_node);
         if (TREE_CODE(data) == INTEGER_CST)
            out.min = TREE_INT_CST_LOW(data);
      }
      {
         auto data = TYPE_MAX_VALUE(this->_node);
         if (TREE_CODE(data) == INTEGER_CST)
            out.max = TREE_INT_CST_LOW(data);
      }
      return out;
   }
   
   type::floating_point_info type::get_floating_point_info() const {
      gcc_assert(is_floating_point());
      floating_point_info out;
      out.bitcount = TYPE_PRECISION(this->_node);
      return out;
   }
   
   type::integral_info type::get_integral_info() const {
      gcc_assert(is_integer() || is_enum() || is_fixed_point());
      integral_info out;
      out.bitcount  = TYPE_PRECISION(this->_node);
      out.is_signed = this->is_signed();
      {
         auto data = TYPE_MIN_VALUE(this->_node);
         if (TREE_CODE(data) == INTEGER_CST)
            out.min = TREE_INT_CST_LOW(data);
      }
      {
         auto data = TYPE_MAX_VALUE(this->_node);
         if (TREE_CODE(data) == INTEGER_CST)
            out.max = TREE_INT_CST_LOW(data);
      }
      return out;
   }
   
   //#pragma region Arrays
      bool type::is_variable_length_array() const {
         if (!is_array())
            return false;
         return this->array_extent().has_value();
      }
      
      std::optional<size_t> type::array_extent() const {
         gcc_assert(is_array());
         auto domain = TYPE_DOMAIN(this->_node);
         if (domain == NULL_TREE)
            return {};
         auto max = TYPE_MAX_VALUE(domain);
         //
         // Fail on non-constant:
         //
         if (!max)
            return {};
         if (TREE_CODE(max) != INTEGER_CST)
            return {};
         if (!TREE_CONSTANT(max))
            return {};
         //
         return (size_t)TREE_INT_CST_LOW(max) + 1;
      }
      
      size_t type::array_rank() const {
         if (!is_array())
            return 0;
         
         size_t i = 0;
         auto   t = this->_node;
         do {
            ++i;
         } while (t = TREE_TYPE(t), t != NULL_TREE && TREE_CODE(t) == ARRAY_TYPE);
         return i;
      }
      
      type type::array_value_type() const {
         gcc_assert(is_array());
         type out;
         out.set_from_untyped(TREE_TYPE(this->_node));
         return out;
      }
   //#pragma endregion
         
   //#pragma region Enums
      [[nodiscard]] std::vector<type::enum_member> type::all_enum_members() const {
         std::vector<type::enum_member> list;
         if (!is_enum())
            return list;
         for_each_enum_member([&list](const enum_member& item) {
            list.push_back(item);
         });
         return list;
      }
   //#pragma endregion
   
   //#pragma region Functions
      type type::function_return_type() const {
         gcc_assert(is_function());
         type out;
         out.set_from_untyped(TREE_TYPE(this->_node));
         return out;
      }
      
      list_node type::function_arguments() const {
         gcc_assert(is_function());
         return list_node(TYPE_ARG_TYPES(this->_node));
      }
      
      type type::nth_argument_type(size_t n) const {
         gcc_assert(is_function());
         type out;
         out.set_from_untyped(function_arguments().untyped_nth_value(n));
         return out;
      }
      
      type type::is_method_of() const {
         gcc_assert(is_method());
         type t;
         t.set_from_untyped(TYPE_METHOD_BASETYPE(this->_node));
         return t;
      }
      
      bool type::is_varargs_function() const {
         if (!is_function())
            return false;
         auto list = this->function_arguments();
         if (list.empty())
            return false; // unprototyped
         
         auto back = list.untyped_back();
         if (back != NULL_TREE && TREE_VALUE(back) == void_list_node)
            return true;
         
         return false;
      }
      
      size_t type::fixed_function_arg_count() const {
         gcc_assert(is_function());
         
         auto   list = function_arguments();
         size_t size = list.size();
         if (size) {
            if (list.untyped_back() == void_list_node)
               --size;
         }
         return size;
      }
      
      bool type::is_unprototyped_function() const {
         return is_function() && TYPE_ARG_TYPES(this->_node) == NULL;
      }
   //#pragma endregion
   
   //#pragma region Structs and unions
      list_node type::all_members() const {
         gcc_assert(is_record() || is_union());
         return list_node(TYPE_FIELDS(this->_node));
      }
   //#pragma endregion
}