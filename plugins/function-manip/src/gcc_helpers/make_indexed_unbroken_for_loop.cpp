#include "gcc_helpers/make_indexed_unbroken_for_loop.h"
#include <gcc-plugin.h>
#include <tree.h>
#include <tree-iterator.h> // alloc_stmt_list, append_to_statement_list
#include "gcc_helpers/make_preincrement_expr.h"

namespace gcc_helpers {
   //
   // Create a simple for-loop with no break or continue statements, 
   // which uses a numeric index for the loop condition.
   //
   extern tree make_indexed_unbroken_for_loop(
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
      
      auto index_type = TREE_TYPE(index_decl);
      
      tree for_loop = build_block(NULL_TREE, NULL_TREE, NULL_TREE, NULL_TREE);
      
      TREE_USED(index_decl) = 1;
      if (begin != 0) {
         auto assign     = build2(
            MODIFY_EXPR,
            index_type, // ? may need to be the type we're assigning ?
            index_decl, // dst
            build_int_cst(index_type, begin) // src
         );
         append_to_statement_list(for_loop, &assign);
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
      
      {
         auto goto_expr = build1(GOTO_EXPR, void_type_node, label_test_decl);
         append_to_statement_list(for_loop, &goto_expr);
      }
      append_to_statement_list(for_loop, &label_body_expr);
      append_to_statement_list(for_loop, &body); // concatenates if body is a statement list
      {
         auto incr_expr = gcc_helpers::make_preincrement_expr(index_decl, increment);
         append_to_statement_list(for_loop, &incr_expr);
      }
      append_to_statement_list(for_loop, &label_test_expr);
      
      // TODO: Consider `build_conditional_expr`
      auto branch = fold_build3_loc(
         UNKNOWN_LOCATION,
         COND_EXPR,
         void_type_node,
         build2(LE_EXPR, boolean_type_node, index_decl, build_int_cst(index_type, last)),
         build1(GOTO_EXPR, void_type_node, label_body_decl),
         build1(GOTO_EXPR, void_type_node, label_done_decl)
      );
      append_to_statement_list(for_loop, &branch);
      
      append_to_statement_list(for_loop, &label_done_expr);
      
      return for_loop;
   }
}