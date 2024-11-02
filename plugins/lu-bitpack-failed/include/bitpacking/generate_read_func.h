#pragma once

struct generate_request;
struct global_bitpack_options;

extern void generate_read_func(
   const global_bitpack_options& options,
   const generate_request&       request
);