
let rechunked = {};

rechunked.chunks = {};
//
rechunked.chunks.base = class base {
   constructor() {
   }
};
rechunked.chunks.transform = class transform extends rechunked.chunks.base {
   constructor() {
      super();
      this.types = []; // Array<c_type>
   }
   compare(other) {
      if (this.types.length != other.types.length)
         return false;
      if (this.decl_path.length != other.decl_path.length)
         return false;
      for(let i = 0; i < this.types.length; ++i)
         if (this.types[i] != other.types[i])
            return false;
      for(let i = 0; i < this.decl_path.length; ++i)
         if (!this.decl_path[i].compare(other.decl_path[i]))
            return false;
      return true;
   }
};
rechunked.chunks.qualified_decl = class qualified_decl extends rechunked.chunks.base {
   constructor() {
      super();
      this.descriptors = []; // Array<decl_descriptor>
   }
   append(desc) {
      this.descriptors.push(desc);
   }
   compare(other) {
      if (this.descriptors.length != other.descriptors.length)
         return false;
      for(let i = 0; i < this.descriptors.length; ++i)
         if (this.descriptors[i] != other.descriptors[i])
            return false;
      return true;
   }
   to_string() {
      let text = "";
      for(let desc of this.descriptors) {
         if (text)
            text += '.';
         text += desc.name;
      }
      return text;
   }
};
rechunked.chunks.array_slice = class array_slice extends rechunked.chunks.base {
   constructor() {
      super();
      this.start = 0;
      this.count = 0;
   }
   
   compare(other) {
      if (this.start != other.start)
         return false;
      return this.count == other.count;
   }
   
   to_string() {
      if (this.count == 1)
         return `[${this.start}]`;
      let end = this.start + this.count;
      return `[${this.start}:${end}]`;
   }
};
rechunked.chunks.condition = class condition extends rechunked.chunks.base {
   constructor() {
      super();
      this.lhs = null; // rechunked.item // only qualified_decl or array_slice chunks
      this.rhs = 0;
   }
   
   compare_lhs(other) {
      if (this.lhs.length != other.lhs.length)
         return false;
      for(let i = 0; i < this.lhs.length; ++i)
         if (!this.lhs[i].compare(other.lhs[i]))
            return false;
      return true;
   }
   compare(other) {
      if (this.rhs != other.rhs)
         return false;
      return this.compare_lhs(other);
   }
   
   to_string() {
      let out = "";
      out += this.lhs.to_string();
      out += " == ";
      out += this.rhs;
      return out;
   }
};

rechunked.condition_lhs = class condition_lhs {
   constructor(parent, seri_cnd) {
      this.parent      = parent;
      this.relative_to = -1; // index of a chunk in the containing rechunked item
      this.chunks      = [];
      
      if (parent && seri_cnd) {
         for(let segm of seri_cnd.segments) {
            let back = null;
            if (this.chunks.length > 0)
               back = this.chunks[this.chunks.length - 1];
            //
            if (back instanceof rechunked.chunks.qualified_decl) {
               back.descriptors.push(segm.descriptor);
            } else {
               let back = new rechunked.chunks.qualified_decl();
               back.descriptors.push(segm.descriptor);
               this.chunks.push(back);
            }
            
            for(let aai of segm.array_accesses) {
               let back = new rechunked.chunks.array_slice();
               back.start = aai.start;
               back.count = aai.count;
               this.chunks.push(back);
            }
         }
         
         /*let end = Math.min(this.chunks.length, parent.chunks.length);
         {
            let i = 0;
            let j = 0;
            for(; i < this.chunks.length && j < parent.chunks.length; ++i, ++j) {
               let a = this.chunks[i];
               let b = parent.chunks[j];
               if (
                  b instanceof rechunked.chunks.qualified_decl ||
                  b instanceof rechunked.chunks.array_slice
               ) {
                  if (!a.compare(b))
                     break;
               } else {
                  --i;
               }
            }
            this.relative_to = j - 1;
            this.chunks.splice(0, i);
         }*/
      }
   }
   
   to_string() {
      let out = "";
      
      function _chunk_list_to_string(list, end) {
         if (end === void 0)
            end = list.length;
         for(let i = 0; i < end; ++i) {
            let chunk = list[i];
            if (chunk instanceof rechunked.chunks.condition) // from parent
               continue;
            if (chunk instanceof rechunked.chunks.transform) // from parent
               continue;
            if (!(chunk instanceof rechunked.chunks.array_slice)) {
               if (out)
                  out += '.';
            }
            out += chunk.to_string();
         }
      }
      
      if (this.relative_to >= 0) {
         if (this.parent) {
            _chunk_list_to_string(this.parent.chunks, this.relative_to + 1);
         } else {
            out += `[#${this.relative_to}...]`;
         }
      }
      _chunk_list_to_string(this.chunks);
      return out;
   }
};

rechunked.item = class item {
   constructor() {
      this.chunks = [];
   }
   from_serialization_item(item) {
      for(let segm of item.segments) {
         if (segm.condition) {
            let chnk = new rechunked.chunks.condition();
            chnk.lhs = new rechunked.condition_lhs(this, segm.condition);
            chnk.rhs = segm.condition.value;
            this.chunks.push(chnk);
         }
         
         let back = null;
         if (this.chunks.length > 0)
            back = this.chunks[this.chunks.length - 1];
         //
         if (back instanceof rechunked.chunks.qualified_decl) {
            back.descriptors.push(segm.descriptor);
         } else {
            let back = new rechunked.chunks.qualified_decl();
            back.descriptors.push(segm.descriptor);
            this.chunks.push(back);
         }
         
         for(let aai of segm.array_accesses) {
            let back = new rechunked.chunks.array_slice();
            back.start = aai.start;
            back.count = aai.count;
            this.chunks.push(back);
         }
         
         if (segm.descriptor.transformed_types.length > 0) {
            let list = segm.descriptor.transformed_types;
            let chnk = new rechunked.chunks.transform();
            this.chunks.push(chnk);
            for(let type of list) {
               chnk.types.push(type);
            }
         }
      }
   }
};