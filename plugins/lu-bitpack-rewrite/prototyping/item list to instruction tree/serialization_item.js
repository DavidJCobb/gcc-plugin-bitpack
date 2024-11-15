
class serialization_item {
   static basic_segment = class basic_segment {
      constructor() {
         this.descriptor     = null; // decl_descriptor
         this.array_accesses = [];
      }
      
      compare(other) {
         if (this.descriptor != other.descriptor)
            return false;
         if (this.array_accesses.length != other.array_accesses.length)
            return false;
         for(let i = 0; i < this.array_accesses.length; ++i)
            if (!this.array_accesses[i].compare(other.array_accesses[i]))
               return false;
         return true;
      }
      
      to_string() {
         let out = this.descriptor.name;
         for(let a of this.array_accesses)
            out += a.to_string();
         return out;
      }
   };
   
   static condition = class condition {
      constructor() {
         this.segments = []; // LHS // Array<basic_segment>
         this.value    = 0;  // RHS
      }
      
      compare_lhs(other) {
         if (this.segments.length != other.segments.length)
            return false;
         for(let i = 0; i < this.segments.length; ++i)
            if (!this.segments[i].compare(other.segments[i]))
               return false;
         return true;
      }
      compare(other) {
         if (this.value != other.value)
            return false;
         if (!this.compare_lhs(other))
            return false;
         return true;
      }
      
      to_string() {
         let out = "";
         for(let i = 0; i < this.segments.length; ++i) {
            if (i > 0)
               out += ".";
            out += this.segments[i].to_string();
         }
         out += " == ";
         out += this.value;
         return out;
      }
   };
   
   static segment = class segment extends this.basic_segment {
      constructor() {
         super();
         this.condition = null;
      }
      
      get transformations() {
         let out = [];
         for(let type of this.descriptor.transformed_types)
            out.push(type.name);
         return out;
      }
      
      compare(other) {
         if (!super.compare(other))
            return false;
         if (this.condition) {
            if (!other.condition)
               return false;
            if (!this.condition.compare(other.condition))
               return false;
         } else {
            if (other.condition)
               return false;
         }
         if (this.transformations.length != other.transformations.length)
            return false;
         for(let i = 0; i < this.transformations.length; ++i)
            if (this.transformations[i] != other.transformations[i])
               return false;
         return true;
      }
      
      to_string() {
         let out = "";
         if (this.condition) {
            out = "(" + this.condition.to_string() + ") ";
         }
         out += super.to_string();
         for(let tran of this.transformations)
            out += ` as ${tran}`;
         return out;
      }
   };
   
   constructor() {
      this.segments = []; // Array<segment>
   }
   
   compare_stem(stem_segment_count, other) {
      if (other.segments.length < stem_segment_count)
         return false;
      if (this.segments.length < stem_segment_count)
         return false;
      for(let i = 0; i < stem_segment_count; ++i) {
         let a = this.segments[i];
         let b = other.segments[i];
         if (!a.compare(b))
            return false;
      }
      return true;
   }
   
   from_string(s) {
      s = s.split("|");
      for(let frag of s) {
         let segm = new this.constructor.segment();
         segm.from_string(frag);
         this.segments.push(segm);
      }
   }
   to_string() {
      let out = "";
      for(let i = 0; i < this.segments.length; ++i) {
         if (i > 0)
            out += "|";
         let segm = this.segments[i];
         out += segm.to_string();
      }
      return out;
   }
   
};