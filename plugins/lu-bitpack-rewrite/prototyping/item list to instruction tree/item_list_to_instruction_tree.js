
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
   
   //
   // We have a flat list of serialization items. Each represents the path to a 
   // DECL that will be serialized in its entirety into the bitstream, and the 
   // conditions under which a particular DECL will be serialized (relevant for 
   // UNION_TYPE members). This flat list of paths is the easiest format to work 
   // with for the purposes of dividing the DECLs into fixed-size sectors, but 
   // it's very, very unwieldy for codegen, where we need to generate hierarchies 
   // of code blocks. As such, our ultimate goal is to go from a flat list of 
   // items to a tree. There's some prepwork that we want to do to make that task 
   // easier, though.
   //
   // We're going to convert the flat list of serialization items into a flat list 
   // of "re-chunked items." A re-chunked item is the same path, but divided up at 
   // each point where we'd end up creating a parent/child node hierarchy in our 
   // final tree.
   //
   // In a serialization item, transformations and conditions are annotations that 
   // are applied to individual segments:
   //
   //    a|b|c|d as Type|e|(f == 0) g
   //
   // In a re-chunked item, segments can have different types, and transformations 
   // and conditions are separate segments, of different types than the rest. In 
   // "visual" terms, it looks like this, where each segment has start and end 
   // delimiters, and the delimiter type indicates the segment type.
   //
   //    <a|b|c> [d as Type] <e> (f == 0) <g>
   //
   // Angle brackets are "qualified DECL" segments; square brackets are "transform" 
   // segments; and parentheses are "condition" segments. It is illegal to have 
   // two consecutive "qualified DECL" segments; they should always be merged. 
   // (They represent, after all, the content *between* the points we're dividing 
   // the path at.)
   //
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
         
         compare(other) {
            if (this.decl_path.length != other.decl_path.length)
               return false;
            for(let i = 0; i < this.decl_path.length; ++i)
               if (!this.decl_path[i].compare(other.decl_path[i]))
                  return false;
            return true;
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
      
      //
      // Now that we've converted our serialization items into re-chunked items, we 
      // want to traverse over them and generate a tree of nodes. There are a few 
      // things that we need to keep track of during this process.
      //
      // Suppose we're generating a node tree for these four re-chunked items:
      //
      //    <a|b|c> [d as Type] <e> (f == 0) <g>
      //    <a|b|c> [d as Type] <e> (f == 1) <h>
      //    <a|b|c> [i as Type] <j>
      //    <a|b|c> [i as Type] <k>
      //
      // The final hierarchy that we want to have is:
      //
      //  - transform `a.b.c.d` as `Type`
      //     - switch `f`
      //        - case 0
      //           - `a.b.c.d.e.g`
      //        - case 1
      //           - `a.b.c.d.e.h`
      //  - transform `a.b.c.i` as `Type`
      //     - `a.b.c.i.j`
      //     - `a.b.c.i.k`
      //
      // When we see the first re-chunked item, we'll generate these nodes, which 
      // I'm going to give unique IDs for clarity later on:
      //
      //  - [#0] transform `a.b.c.d` as `Type`
      //     - [#1] switch `f`
      //        - [#2] case 0
      //           - [#3] `a.b.c.d.e.g`
      //
      // We'll store a stack, with which we'll remember the non-leaf nodes we've 
      // created (read: the nodes we may want later re-chunked items to share and 
      // append into) and the chunks which led us to produce them:
      //
      //    NODE     from CHUNK
      //
      //    null     from <a|b|c>
      //    #0       from [d as Type]
      //    null     from <e>
      //    #1       from (f == 0)
      //    #2       from (f == 0)
      //    null     from <g>
      //
      // Then, we'll move on to the second re-chunked item. We'll compare each of 
      // its chunks to the previous item's chunks and find the point of divergence. 
      // In this case, we diverge at node #2 (chunk 3), with a chunk whose LHS is 
      // identical but whose RHS differs. So, we'll clear nodes off the stack:
      //
      //    NODE     from CHUNK
      //
      //    null     from <a|b|c>
      //    #0       from [d as Type]
      //    null     from <e>
      //    #1       from (f == 0)
      //
      // (This step is why the stack has entries for all chunks even though only 
      // some chunks actually generate nodes: it simplifies the process of finding 
      // the point of divergence between two re-chunked items' chunks, and then 
      // matching that to a point of divergence (from which to clear) on the stack.)
      //
      // Then, we'll examine the current re-chunked item at the point of divergence, 
      // and create new nodes as appropriate, inserting into the last shared node 
      // while also updating the stack. This will produce the following hierarchy 
      // and stack.
      //
      //  - [#0] transform `a.b.c.d` as `Type`
      //     - [#1] switch `f`
      //        - [#2] case 0
      //           - [#3] `a.b.c.d.e.g`
      //        - [#4] case 1
      //           - [#5] `a.b.c.d.e.h`
      // 
      //    NODE     from CHUNK
      //
      //    null     from <a|b|c>
      //    #0       from [d as Type]
      //    null     from <e>
      //    #1       from (f == 0)
      //    #4       from (f == 1)
      //    null     from <h>
      //
      // Finally, we come to the third re-chunked item. Again, we clear nodes off 
      // the stack at and past the point of divergence:
      //
      //    NODE     from CHUNK
      //
      //    null     from <a|b|c>
      //
      // The stack no longer has any nodes, so we append to the root.
      //
      //  - [#0] transform `a.b.c.d` as `Type`
      //     - [#1] switch `f`
      //        - [#2] case 0
      //           - [#3] `a.b.c.d.e.g`
      //        - [#4] case 1
      //           - [#5] `a.b.c.d.e.h`
      //  - [#6] transform `a.b.c.i` as `Type`
      //     - [#7] `a.b.c.i.j`
      // 
      //    NODE     from CHUNK
      //
      //    null     from <a|b|c>
      //    #6       from [i as Type]
      //    null     from <j>
      //
      // The fourth note is similar. First, we clear non-matching stack chunks:
      // 
      //    NODE     from CHUNK
      //
      //    null     from <a|b|c>
      //    #6       from [i as Type]
      //
      // And then we append:
      //
      //  - [#0] transform `a.b.c.d` as `Type`
      //     - [#1] switch `f`
      //        - [#2] case 0
      //           - [#3] `a.b.c.d.e.g`
      //        - [#4] case 1
      //           - [#5] `a.b.c.d.e.h`
      //  - [#6] transform `a.b.c.i` as `Type`
      //     - [#7] `a.b.c.i.j`
      //     - [#7] `a.b.c.i.k`
      // 
      //    NODE     from CHUNK
      //
      //    null     from <a|b|c>
      //    #6       from [i as Type]
      //    null     from <k>
      //
      let root  = new container();
      {
         let stack = []; // { chunk: prev_chunk, node: generated_node }
         //
         // The stack is what we use to track and reuse hierarchical nodes from the 
         // previous re-chunked list items. It typically contains one entry per chunk, 
         // mapping chunks to the hierarchical nodes they've generated (if any). There 
         // are two exceptions:
         //
         //  - Condition chunks generate two nodes. The first is the `branch` node 
         //    and the second is an inner `container` node. You could think of this as 
         //    storing a `switch` and a `case`.
         //
         //  - QualifiedDecls don't generate container nodes; they only generate a leaf 
         //    node. Therefore, stack entries for QualifiedDecls store a `null` node.
         //
         // The ultimate goal of the stack is to deal with situations like the following, 
         // by having them share nodes:
         //
         //    a|b as Type|c
         //    a|b as Type|d // `c` and `d` should be children of the same node
         //
         //    a|b
         //    a|(b == 0) c
         //    a|(b == 1) d // `c` and `d` should go under different "cases" nodes within 
         //                 // the same "switch" node.
         //
         
         const size = rechunked_items.length;
         for(let i = 0; i < size; ++i) {
            let item    = rechunked_items[i];
            let j       = 0;
            let switch_matched = false;
            if (i > 0) {
               let prev = rechunked_items[i - 1];
               
               //
               // Let's compare the current node to the previous node, and pop any stack 
               // entries that no longer match.
               //
               // Here, `j` will become the index of the diverging chunk, while `last` 
               // will become the index of the diverging stack entry. The `switch_matched` 
               // bool will allow us to reuse a "switch" node even if we need a new "case" 
               // node.
               //
               let last = 0;
               for(; j < prev.length && j < item.length; ++last, ++j) {
                  switch_matched = false;
                  let chunk_a = prev[j];
                  let chunk_b = item[j];
                  if (chunk_a.constructor != chunk_b.constructor)
                     break;
                  if (chunk_a instanceof serialization_item.condition) {
                     //
                     // We're comparing condition chunks. If the LHS and the RHS both 
                     // fail to match, then we want to pop the "switch" and "case" nodes 
                     // from the stack. However, if the LHS compares equal, then we want 
                     // to keep the "switch" node, pop only the "case" node, and make a 
                     // new "case" node. By the time we decide to make a new node, the 
                     // `switch_matched` bool should be `true` if we popped a "case" but 
                     // not a "switch."
                     //
                     if (chunk_a.compare_lhs(chunk_b)) {
                        switch_matched = true;
                        //
                        // If the "switch" and "case" nodes both match, increment `last` 
                        // twice (here and as part of the for-loop); if only the "switch" 
                        // node matches, increment `last` once (here, and then we break 
                        // out of the loop).
                        //
                        ++last;
                     }
                  }
                  if (!chunk_a.compare(chunk_b))
                     break;
                  
                  switch_matched = false;
               }
               if (j < prev.length) {
                  stack.splice(last, stack.length - last); // remove from k onward
                  if (stack.length > 0)
                     //
                     // The stack should never end with a leaf node after we've clipped 
                     // off diverging entries. The only way that would ever happen is if 
                     // there were two consecutive QualifiedDecls in the re-chunked item, 
                     // which would imply a mistake made while re-chunking.
                     //
                     console.assert(stack[stack.length - 1] !== null);
               }
            }
            
            // j == point of divergence
            
            // This is something unique to the JavaScript implementation, and will likely 
            // have to change when we go back to C++: we combine QualifiedDecls across the 
            // entire path in order to form the final "leaf" path. For example:
            //
            //    <a|b|c> [d as Type] <e>
            //
            // We'll create a transformation node for `a.b.c.d`, and then we create a leaf 
            // node for `a.b.c.d.e`. This is done in JavaScript so that the visual output 
            // can show the data path in an intuitive way.
            //
            // In C++, we'll likely want instruction nodes that can refer to an object to 
            // do so using a `decl_pair`. For transformation nodes, we'll want to create 
            // a pair of `VAR_DECL`s early (these will be the function-local variables 
            // that hold the tarnsformed object), and then any data members or arrray 
            // accesses will be relevant to those. In other words, where JavaScript wants 
            // `a.b.c.d.e`, the C++ code will want `__transformed_d.e`, and so all uses 
            // of this `decl_path` variable will need to work differently.
            //
            // I think what we'll want to do in C++ is glue from the last transformation 
            // node in the path onward. So:
            //
            //    <a|b|c> [d as Type] <e> (f == 0) <g|h>
            //    <t> [u as Type] <v> [w as Type] <x> (y == 0) <z>
            //
            // These would produce, in C++, the following "decl paths" tha would become 
            // leaf `single` nodes:
            //
            //    __transformed_d.e.g.h
            //    __transformed_w.x.z
            //
            // The caveat to this, of course, is that the transformation node which produces 
            // `__transformed_d` would want to be aware of `a.b.c.d`, and the transformation 
            // node which produces `__transformed_w` would similarly want to be aware of 
            // `__transformed_u.v.w`. So it's not enough to just forego this loop entirely 
            // and only build a "decl path" when it's time to add a leaf node; we need to 
            // instead track the "decl path" throughout both loops below, but when we see a 
            // transformation node, clear the path and point it at the transformation node's 
            // output variable. (When we're *making* a transformation node, we'd feed the 
            // existing decl path plus the to-be-transformed DECL in, and *then* clear and 
            // replace the decl path.)
            //
            let decl_path = [];
            for(let k = 0; k < j; ++k) {
               let chunk = item[k];
               if (chunk instanceof QualifiedDecl)
                  decl_path = decl_path.concat(chunk.decl_path);
            }
            
            for(; j < item.length; ++j) {
               
               // Find the last non-null node on the stack. If there isn't one, then use 
               // the root node (which, to keep the above processes simple, is not tracked 
               // on the stack).
               let parent = null;
               {
                  let n = stack.findLastIndex(function(e) { return e.node != null; });
                  if (n < 0)
                     parent = root;
                  else
                     parent = stack[n].node;
               }
               
               let chunk = item[j];
               if (chunk instanceof serialization_item.condition) {
                  let node;
                  if (switch_matched) {
                     //
                     // We popped a "case" node off the stack, but are reusing the "switch" 
                     // "switch" node. We want to create just a new "case."
                     //
                     // NOTE: We wouldn't need a `switch_matched` bool if we instead had a 
                     // separate node class for "case" nodes, as we could just do a type 
                     // check on `parent`.
                     //
                     node = parent;
                     console.assert(node instanceof branch);
                  } else {
                     //
                     // We're not reusing a "switch" node, so we need to create a new one 
                     // of those, and then create a "case" node for it.
                     //
                     node = new branch();
                     node.condition_operand = chunk.segments;
                     parent.instructions.push(node);
                     stack.push({ chunk: chunk, node: node });
                  }
                  
                  let bran = new container();
                  node.by_value[chunk.value] = bran;
                  stack.push({ chunk: chunk, node: bran });
                  //
                  // Clear the "reuse a switch" flag before we move on.
                  //
                  switch_matched = false;
                  continue;
               }
               console.assert(!switch_matched);
               
               if (chunk instanceof QualifiedDecl) {
                  decl_path = decl_path.concat(chunk.decl_path);
                  
                  if (j == item.length - 1) {
                     //
                     // We should only create a node in response to a QualifiedDecl chunk if 
                     // that chunk is the endcap chunk, because these nodes are leaf nodes.
                     //
                     // Conditions will generally produce a path that looks like this:
                     //
                     //    a|data|(a.tag == 0) x
                     //
                     // Or, more straightforwardly:
                     //
                     //    if (a.tag == 0)
                     //       __stream(a.data.x);
                     //
                     // When that path is re-chunked, we end up with:
                     //
                     //  - QualifiedDecl: a.data
                     //  - Conditional:   a.tag == 0
                     //  - QualifiedDecl: x
                     //
                     // We need to stitch QualifiedDecls together to form the final leaf node 
                     // with path `a.data.x`.
                     //
                     let node = new single();
                     node.object = decl_path;
                     parent.instructions.push(node);
                  }
                  stack.push({ chunk: chunk, node: null });
                  continue;
               }
               if (chunk instanceof TransformList) {
                  let node = new transform();
                  node.object = decl_path;
                  node.types  = chunk.types;
                  parent.instructions.push(node);
                  stack.push({ chunk: chunk, node: node });
                  continue;
               }
               console.assert(false); // unreachable
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
               
               i_ul = document.createElement("ul");
               b_li.append(i_ul);
               
               let list = node.by_value[cv].instructions;
               for(let child of list)
                  _dump_node(child, i_ul);
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