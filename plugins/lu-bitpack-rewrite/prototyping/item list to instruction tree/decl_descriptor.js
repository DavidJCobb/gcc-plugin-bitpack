
class decl_descriptor {
   static #counter = 0;
   
   // Unique ID, so we can tell content-identical DECLs apart.
   #id = -1;
   
   constructor(decl) {
      this.decl              = decl;
      this.transformed_types = [];
      
      this.#id = ++this.constructor.#counter;
   }
   get id() {
      return this.#id;
   }
   
   get name() {
      return this.decl.name;
   }
};

class decl_descriptor_dictionary {
   constructor() {
      this.descriptors = [];
   }
   
   static #instance = null;
   static get() {
      if (this.#instance)
         return this.#instance;
      return (this.#instance = new decl_descriptor_dictionary());
   }
   
   describe(decl) {
      for(let desc of this.descriptors)
         if (desc.decl == decl)
            return desc;
      let desc = new decl_descriptor(decl);
      this.descriptors.push(desc);
      return desc;
   }
};