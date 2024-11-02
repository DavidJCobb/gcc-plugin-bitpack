#pragma once
#include <functional>
#include <string_view>

struct kv_string_param {
   std::string_view name;
   bool has_param = false;
   bool int_param = false;
   std::function<bool(std::string_view text, int number)> handler;
};

namespace _impl {
   extern bool handle_kv_string(std::string_view data, const kv_string_param* params, size_t param_count);
}

template<size_t Count>
bool handle_kv_string(std::string_view data, const std::array<kv_string_param, Count>& params) {
   return _impl::handle_kv_string(data, params.data(), params.size());
}