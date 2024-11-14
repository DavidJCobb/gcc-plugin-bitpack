
function item_list_to_instruction_tree(items) {
   /*
   
      TODO: Hm... Maybe split each item at any point where it'd have a 
      block? That is: split at array slices, at conditions, and at any 
      transforms?
      
         a|b|(c == 0) d
         a|b|(c == 1) e|f
         a|b|(c == 1) e|g
         a|b|(c == 2) h
         a|i as J|k
         a|i as J|l
         a|m as J|k
         a|m as J|l
         a|n as K
         a|o
         a|p|(o == 0) q|r
         a|p|(o == 0) q|(r == 0) s
         a|p|(o == 0) q|(r == 0) t
         
      Split to...
      
         a|b|   (c == 0)   d
         a|b|   (c == 1)   e|f
         a|b|   (c == 1)   e|g
         a|b|   (c == 2)   h
         a|     i as J    |k
         a|     i as J    |l
         a|     m as J    |k
         a|     m as J    |l
         a|     n as K
         a|o
         a|p|   (o == 0)   q|r
         a|p|   (o == 0)   q|    (r == 0)   s
         a|p|   (o == 0)   q|    (r == 0)   t
      
      Then we coalesce at the split points, I think.
      
         (c == 0)
            a|b
         (c == 1)
            a|b|e|f
            a|b|e|g
         (c == 2)
            a|b|h
         J
            a|i|k
            a|i|l
         J
            a|m|k
            a|m|l
         K
            a|n
         a|o
         (o == 0)
            a|p|q|r
            (r == 0)
               a|p|q|s
               a|p|q|t
   
      We could literally build this as a tree and then mold it into the 
      shape of instructions. Alternatively, we can build the instruction 
      tree directly: iterate through the item list, maintaining a stack 
      of the conditions and transformations we've seen.
      
      struct stack_entry {
         size_t depth; // this entry is from the N-th segment of a seen path
         std::variant<
            condition,        // condition
            std::vector<type> // transformed types
         > data;
         std::vector<instructions::base*> instructions;
      };
      
      The coalesced tree shown above is almost what we want. There's one 
      more thing:
      
         (c ?
            0:
               a|b
            1:
               a|b|e|f
               a|b|e|g
            2:
               a|b|h
         )
         J (
            a|i|k
            a|i|l
         )
         J (
            a|m|k
            a|m|l
         )
         K (
            a|n
         )
         a|o
         (o ?
            0:
               a|p|q|r
               (r ?
                  0:
                     a|p|q|s
                     a|p|q|t
               )
         )
      
   */
   {
      // using TransformList = Array<string>;
      // using QualifiedDecl = Array<serialization_item.basic_segment>;
      // using Condition     = serialization_item.condition;
      //
      // using Chunk = Array<QualifiedDecl | Condition | TransformList>;
      // 
      // Goal is to split each serialization item into an Array<Chunk>.
      //
      class TransformList {
         constructor(l) {
            this.types     = l || [];
            this.decl_path = null;
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
      class QualifiedDecl {
         constructor() {
            this.decl_path = []; // Array<serialization_item.basic_segment>
         }
         append(segm) {
            this.decl_path.push(segm);
         }
      };
      
      let rechunked_items = [];
      for(let item of items) {
         let rech = [];
         for(let segm of item.segments) {
            // Condition
            if (segm.condition)
               rech.push(segm.condition);
            
            // QualifiedDecl
            let basic = new serialization_item.basic_segment();
            basic.object_name    = segm.object_name;
            basic.array_accesses = segm.array_accesses;
            //
            let back = null;
            if (rech.length > 0)
               back = rech[rech.length - 1];
            //
            if (back instanceof QualifiedDecl) {
               back.append(basic);
            } else {
               let qd = new QualifiedDecl();
               qd.append(basic);
               rech.push(qd);
            }
            
            // TransformList
            if (segm.transformations.length > 0) {
               let decl_path = [];
               for(let prior of rech) {
                  if (prior instanceof QualifiedDecl) {
                     decl_path = decl_path.concat(prior.decl_path);
                  } else if (prior instanceof TransformList) {
                     decl_path = prior.decl_path;
                  }
               }
               let tran = new TransformList(segm.transformations);
               tran.decl_path = decl_path;
               rech.push(tran);
            }
         }
         rechunked_items.push(rech);
      }
      console.log("Rechunked items:");
      console.log(rechunked_items);
      //
      // Debugprint.
      //
      {
         let ul = document.getElementById("out-rechunked");
         ul.replaceChildren();
         
         for(let item of rechunked_items) {
            let li = document.createElement("li");
            ul.append(li);
            for(let chunk of item) {
               let span = document.createElement("span");
               if (chunk instanceof serialization_item.condition) {
                  span.className = "condition";
                  span.textContent = chunk.to_string();
               } else if (chunk instanceof QualifiedDecl) {
                  span.className = "qualified-decl";
                  let text = "";
                  for(let decl of chunk.decl_path) {
                     if (text)
                        text += '.';
                     text += decl.to_string();
                  }
                  span.textContent = text;
               } else if (chunk instanceof TransformList) {
                  span.className = "transform-list";
                  let text = "";
                  for(let type of chunk.types)
                     text += " as " + type;
                  span.textContent = text;
               }
               li.append(span);
            }
         }
      }
      
      let root  = new container();
      let depth = [];
      let nodes = [];
      let cnd_v = [];
      for(let item of rechunked_items) {
         let item_containers = [];
         for(let chunk of item) {
            if (chunk instanceof TransformList) {
               item_containers.push(chunk);
            } else if (chunk instanceof serialization_item.condition) {
               item_containers.push(chunk);
            }
         }
         
         let common = 0;
         for(let i = 0; i < item_containers.length && i < depth.length; ++i, ++common) {
            let a = depth[i];
            let b = item_containers[i];
            if (a.constructor != b.constructor)
               break;
            if (a instanceof TransformList) {
               if (!a.compare(b))
                  break;
            } else if (a instanceof serialization_item.condition) {
               if (!a.compare(b))
                  break;
            }
         }
         if (common < depth.length) {
            let diff = depth.length - common;
            depth.splice(-diff, diff); // delete last N items
            nodes.splice(-diff, diff);
            cnd_v.splice(-diff, diff);
         }
         
         let i = 0;
         if (item_containers.length > 0) {
            i = item.indexOf(item_containers[common - 1]);
            if (i < 0)
               i = 0;
         }
         for(; i < item.length; ++i) {
            let chunk = item[i];
            
            let node_back  = null;
            let cnd_v_back = null;
            if (nodes.length > 0) {
               node_back  = nodes[nodes.length - 1];
               cnd_v_back = cnd_v[nodes.length - 1];
            }
            
            let node;
            let node_v;
            let is_container = false;
            if (chunk instanceof TransformList) {
               is_container = true;
               node = new transform();
               node.object = chunk.decl_path;
               node.types  = chunk.types;
            } else if (chunk instanceof serialization_item.condition) {
               is_container = true;
               node = new branch();
               node.condition_operand = chunk.segments;
               node_v = chunk.value;
            } else if (chunk instanceof QualifiedDecl) {
               node = new single();
               node.object = chunk.decl_path;
            } else {
               throw new Error("unreachable");
            }
            
            if (node_back) {
               if (node_back instanceof branch) {
                  let list = node_back.by_value[node_v];
                  if (!list)
                     list = node_back.by_value[node_v] = [];
                  list.push(node);
               } else {
                  if (!node_back.instructions)
                     throw new Error("not a valid parent node");
                  node_back.instructions.push(node);
               }
            } else {
               root.instructions.push(node);
            }
            if (is_container) {
               depth.push(chunk);
               nodes.push(node);
               cnd_v.push(node_v);
            }
         }
      }
      console.log(root);
      //
      // Debugprint.
      //
      {
         let ul = document.getElementById("out-treed");
         ul.replaceChildren();
         
         function _dump_branch_children(node, ul) {
            let li = document.createElement("li");
            ul.append(li);
            li.className = "condition";
            
            let span = document.createElement("span");
            li.append(span);
            {
               let text = "";
               for(let segm of node.condition_operand) {
                  if (text)
                     text += '.';
                  text += segm.to_string();
               }
               span.textContent = "branch on: " + text;
            }
            
            let b_ul = document.createElement("ul");
            li.append(b_ul);
            for(let cv of Object.keys(node.by_value)) {
               let b_li = document.createElement("li");
               b_li.className = "branch-condition-value";
               b_ul.append(b_li);
               
               let span = document.createElement("span");
               b_li.append(span);
               span.textContent = "== " + cv;
               
               b_ul = document.createElement("ul");
               b_li.append(b_ul);
               
               let list = node.by_value[cv];
               for(let child of list)
                  _dump_node(child, b_ul);
            }
         }
         function _dump_node(node, ul) {
            if (node instanceof branch) {
               _dump_branch_children(node, ul);
               return;
            }
            
            let li = document.createElement("li");
            ul.append(li);
            if (node instanceof transform) {
               li.className = "transform";
            }
            
            let span = document.createElement("span");
            li.append(span);
            if (node instanceof transform) {
               let alts = document.createElement("span");
               alts.className = "qualified-decl";
               {
                  let text = "";
                  for(let segm of node.object) {
                     if (text)
                        text += ".";
                     text += segm.to_string();
                  }
                  alts.textContent = text;
               }
               li.prepend(alts);
               
               let text = "";
               for(let type of node.types)
                  text += " as " + type;
               span.textContent = text;
            } else if (node instanceof single) {
               li.className = "single";
               
               let text = "";
               for(let segm of node.object) {
                  if (text)
                     text += ".";
                  text += segm.to_string();
               }
               span.textContent = text;
            } else if (node == root) {
               span.textContent = "<root>";
            } else {
               span.textContent = "<???>";
            }
            
            if (!node.instructions)
               return;
            b_ul = document.createElement("ul");
            li.append(b_ul);
            for(let child of node.instructions) {
               _dump_node(child, b_ul);
            }
         }
         
         _dump_node(root, ul);
      }
      
      // TODO
   }
   
   
   
   
   //
   // Build a fully-exploded tree, where every path segment becomes 
   // a tree node.
   //
   // EDIT: Hm... No, this isn't quite right.
   //
   
   class ExplodedTreeNode {
      constructor() {
         this.segment  = null;
         this.children = [];
         this.parent   = null;
      }
      append(node) {
         node.parent = this;
         this.children.push(node);
      }
   };
   
   let root  = new ExplodedTreeNode();
   let depth = []; // Array<serialization_item.segment>
   let nodes = []; // one ExplodedTreeNode per depth item
   
   for(let item of items) {
      let common = 0;
      for(let i = 0; i < depth.length; ++common, ++i) {
         if (i >= item.segments.length)
            break;
         let a = depth[i];
         let b = item.segments[i];
         if (!a.compare(b))
            break;
      }
      if (common < depth.length) {
         let diff = depth.length - common;
         depth.splice(-diff, diff);
         nodes.splice(-diff, diff);
      }
      while (depth.length < item.segments.length) {
         let segm = item.segments[depth.length];
         let node = new ExplodedTreeNode();
         node.segment = segm;
         
         depth.push(segm);
         if (nodes.length > 0)
            nodes[nodes.length - 1].append(node);
         else
            root.append(node);
         nodes.push(node);
      }
   }
   console.log(root);
   //
   // Debugprint.
   //
   {
      let ul = document.getElementById("out-fully-exploded");
      ul.replaceChildren();
      
      function recurse(node, dst) {
         let li = document.createElement("li");
         li.textContent = node.segment.to_string();
         dst.append(li);
         if (node.children.length > 0) {
            let ul = document.createElement("ul");
            li.append(ul);
            for(let child of node.children)
               recurse(child, ul);
         }
      }
      
      for(let child of root.children)
         recurse(child, ul);
   }
   
   //
   // Build simplified tree.
   //
   
   
   
   return root;
}