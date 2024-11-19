#pragma once
#include <string>
#include "lu/singleton.h"
#include "bitpacking/global_options.h"
#include "gcc_wrappers/decl/function.h"

class basic_global_state;
class basic_global_state : public lu::singleton<basic_global_state> {
   public:
      struct {
         gcc_wrappers::decl::function memcpy;
         gcc_wrappers::decl::function memset;
      } builtin_functions;
      struct {
         bitpacking::global_options::requested requested;
         bitpacking::global_options::computed  computed;
      } global_options;
      std::string xml_output_path;
      
      bool any_attributes_seen   = false;
      bool any_attributes_missed = false;
      bool enabled               = false;
      bool global_options_seen   = false;
      struct {
         location_t first_missed_attribute = UNKNOWN_LOCATION;
      } error_locations;
      struct {
         bool data_options_before_global_options = false;
      } errors_reported;
      
      void on_attribute_missed(tree applied_to);
      void on_attribute_seen(tree applied_to);
      void pull_builtin_functions();
};
