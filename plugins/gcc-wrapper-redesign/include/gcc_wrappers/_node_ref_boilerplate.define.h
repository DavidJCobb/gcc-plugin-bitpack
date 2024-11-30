#undef  GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE
#define GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(type_name) \
   public: \
      static type_name wrap(tree); \
      \
      template<typename Subclass> requires impl::can_is_as<type_name, Subclass> \
      bool is() const { \
         return Subclass::raw_node_is(this->_node); \
      } \
      \
      template<typename Subclass> requires impl::can_is_as<type_name, Subclass> \
      Subclass as() { \
         if (is<Subclass>()) \
            return Subclass::wrap(this->_node); \
         return Subclass{}; \
      } \
      \
      template<typename Subclass> requires impl::can_is_as<type_name, Subclass> \
      const Subclass as() const { \
         if (is<Subclass>()) \
            return Subclass::wrap(this->_node); \
         return Subclass{}; \
      }

#undef  DECLARE_GCC_NODE_POINTER_WRAPPER
#define DECLARE_GCC_NODE_POINTER_WRAPPER(reference_wrapper_type_name) \
   using reference_wrapper_type_name##_ptr = node_pointer_template< reference_wrapper_type_name >;