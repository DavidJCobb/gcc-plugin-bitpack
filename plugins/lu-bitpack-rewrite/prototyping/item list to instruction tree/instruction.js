
let instructions = {};

// abstract
instructions.base = class base {
   constructor() {
   }
};

// Represents a single value that should be serialized whole. This could 
// be an entire array, an entire struct, or a "primitive" value such as 
// an integer, opaque buffer, or string.
instructions.single = class single extends instructions.base {
   constructor() {
      super();
      this.value = null; // value_path
   }
};

// abstract
instructions.container = class container extends instructions.base {
   constructor() {
      super();
      this.instructions = [];
   }
};

// Represents access into an array via a for-loop. This doesn't represent 
// actually serializing an array element (because we may instead serialize 
// members of that element); it only represents the for-loop itself.
//
// NOTE: `array.count` may be 1. In that case, we ideally wouldn't generate 
// a for-loop; we'd "unroll" ourselves. However, child and descendant nodes 
// may refer to our loop-index VAR_DECLs; we'd need some sort of state that 
// recursive codegen calls could use to say, "Oh, that VAR_DECL should be 
// the integer constant X. I will just access X instead."
instructions.array_slice = class array_slice extends instructions.container {
   static loop_index_type = (function() {
      let type = new c_type();
      type.name = "int";
      return type;
   })();
   
   constructor() {
      super();
      this.array = {
         value: null, // value_path to array
         start: 0,
         count: 0,
      };
      
      this.loop_index_decl = new c_decl();
      this.loop_index_decl.name = "__i_" + this.loop_index_decl.id;
      this.loop_index_decl.type = this.constructor.loop_index_type;
      
      this.loop_index_desc = decl_descriptor_dictionary.get().describe(this.loop_index_decl);
   }
};

instructions.transform = class transform extends instructions.container {
   constructor(type_list) {
      super();
      this.types = type_list;
      
      this.to_be_transformed_value = null; // value_path
      
      this.transformed_decl      = new c_decl();
      this.transformed_decl.name = "__transformed_" + this.transformed_decl.id;
      this.transformed_decl.type = type_list[type_list.length - 1];
      
      this.transformed_desc = decl_descriptor_dictionary.get().describe(this.transformed_decl);
   }
};

instructions.union_switch = class union_switch extends instructions.base {
   constructor() {
      super();
      
      this.condition_operand = null; // value_path
      
      // std::unordered_map<intmax_t, std::unique_ptr<union_case>>
      this.by_value = {};
   }
};

instructions.union_case = class union_case extends instructions.container {
   constructor() {
      super();
   }
};