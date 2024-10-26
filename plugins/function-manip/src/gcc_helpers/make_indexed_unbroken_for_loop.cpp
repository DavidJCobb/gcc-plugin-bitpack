#pragma once
#include <gcc-plugin.h>
#include <tree.h>

namespace gcc_helpers {
   //
   // Create a simple for-loop with no break or continue statements, 
   // which uses a numeric index for the loop condition.
   //
   inline tree make_indexed_unbroken_for_loop(
      tree     containing_function_decl,
      tree     index_decl,
      intmax_t begin,
      intmax_t last,
      intmax_t increment,
      tree     body // statement list
   ) {
      //
      // GCC translates C for loops to GENERIC for the following:
      // 
      //    {
      //       int i = begin;
      //       {
      //          goto test;
      //       body:
      //          ...;
      //          ++i;
      //       test:
      //          if (i <= last)
      //             goto body;
      //          else
      //             goto done;
      //       done:
      //       }
      //    }
      //
      // We'll try to match it.
      //
      
      auto _make_label_decl = [containing_function_decl]() {
         tree decl = build_decl(UNKNOWN_LOCATION, LABEL_DECL, NULL_TREE, void_type_node);
         DECL_ARTIFICIAL(decl) = 1;
         DECL_IGNORED_P(decl) = 1;
         DECL_CONTEXT(decl) = containing_function_decl;
         return decl;
      };
      
      tree for_loop = build_block(NULL_TREE, NULL_TREE, NULL_TREE, NULL_TREE);
      
      TREE_USED(index_decl) = 1;
      if (begin != 0) {
         auto index_type = TREE_TYPE(index_decl);
         static_assert(false, "TODO: MODIFY_EXPR to set initial value");
      }
      
      //
      // TIP: We can simplify the code below by storing only the label exprs 
      //      in variables, and then using LABEL_EXPR_LABEL(expr) to access 
      //      the label decls. Fewer variables to keep track of; fewer places 
      //      where we might typo.
      //
      
      auto label_body_decl = _make_label_decl();
      auto label_test_decl = _make_label_decl();
      auto label_done_decl = _make_label_decl();
      //
      auto label_body_expr = build1(LABEL_EXPR, void_type_node, label_body_decl);
      auto label_test_expr = build1(LABEL_EXPR, void_type_node, label_test_decl);
      auto label_done_expr = build1(LABEL_EXPR, void_type_node, label_done_decl);
      
      append_to_statement_list(
         for_loop,
         build1(GOTO_EXPR, void_type_node, label_test_decl)
      );
      append_to_statement_list(for_loop, label_body_expr);
      append_to_statement_list(for_loop, body); // concatenates if body is a statement list
      append_to_statement_list(for_loop, for_body, gcc_helpers::make_preincrement_expr(index_decl, increment));
      append_to_statement_list(for_loop, label_test_expr);
      
      auto branch = fold_build3_loc(
         UNKNOWN_LOCATION,
         COND_EXPR,
         void_type_node,
         cond,
         build1(GOTO_EXPR, void_type_node, label_body_decl),
         build1(GOTO_EXPR, void_type_node, label_done_decl)
      );
      append_to_statement_list(for_loop, branch);
      
      append_to_statement_list(for_loop, label_done_expr);
      
      return for_loop;
   }
}