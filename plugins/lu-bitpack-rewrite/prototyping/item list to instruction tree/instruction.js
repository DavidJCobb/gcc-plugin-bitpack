
class instruction {
};

class root extends instruction {
   constructor() {
      super();
      this.instructions = [];
   }
};

class single extends instruction {
   constructor() {
      super();
      this.object = null; // Array<serialization_item.basic_segment>
   }
};

class indexed_group extends instruction {
   constructor() {
      super();
      this.array = {
         object: null, // Array<serialization_item.basic_segment>
         start:  0,
         count:  0,
      };
      this.instructions = [];
   }
};

class transform extends instruction {
   constructor() {
      super();
      this.types  = [];
      this.object = null; // Array<serialization_item.basic_segment>
      this.instructions = [];
   }
};

class branch extends instruction {
   constructor() {
      super();
      this.condition_operand = null; // Array<serialization_item.basic_segment>
      this.by_value = {};
      
      // this.by_value[n] = Array<instruction>
   }
};