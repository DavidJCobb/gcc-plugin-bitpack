
let struct_parser;
{
   function is_alpha(c) {
      let cc = c.charCodeAt(0);
      if (cc >= ("a").charCodeAt(0) && cc <= ("z").charCodeAt(0))
         return true;
      if (cc >= ("A").charCodeAt(0) && cc <= ("Z").charCodeAt(0))
         return true;
      return false;
   }
   function is_digit(c) {
      let cc = c.charCodeAt(0);
      return (cc >= ("0").charCodeAt(0) && cc <= ("9").charCodeAt(0));
   }
   function is_whitespace(c) {
      return (" \n\r\t").indexOf(c) >= 0;
   }
   
   struct_parser = class struct_parser {
      constructor(text) {
         this.text = text;
         this.pos  = 0;
         
         this.results = {
            types:     [],
            variables: [],
         };
      }
      
      lookup_typename(name) {
         for(let type of this.results.types)
            if (type.name == name)
               return type;
         return null;
      }
      
      skip_whitespace() {
         for(; this.pos < this.text.length; ++this.pos) {
            let c = this.text[this.pos];
            if (!is_whitespace(c))
               break;
         }
      }
      
      // Skip any whitespace ahead, and then execute `functor` until it 
      // returns a boolean (non-undefined) value. If it returns true, 
      // then set `this.pos` to the position of the current character. 
      scan(functor) {
         this.skip_whitespace();
         let i = this.pos;
         if (i >= this.text.length)
            return;
         for(; i < this.text.length; ++i) {
            let res = functor(this.text[i]);
            if (res === true) {
               this.pos = i;
               break;
            }
            if (res === false) {
               break;
            }
         }
      }
      
      is_at_effective_end() {
         for(let i = this.pos; i < this.text.length; ++i)
            if (!is_whitespace(this.text[i]))
               return false;
         return true;
      }
      
      // Extract and return a legal C identifier, if one is present, 
      // or null otherwise.
      extract_identifier() {
         let first = true;
         let ident = "";
         this.scan(function(c) {
            if (first) {
               first = false;
               if (!is_alpha(c) && c != '_')
                  return !!ident;
            } else {
               if (!is_alpha(c) && c != '_' && !is_digit(c))
                  return !!ident;
            }
            ident += c;
         });
         if (ident == "")
            return null;
         return ident;
      }
      
      extract_next_token() {
         let i = this.pos;
         for(; i < this.text.length; ++i) {
            let c = this.text[i];
            if (!(" \n\r\t").contains(c))
               break;
         }
         if (i >= this.text.length)
            return null;
         
         let ident = this.extract_identifier();
         if (ident)
            return ident;
         return c;
      }
      
      extract_char(desired) {
         this.skip_whitespace();
         if (this.pos >= this.text.length)
            return false;
         if (this.text[this.pos] != desired)
            return false;
         ++this.pos;
         return true;
      }
      
      //
      // Rules:
      //
      
      extract_integer_literal() {
         let base        = 10;
         let seen_digits = false;
         let seen_sign   = 1;
         let value       = 0;
         this.scan(function (c) {
            if (!seen_digits) {
               if (c == '+') {
                  seen_sign = 1;
                  return;
               }
               if (c == '-') {
                  seen_sign = -seen_sign;
                  return;
               }
               if (is_digit(c)) {
                  seen_digits = true;
                  value = (value * base) + c;
                  return;
               }
            }
            if (!is_digit(c)) {
               return seen_digits;
            }
            value = (value * base) + c;
         });
         if (!seen_digits)
            return null;
         return value * seen_sign;
      }
      
      extract_attribute() {
         this.skip_whitespace();
         
         let prior = this.pos;
         let ident = this.extract_identifier();
         if (ident == "__attribute__") {
            this.skip_whitespace();
            if (this.extract_char("(") && this.extract_char("(")) {
               let name = this.extract_identifier();
               if (name === null)
                  throw new Error("attribute name expected");
               
               let value = null;
               if (this.extract_char("(")) {
                  let token = this.extract_integer_literal();
                  if (token === null) {
                     token = this.extract_identifier();
                     if (token === null) {
                        throw new Error("integer or identifier expected (other attribute args are not supported right now)");
                     }
                  }
                  value = token;
                  if (!this.extract_char(")"))
                     throw new Error("expected ')' to close attribute arguments");
               }
               if (!this.extract_char(')') || !this.extract_char(')'))
                  throw new Error("expected '))' to close attribute");
               return { name: name, value: value };
            }
         }
         this.pos = prior;
         return null;
      }
      
      extract_array_extent() {
         this.skip_whitespace();
         if (!this.extract_char('['))
            return null;
         let extent = this.extract_integer_literal();
         if (extent === null)
            throw new Error("expected integer literal");
         if (!this.extract_char(']'))
            throw new Error("expected ']'");
         return extent;
      }
      
      extract_declaration() {
         let attributes = {};
         {
            let pair = this.extract_attribute();
            while (pair !== null) {
               attributes[pair.name] = pair.value;
               pair = this.extract_attribute();
            }
         }
         
         let token = this.extract_identifier();
         if (token === null)
            return null;
         
         let type;
         if (token == "struct" || token == "union") {
            type = this.extract_tag_declaration(token);
         } else {
            type = this.lookup_typename(token);
            if (!type) {
               type = new c_type();
               type.name = token;
               this.results.types.push(type);
            }
         }
         
         let ident = this.extract_identifier();
         if (ident === null)
            throw new Error("expected identifier");
         
         let decl = new c_decl();
         decl.name       = ident;
         decl.type       = type;
         decl.attributes = attributes;
         
         let rank = this.extract_array_extent();
         while (rank !== null) {
            decl.array_extents.push(rank);
            rank = this.extract_array_extent();
         }
         
         if (!this.extract_char(';'))
            throw new Error("expected ';'");
         
         return decl;
      }
      
      extract_tag_declaration(keyword) {
         let subject    = new (keyword == "struct" ? c_struct : c_union)();
         let attributes = {};
         {
            let pair = this.extract_attribute();
            while (pair !== null) {
               attributes[pair.name] = pair.value;
               pair = this.extract_attribute();
            }
         }
         let tag_name = this.extract_identifier();
         subject.name       = tag_name;
         subject.attributes = attributes;
         
         let definition = this.extract_char("{");
         if (!tag_name && !definition)
            throw new Error("expected '{' to begin definition of anonymous " + keyword);
         
         if (definition) {
            let member = this.extract_declaration();
            while (member != null) {
               subject.members.push(member);
               member = this.extract_declaration();
            }
            
            if (!this.extract_char("}"))
               throw new Error("expected '}'");
         }
         
         let previously_declared = null;
         if (tag_name) {
            previously_declared = this.lookup_typename(tag_name);
            if (previously_declared) {
               if (previously_declared.constructor != subject.constructor)
                  throw new Error("type redeclared with a different keyword");
               subject = previously_declared;
            }
         }
         if (subject !== previously_declared)
            this.results.types.push(subject);
         
         return subject;
      }
      
      //
      
      parse() {
         while (!this.is_at_effective_end()) {
            let type = null;
            
            let token = this.extract_identifier();
            if (token == null)
               throw new Error("expected identifier token");
            
            let must_be_variable = false;
            if (token == "static") {
               must_be_variable = true;
               token = this.extract_identifier();
               if (token == null)
                  throw new Error("expected typename after `static`");
            }
            
            if (token == "struct" || token == "union") {
               type  = this.extract_tag_declaration(token);
            } else {
               type = this.lookup_typename(token);
               if (!type) {
                  type = new c_type();
                  type.name = token;
                  this.results.types.push(type);
               }
            }
            let ident = this.extract_identifier();
            if (ident === null) {
               if (must_be_variable)
                  throw new Error("expected identifier after `static ${token} ...`");
               if (token == "struct" || token == "union") {
                  //
                  // Could be a tag-declaration.
                  //
                  if (!this.extract_char(';')) {
                     let message = `expected identifier or ';' after declaration of type \`${token}${type.name ? " " : ""}${type.name}\``;
                     throw new Error(message);
                  }
                  continue;
               }
            }
            let decl = new c_decl();
            decl.name = ident;
            decl.type = type;
            this.results.variables.push(decl);
            if (!this.extract_char(";"))
               throw new Error("expected ';' after declaration of variable `" + ident + '`');
         }
      }
   };
}
