
class decl_descriptor {
   static #counter = 0;
   
   // Unique ID, so we can tell content-identical DECLs apart.
   #id = -1;
   
   constructor(decl) {
      this.decl              = decl;
      this.transformed_types = [];
      
      {
         let tt = null;
         if (decl.type) {
            let value = decl.type.attributes["transform_to"];
            if (value)
               tt = value;
         }
         let value = decl.attributes["transform_to"];
         if (value)
            tt = value;
         
         if (tt) {
            let type = c_type_dictionary.get().lookup_name(tt);
            if (!type)
               throw new Error("typename '" + tt + "' is undefined");
            this.transformed_types.push(type);
            
            {
               let name = type.attributes["transform_to"];
               while (name) {
                  type = c_type_dictionary.get().lookup_name(name);
                  if (!type)
                     throw new Error("typename '" + name + "' is undefined");
                  this.transformed_types.push(type);
                  name = type.attributes["transform_to"];
               }
            }
         }
      }
      
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
   
   clear() {
      this.descriptors = [];
   }
   
   describe(decl) {
      console.assert(!!decl);
      for(let desc of this.descriptors)
         if (desc.decl == decl)
            return desc;
      let desc = new decl_descriptor(decl);
      this.descriptors.push(desc);
      return desc;
   }
};