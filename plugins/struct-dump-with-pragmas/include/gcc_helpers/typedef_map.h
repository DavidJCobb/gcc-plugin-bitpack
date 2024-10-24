#include <string>
#include <string_view>
#include <unordered_map>

namespace gcc_helpers {
   //
   // Map of all types, allowing us to iterate over transitive typedefs.
   //
   class typedef_map {
      public:
         struct type_info {
            std::string name;
            struct {
               std::string name;
               type_info*  info = nullptr;
            } alias_of;
            bool is_conflicted = false;
            bool is_integer    = false;
            bool is_pointer    = false;
         };
         
      protected:
         std::unordered_map<std::string, type_info*> _types;
      
         void _connect_all();
      
      public:
         ~typedef_map();
      
         void build();
         const type_info* query(const std::string_view);
   };
}