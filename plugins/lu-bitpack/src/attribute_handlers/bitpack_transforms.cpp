#include "attribute_handlers/bitpack_transforms.h"
#include <optional>
#include <stdexcept>
#include <string>
#include <c-family/c-common.h> // lookup_name
#include "lu/strings/handle_kv_string.h"
#include "attribute_handlers/helpers/bp_attr_context.h"
#include "attribute_handlers/generic_bitpacking_data_option.h"
#include "bitpacking/transform_function_validation_helpers.h"
#include "gcc_wrappers/decl/function.h"
#include "gcc_wrappers/expr/string_constant.h"
#include <stringpool.h> // dependency for <attribs.h>
#include <attribs.h> // decl_attributes
namespace gw {
   using namespace gcc_wrappers;
}

namespace attribute_handlers {
   static void _reassemble_parameters(
      tree* node_ptr,
      tree  name,
      gw::expr::string_constant data,
      int   flags,
      //
      helpers::bp_attr_context& context
   ) {
      struct {
         std::optional<std::string> pre_pack;
         std::optional<std::string> post_unpack;
      } params;
      try {
         lu::strings::handle_kv_string(data.value_view(), std::array{
            lu::strings::kv_string_param{
               .name      = "pre_pack",
               .has_param = true,
               .handler   = [&params](std::string_view v, int vi) {
                  params.pre_pack = v;
               },
            },
            lu::strings::kv_string_param{
               .name      = "post_unpack",
               .has_param = true,
               .handler   = [&params](std::string_view v, int vi) {
                  params.post_unpack = v;
               },
            },
         });
      } catch (std::runtime_error& ex) {
         std::string message = "argument failed to parse: ";
         message += ex.what();
         context.report_error(message);
         return;
      }
      
      gw::decl::function pre_pack;
      gw::decl::function post_unpack;
      
      auto _get_func = [&context](
         const char* what,
         const std::optional<std::string>& src_opt,
         gw::decl::function& dst
      ) {
         if (!src_opt.has_value())
            return;
         const auto& src = *src_opt;
         if (src.empty()) {
            context.report_error("the %s function name, if specified, cannot be blank", what);
            return;
         }
         auto id   = get_identifier(src.c_str());
         auto node = lookup_name(id);
         if (node != NULL_TREE && TREE_CODE(node) == ADDR_EXPR) {
            //
            // Allow `&func`.
            //
            node = TREE_OPERAND(node, 0);
         }
         if (node == NULL_TREE) {
            context.report_error("problem with the %s option: no function exists with identifier %qE", what, id);
            return;
         }
         if (TREE_CODE(node) != FUNCTION_DECL) {
            context.report_error("problem with the %s option: identifier %qE does not refer to a function", what, id);
            return;
         }
         dst = gw::decl::function::from_untyped(node);
      };
      
      _get_func("pre-pack",    params.pre_pack,    pre_pack);
      _get_func("post-unpack", params.post_unpack, post_unpack);
      
      if (!pre_pack.empty() && !bitpacking::can_be_pre_pack_function(pre_pack)) {
         auto name = pre_pack.name().data();
         context.report_error("the specified pre-pack function %<%s%> has the wrong signature (should be %<void %s(const InSituType*, PackedType*)%>", name, name);
      }
      if (!post_unpack.empty() && !bitpacking::can_be_post_unpack_function(post_unpack)) {
         auto name = pre_pack.name().data();
         context.report_error("the specified post-unpack function %<%s%> has the wrong signature (should be %<void %s(InSituType*, const PackedType*)%>", name, name);
      }
      
      if (context.has_any_errors()) {
         return;
      }
      
      tree args = tree_cons(
         NULL_TREE,
         pre_pack.as_untyped(),
         tree_cons(
            NULL_TREE,
            post_unpack.as_untyped(),
            NULL_TREE
         )
      );
      
      auto attr_node = tree_cons(name, args, NULL_TREE);
      decl_attributes(
         &node_ptr[0],
         attr_node,
         flags | ATTR_FLAG_INTERNAL,
         node_ptr[1]
      );
   }

   extern tree bitpack_transforms(tree* node_ptr, tree name, tree args, int flags, bool* no_add_attrs) {
      *no_add_attrs = false;
      
      auto result = generic_bitpacking_data_option(node_ptr, name, args, flags, no_add_attrs);
      if (*no_add_attrs) {
         return result;
      }
      
      if (flags & ATTR_FLAG_INTERNAL) {
         return NULL_TREE;
      }
      if (list_length(args) > 1) {
         error("wrong number of arguments specified for %qE attribute", name);
         *no_add_attrs = true;
         return NULL_TREE;
      }
      
      helpers::bp_attr_context context(node_ptr, name, flags);
      gw::expr::string_constant data;
      {
         auto next = TREE_VALUE(args);
         if (next != NULL_TREE && TREE_CODE(next) == STRING_CST) {
            data = gw::expr::string_constant::from_untyped(next);
         } else {
            context.report_error("argument, if specified, must be a string constant");
         }
      }
      
      *no_add_attrs = true;
      if (!data.empty()) {
         //
         // Parse the arguments and reconstruct them.
         //
         _reassemble_parameters(
            node_ptr,
            name,
            data,
            flags,
            context
         );
      }
      return NULL_TREE;
   }
}