#pragma once
#include <gcc-plugin.h>
#include <tree.h>

namespace gcc_helpers {
   inline tree make_preincrement_expr(tree var_decl, int increment_by) {
      auto type  = TREE_TYPE(var_decl);
      tree value = build_int_cst(type, integer_constant);
      return build2(PREINCREMENT_EXPR, type, data, value);
   }
}