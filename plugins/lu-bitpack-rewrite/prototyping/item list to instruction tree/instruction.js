
class instruction {
};

class container extends instruction {
   constructor() {
      super();
      this.instructions = [];
   }
};

class single extends instruction {
   constructor() {
      super();
      
      // In C++, this is a list of `decl_pair`s representing the path to the 
      // value to serialize.
      this.object = null; // Array<serialization_item.basic_segment>
   }
};

// We may not actually need this. Pretend for a moment that everywhere I 
// wrote `decl_pair` in this file, I was referring instead to a struct that 
// held `decl_descriptor` pointers *and* a list of `array_access_info`. We 
// can recursively handle for-loops when we do codegen.
class indexed_group extends container {
   constructor() {
      super();
      this.array = {
         
         // In C++, we'd want this to be a list of `decl_pair`s representing 
         // the path to the array whose elements we'll serialize in a loop.
         object: null, // Array<serialization_item.basic_segment>
         
         start:  0,
         count:  0,
      };
   }
};

class transform extends container {
   constructor() {
      super();
      this.types  = [];
      
      // In C++, we'd want this to be:
      // 
      //  - A list of `decl_pair`s representing the to-be-transformed value
      // 
      //  - A pair of `VAR_DECL`s representing function-local variables to hold 
      //    the final transformed value
      // 
      //  - A `decl_pair` wrapping those `VAR_DECL`s; member access will be 
      //    relative to this `decl_pair`
      // 
      this.object = null; // Array<serialization_item.basic_segment>
   }
};

// TODO: Rename to `union_switch`
// TODO: Create a `union_case` class to match, and use that for by-value nodes
//       instead of generic `container` nodes
class branch extends instruction {
   constructor() {
      super();
      
      // In C++, this will be a list of `decl_pair`s representing the path to the 
      // tag value that we'll be checking as a condition.
      this.condition_operand = null; // Array<serialization_item.basic_segment>
      
      this.by_value = {};
      
      // this.by_value[n] = Array<container>
   }
};