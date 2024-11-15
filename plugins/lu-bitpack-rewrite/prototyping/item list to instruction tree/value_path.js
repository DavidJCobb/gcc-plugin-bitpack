
// In C++, this class would be:
//
// std::variant<
//    descriptor_pair, // member access
//    descriptor_pair, // array index within a loop; these are the VAR_DECL of a local index variable
//    intmax_t         // array index; the index is an integer constant
// >
//
class value_path_segment {
   constructor() {
      // Only one of these two is ever present.
      this.descriptor  = null;
      this.array_index = null; // descriptor (pair) of local VAR_DECL, OR integer constant
   }
   clone() {
      let out = new value_path_segment;
      out.descriptor  = this.descriptor;
      out.array_index = this.array_index;
      return out;
   }
   is_array_index() {
      if (this.array_index)
         return true;
      if (this.array_index === 0)
         return true;
      return false;
   }
};

class value_path {
   constructor() {
      this.segments = []; // Array<value_path_segment>
   }
   access_array_element(constant_index_or_index_descriptor) {
      let segm = new value_path_segment();
      segm.array_index = constant_index_or_index_descriptor;
      this.segments.push(segm);
   }
   access_member(descriptor) {
      let segm = new value_path_segment();
      segm.descriptor = descriptor;
      this.segments.push(segm);
   }
   replace(descriptor) {
      let segm = new value_path_segment();
      segm.descriptor = descriptor;
      this.segments = [segm];
   }
   
   clone() {
      let out = new value_path();
      for(let segm of this.segments) {
         out.segments.push(segm.clone());
      }
      return out;
   }
   
   to_string() {
      let out = "";
      for(let segm of this.segments) {
         if (out) {
            if (!segm.is_array_index())
               out += ".";
         }
         if (segm.descriptor) {
            out += segm.descriptor.name;
         } else if (segm.is_array_index()) {
            out += "[";
            if (+segm.array_index === segm.array_index)
               out += segm.array_index;
            else
               out += segm.array_index.name;
            out += "]";
         }
      }
      return out;
   }
};