
class value_path_segment {
   constructor() {
      // Only one of these two is ever present.
      this.descriptor  = null;
      this.array_index = null; // descriptor (pair) of local VAR_DECL
   }
   clone() {
      let out = new value_path_segment;
      out.descriptor  = this.descriptor;
      out.array_index = this.array_index;
      return out;
   }
};

class value_path {
   constructor() {
      this.segments = []; // Array<value_path_segment>
   }
   access_array_element(index_descriptor) {
      let segm = new value_path_segment();
      segm.array_index = index_descriptor;
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
            if (!segm.array_index)
               out += ".";
         }
         if (segm.descriptor) {
            out += segm.descriptor.name;
         } else if (segm.array_index) {
            out += "[";
            out += segm.array_index.name;
            out += "]";
         }
      }
      return out;
   }
};