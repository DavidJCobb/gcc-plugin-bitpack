
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
      static type_name from_untyped(tree);       \
      \
      template<typename Subclass> requires impl::can_is_as<type_name, Subclass> \
      bool is() const { \
         return !empty() && Subclass::node_is(this->_node); \
      } \
      \
      template<typename Subclass> requires impl::can_is_as<type_name, Subclass> \
      Subclass as() { \
         if (is<Subclass>()) \
            return Subclass::from_untyped(this->_node); \
         return Subclass{}; \
      } \
      \
      template<typename Subclass> requires impl::can_is_as<type_name, Subclass> \
      const Subclass as() const { \
         if (is<Subclass>()) \
            return Subclass::from_untyped(this->_node); \
         return Subclass{}; \
      }
