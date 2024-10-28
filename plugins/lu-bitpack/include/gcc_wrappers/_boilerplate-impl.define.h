
//
// Defines the following functions:
//
//    void T::set_from_untyped(tree);
//       Verifies the TREE_CODE if possible.
//
//    static T T::from_untyped(tree);
//       Wraps an unwrapped tree, verifying the TREE_CODE 
//       if possible.
//

#undef WRAPPED_TREE_NODE_BOILERPLATE
#define WRAPPED_TREE_NODE_BOILERPLATE(type_name)                  \
   void type_name ::set_from_untyped(tree t) {                    \
      _set_from_untyped<type_name>(t);                            \
   }                                                              \
   /*static*/ type_name type_name ::from_untyped(tree t) {        \
      return _from_unwrapped<type_name>(t); \
   }

