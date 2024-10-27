
//
// Declares the following functions:
//
//    void T::set_from_untyped(tree);
//       Verifies the TREE_CODE if possible.
//
//    static T T::from_untyped(tree);
//       Wraps an unwrapped tree, verifying the TREE_CODE 
//       if possible.
//
// Only works if the wrapper doesn't define additional fields.
//

#define WRAPPED_TREE_NODE_BOILERPLATE(type_name) \
   public:                                       \
      void set_from_untyped(tree);               \
      static type_name from_untyped(tree);
