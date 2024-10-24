#include <cstdint>
#include <optional>

struct BitpackOptions {
   bool do_not_serialize = false;
   //
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