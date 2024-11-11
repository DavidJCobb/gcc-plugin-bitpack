#include "attribute_handlers/bitpack_transforms.h"
#include <optional>
#include <stdexcept>
#include <string>
#include <c-family/c-common.h> // lookup_name
#include <stringpool.h> // get_identifier
#include "lu/strings/handle_kv_string.h"
#include "attribute_handlers/helpers/bp_attr_context.h"
#include "attribute_handlers/generic_bitpacking_data_option.h"
#include "bitpacking/transform_function_validation_helpers.h"
#include "gcc_wrappers/decl/function.h"
#include "gcc_wrappers/expr/string_constant.h"
namespace gw {
   using namespace gcc_wrappers;
}

namespace attribute_handlers {
   static void _reassemble_parameters(
      gw::expr::string_constant data,
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
      
      bool both_are_individually_correct = true;
      
      if (!params.pre_pack.has_value()) {
         context.report_error("does not specify a pre-pack function name");
         both_are_individually_correct = false;
      }
      if (!params.post_unpack.has_value()) {
         context.report_error("does not specify a post-unpack function name");
         both_are_individually_correct = false;
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
         both_are_individually_correct = false;
      }
      if (!post_unpack.empty() && !bitpacking::can_be_post_unpack_function(post_unpack)) {
         auto name = pre_pack.name().data();
         context.report_error("the specified post-unpack function %<%s%> has the wrong signature (should be %<void %s(InSituType*, const PackedType*)%>", name, name);
         both_are_individually_correct = false;
      }
      
      if (both_are_individually_correct) {
         //
         // Verify that the functions match each other.
         //
         auto mismatch_text = bitpacking::check_transform_functions_match_types(pre_pack, post_unpack);
         if (!mismatch_text.empty()) {
            mismatch_text.insert(0, "specifies incompatible functions: ");
            context.report_error(mismatch_text);
         }
         //
         // If the functions' in situ types match, verify that they match 
         // the thing we're applying this attribute to.
         //
         auto in_situ = bitpacking::get_in_situ_type(pre_pack, post_unpack);
         if (!in_situ.empty()) {
            auto type = context.type_of_target();
            if (in_situ.main_variant() != type.main_variant()) {
               auto pp_a = in_situ.pretty_print();
               auto pp_b = type.pretty_print();
               context.report_error("specifies functions that transform type %<%s%>, but would be applied to type %<%s%>", pp_a.c_str(), pp_b.c_str());
            }
         }
      }
      
      if (context.has_any_errors()) {
         return;
      }
      
      gw::list_node args;
      args.append(NULL_TREE, pre_pack.as_untyped());
      args.append(NULL_TREE, post_unpack.as_untyped());
      context.reapply_with_new_args(args);
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
      context.check_and_report_contradictory_x_options(helpers::x_option_type::transforms);
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
         _reassemble_parameters(data, context);
      }
      return NULL_TREE;
   }
}