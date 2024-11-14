
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
      
      to_string() {
         let out = this.object_name;
         for(let a of this.array_accesses) {
            if (a.count > 1) {
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
         text = text.substring(0, text.length - 1);
         text = text.split("][");
         for(let frag of text) {
            frag = frag.split(":");
            
            let item = new array_access_info();
            item.start = +frag[0];
            this.array_accesses.push(item);
            if (frag[1]) {
               let end = +item[1];
               item.count = end - item.start;
            }
         }
      }
   };
   static segment = class segment extends this.basic_segment {
      constructor() {
         super();
         this.transformations = []; // Array<String>
      }
   };
   static condition = class condition {
      constructor() {
         this.segments = []; // Array<basic_segment>
         this.value    = 0;
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
   
   constructor() {
      this.conditions = [];
      this.segments   = []; // Array<segment>
   }
   
   from_string(s) {
      //
      // Handle leading conditions.
      //
      if (s.startsWith("if (")) {
         let match = s.match(/^if \(([^\(\)]+ == -?\d+)( && [^\(\)]+ == -?\d+)*\) /);
         let items = (match[1] + (match[2] || "")).split(" && ");
         for(let item of items) {
            item = item.split(" == ");
            
            let cnd = new serialization_item.condition();
            this.conditions.push(cnd);
            cnd.value = +item[1];
            
            item = item[0].split(".");
            for(let e of item) {
               let segm  = new serialization_item.basic_segment();
               cnd.segments.push(segm);
               segm.from_string(e);
            }
         }
         s = s.substring(match[0].length);
      }
      
      /*
      
         path → transformed-path | basic-segment-list
         
            transformed-path → transformed-segment-list ('.' basic-segment-list)*
         
            transformed-segment-list → '(' transformed-type-list path ')'
            
               transformed-type-list → (transformed-type ' ')+
               
                  transformed-type → '(' typename ')'
         
            basic-segment-list → basic-segment ('.' basic-segment)*
      
      */
      let i = 0;
      
      let extract_path;
      let extract_trans_path;
      let extract_trans_segm_list;
      let extract_trans_type_list;
      let extract_trans_type;
      let extract_basic_segm_list;
      
      extract_path = function() {
         let result = extract_trans_path();
         if (result === null)
            result = { path: extract_basic_segm_list() };
         return result;
      };
      extract_trans_path = function() {
         let result = extract_trans_segm_list();
         if (result === null)
            return null;
         let path = "";
         if (s[i] == ".") {
            ++i;
            path = extract_basic_segm_list();
         }
         return {
            nest: result,
            path: path
         };
      };
      extract_trans_segm_list = function() {
         if (s[i] != "(")
            return null;
         ++i;
         let types = extract_trans_type_list();
         let nest  = extract_path();
         if (s[i] != ")")
            throw new Error("syntax error in path");
         ++i;
         return {
            types: types,
            nest:  nest,
         };
      };
      extract_trans_type_list = function() {
         let result = [];
         let type   = extract_trans_type();
         while (type !== null) {
            result.push(type);
            if (s[i] == " ")
               ++i;
            type = extract_trans_type();
         }
         return result;
      };
      extract_trans_type = function() {
         if (s[i] != "(" || s[i + 1] == "(")
            return null;
         let n = s.indexOf(")", i);
         if (n < 0)
            return null;
         let type = s.substring(i + 1, n);
         i = n + 1;
         return type;
      };
      extract_basic_segm_list = function() {
         let a = s.indexOf(")", i);
         if (a >= 0) {
            let text = s.substring(i, a);
            i = a;
            return text;
         }
         let text = s.substring(i);
         i = s.length;
         return text;
      };
      
      let extracted = extract_path();
      
      let self = this;
      function _walk(item) {
         if (item + "" === item) {
            item = item.split(".");
            for(let frag of item) {
               let segm  = new serialization_item.segment();
               self.segments.push(segm);
               segm.from_string(frag);
            }
            return;
         }
         if (item.nest) {
            _walk(item.nest);
            
            let back = self.segments[self.segments.length - 1];
            let list = item.types;
            if (list) {
               for(let i = 0; i < list.length; ++i)
               back.transformations.push(list[i] + "");
            }
         }
         if (item.path)
            _walk(item.path);
      }
      _walk(extracted);
   }
   
   to_string() {
      let out = "";
      if (this.conditions.length > 0) {
         out += "if (";
         for(let i = 0; i < this.conditions.length; ++i) {
            let cnd = this.conditions[i];
            if (i > 0)
               out += " && ";
            out += cnd.to_string();
         }
         out += ") ";
      }
      
      let path = "";
      for(let i = 0; i < this.segments.length; ++i) {
         let segm = this.segments[i];
         if (segm.transformations.length > 0) {
            let prefix = "(";
            for(let tran of segm.transformations) {
               prefix += "(" + tran + ") ";
            }
            path = prefix + path;
         }
         for(let i = 0; i < this.segments.length; ++i) {
            if (i > 0)
               path += ".";
            path += this.segments[i].to_string();
         }
         if (segm.transformations.length > 0) {
            path += ")";
         }
      }
      
      return out + path;
   }
};