
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
      this.object = null; // Array<serialization_item.basic_segment>
   }
};

class indexed_group extends container {
   constructor() {
      super();
      this.array = {
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
      this.object = null; // Array<serialization_item.basic_segment>
   }
};

class branch extends instruction {
   constructor() {
      super();
      this.condition_operand = null; // Array<serialization_item.basic_segment>
      this.by_value = {};
      
      // this.by_value[n] = Array<container>
   }
};