#include <cstdint>
#include <optional>

struct BitpackOptions {
   bool do_not_serialize = false;
   //
   std::string_view inherit_from;
   struct {
      std::string_view pre_pack;
      std::string_view post_unpack;
   } funcs;
   struct {
      std::optional<uint8_t>  bitcount;
      std::optional<int64_t>  min;
      std::optional<uint64_t> max;
   } integral;
   struct {
      std::optional<size_t> length;
      bool null_terminated = false;
   } string;
};