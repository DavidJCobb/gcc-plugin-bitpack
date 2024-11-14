
// just enough for JS
class decl_descriptor {
   constructor() {
      this.transformed_types = [];
   }
};

class array_access_info {
   constructor(o) {
      this.start = o?.start || 0;
      this.count = o?.count;
      if (!this.count && this.count !== 0)
         this.count = 1;
   }
};

class serialization_item {
   static basic_segment = class basic_segment {
      constructor() {
         this.object_name    = "";
         this.array_accesses = [];
      }
      
      compare(other) {
         if (this.object_name != other.object_name)
            return false;
         if (this.array_accesses.length != other.array_accesses.length)
            return false;
         for(let i = 0; i < this.array_accesses.length; ++i) {
            if (this.array_accesses[i].start != other.array_accesses[i].start)
               return false;
            if (this.array_accesses[i].count != other.array_accesses[i].count)
               return false;
         }
         return true;
      }
      
      to_string() {
         let out = this.object_name;
         for(let a of this.array_accesses) {
            if (a.count == 1) {
               out += `[${a.start}]`;
            } else {
               out += `[${a.start}:${a.start+a.count}]`;
            }
         }
         return out;
      }
      from_string(text) {
         this.array_accesses = [];
         
         let i = text.indexOf("[");
         if (i < 0) {
            this.object_name = text;
            return;
         }
         this.object_name = text.substring(0, i);
         if (!text.endsWith("]"))
            throw new Error("invalid");
         text = text.substring(i + 1);
         text = text.substring(0, text.length - 1); // clear trailing ']'
         text = text.split("][");
         for(let frag of text) {
            frag = frag.split(":");
            
            let item = new array_access_info();
            item.start = +frag[0];
            item.count = 1;
            this.array_accesses.push(item);
            if (frag[1]) {
               let end = +frag[1];
               item.count = end - item.start;
            }
         }
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
      from_string(text) {
         text = text.split(" == ");
         this.value = +text[1];
         
         text = text[0].split(".");
         for(let frag of text) {
            let item = new serialization_item.basic_segment();
            item.from_string(frag);
            this.segments.push(item);
         }
      }
   };
   
   static segment = class segment extends this.basic_segment {
      constructor() {
         super();
         this.condition       = null;
         this.transformations = []; // Array<String>
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
      from_string(text) {
         if (text.startsWith("(")) {
            let i   = text.indexOf(") ");
            let cnd = text.substring(1, i);
            this.condition = new serialization_item.condition();
            this.condition.from_string(cnd);
            text = text.substring(i + 2);
         }
         let match = text.match(/^([^\[\] ]+(?:\[[\d:\[\]]+\])?)((?: as .+)*)$/);
         if (!match)
            throw new Error("invalid");
         super.from_string(match[1]);
         
         match = match[2].substring(4).split(" as ");
         if (match.length > 0 && !!match[0])
            this.transformations = match;
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