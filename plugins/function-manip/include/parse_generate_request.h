#pragma once
#include <optional>
#include <string>
#include <vector>
#include <gcc-plugin.h>
#include <tree.h>
#include <c-family/c-pragma.h>

struct generate_request {
   std::vector<std::string>> data_identifiers;
   std::string buffer_identifier;
   std::string sector_identifier;
};

extern std::optional<generate_request> parse_generate_request(struct cpp_reader* reader);