#pragma once

namespace bitpacking {
   enum class member_kind {
      none,
      array,
      boolean,
      buffer,
      integer,
      pointer,
      string,
      structure,
      
      // Not yet implemented: a union B that is a member of a struct A, 
      // where the active member of B is indicated by some B-sibling 
      // member A::c.
      union_external_tag,
      
      // Not yet implemented: a union whose members are all struct-type 
      // members, where the types all have the first N fields in common, 
      // and the union tag is one of those members.
      union_internal_tag,
   };
}