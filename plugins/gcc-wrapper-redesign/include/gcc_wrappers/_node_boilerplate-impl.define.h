#undef  GCC_NODE_WRAPPER_BOILERPLATE
#define GCC_NODE_WRAPPER_BOILERPLATE(type_name) \
   /*static*/ type_name type_name::wrap(tree t) { \
      assert(t != NULL_TREE); \
      impl::do_typecheck< type_name >(t); \
      type_name out; \
      out._node = t; \
      return out; \
   }

