class c_type_dictionary {
   static #instance = null;
   static get() {
      if (this.#instance)
         return this.#instance;
      return (this.#instance = new c_type_dictionary());
   }
   
   constructor() {
      this.types = [];
   }
   setup_builtins() {
      // built-in integer types and common typedefs
      for(const bc of [8, 16, 32, 64]) {
         let type = new c_type();
         type.name   = `int${bc}_t`;
         type.sizeof = bc / 8;
         
         type = new c_type();
         type.name   = `uint${bc}_t`;
         type.sizeof = bc / 8;
         
         // common typedefs
         type = new c_type();
         type.name   = `s${bc}`;
         type.sizeof = bc / 8;
         
         type = new c_type();
         type.name   = `u${bc}`;
         type.sizeof = bc / 8;
      }
      
      // built-in types
      {
         let type = new c_type();
         type.name   = "int";
         type.sizeof = 4;
      }
   }
   clear() {
      this.types = [];
   }
   lookup_name(name) {
      for(let type of this.types)
         if (type.name == name)
            return type;
      return null;
   }
};

class c_decl {
   static #counter = 0;
   
   // Unique ID, so we can tell content-identical DECLs apart.
   #id = -1;
   
   constructor() {
      this.name          = null;
      this.type          = null;
      this.array_extents = [];
      this.attributes    = {};
      
      this.#id = ++c_decl.#counter;
   }
   get id() {
      return this.#id;
   }
   
   get typename() {
      if (this.type instanceof c_type)
         return this.type.name;
      if (this.type + "" === this.type)
         return this.type;
      return "";
   }
   
   get serialized_type() {
      let type = this.type;
      let name = this.attributes["transform_to"];
      if (type && !name) {
         name = type.attributes["transform_to"];
      }
      while (name) {
         type = c_type_dictionary.get().lookup_name(name);
         if (!type)
            throw new Error("transformed type `" + name + "` is not defined");
         name = type.attributes["transform_to"];
      }
      return type;
   }
   
   get serialized_bitcount() {
      let type = this.serialized_type;
      let bc   = type.serialized_bitcount;
      for(let extent of this.array_extents)
         bc *= extent;
      return bc;
   }
   
   #_make_si_segment() {
      let desc = decl_descriptor_dictionary.get().describe(this);
      let segm = new serialization_item.segment();
      segm.descriptor = desc;
      for(let extent of this.array_extents) {
         let aai = new array_access_info();
         aai.start = 0;
         aai.count = extent;
         segm.array_accesses.push(aai);
      }
      return segm;
   }
   as_expanded_serialization_item_list(previous_item) {
      let type = this.serialized_type;
      if (type instanceof c_union) {
         let max_size = type.serialized_bitcount;
         
         let items = [];
         for(let i = 0; i < type.members.length; ++i) {
            let memb = type.members[i];
            let subs = memb.as_expanded_serialization_item_list(null);
            {
               let size = memb.serialized_bitcount;
               if (size < max_size) {
                  let item = new serialization_item();
                  let segm = new serialization_item.segment();
                  segm.padding = max_size - size;
                  item.segments.push(segm);
                  subs.push(item);
               }
            }
            for(let item of subs) {
               let cnd = new serialization_item.condition();
               cnd.segments = previous_item.segments;
               cnd.value    = i;
               item.segments[0].condition = cnd;
            }
            items = items.concat(subs);
         }
         for(let item of items) {
            item.segments.unshift(this.#_make_si_segment());
         }
         return items;
      }
      if (type instanceof c_struct) {
         let items = [];
         for(let i = 0; i < type.members.length; ++i) {
            let memb = type.members[i];
            let prev = null;
            if (memb.type instanceof c_union) {
               prev = items[items.length - 1];
            }
            let subs = memb.as_expanded_serialization_item_list(prev);
            items = items.concat(subs);
         }
         for(let item of items) {
            item.segments.unshift(this.#_make_si_segment());
         }
         return items;
      }
      let item = new serialization_item();
      let segm = this.#_make_si_segment();
      item.segments.push(segm);
      return [ item ];
   }
};

class c_type {
   static #counter = 0;
   
   // Unique ID
   #id = -1;
   
   constructor() {
      this.name       = null;
      this.sizeof     = 0;
      this.attributes = {};
      
      this.#id = ++c_type.#counter;
      c_type_dictionary.get().types.push(this);
   }
   get id() {
      return this.#id;
   }
   
   get serialized_bitcount() {
      return this.sizeof * 8;
   }
};

class c_container extends c_type {
   constructor() {
      super();
      this.members = []; // Array<c_decl>
   }
};
class c_struct extends c_container {
   get serialized_bitcount() {
      let bc = 0;
      for(let decl of this.members) {
         bc += decl.serialized_bitcount;
      }
      return bc;
   }
};
class c_union extends c_container {
   get serialized_bitcount() {
      let bc = 0;
      for(let decl of this.members) {
         bc = Math.max(bc, decl.serialized_bitcount);
      }
      return bc;
   }
};
