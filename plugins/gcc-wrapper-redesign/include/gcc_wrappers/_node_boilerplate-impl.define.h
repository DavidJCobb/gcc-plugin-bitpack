#undef  GCC_NODE_WRAPPER_BOILERPLATE
#define GCC_NODE_WRAPPER_BOILERPLATE(type_name) \
   /*static*/ type_name ::wrap(tree t) { \
      assert(t != NULL_TREE); \
      if constexpr (impl::has_typecheck< type_name >) { \
         assert(raw_node_is(t)); \
      } \
      type_name out; \
      out._node = t; \
      return out; \
   }

