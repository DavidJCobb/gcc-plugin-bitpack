
function rechunked_items_to_instruction_tree(items) {
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
   let root = new instructions.container();
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
      
      for(let i = 0; i < items.length; ++i) {
         let item      = items[i];
         let diverge_c = 0;
         if (i > 0) {
            let prev = items[i - 1];
            //
            // Let's compare the current node to the previous node, and pop any stack 
            // entries that no longer match.
            //
            let diverge_s = 0;
            let end       = Math.min(prev.chunks.length, item.chunks.length);
            for(; diverge_c < end; ++diverge_c, ++diverge_s) {
               let chunk_a = prev.chunks[diverge_c];
               let chunk_b = item.chunks[diverge_c];
               if (chunk_a.constructor != chunk_b.constructor)
                  break;
               if (chunk_a instanceof rechunked.chunks.condition) {
                  //
                  // We're comparing condition chunks. If the LHS and the RHS both 
                  // fail to match, then we want to pop the "switch" and "case" nodes 
                  // from the stack. We'll then later make a new "switch" node and a 
                  // new "case" node. However, if the LHS compares equal, then we want 
                  // to keep the "switch" node, pop only the "case" node, and make a 
                  // new "case" node later on.
                  //
                  if (chunk_a.compare_lhs(chunk_b))
                     ++diverge_s;
               }
               if (!chunk_a.compare(chunk_b))
                  break;
            }
            
            if (diverge_c < prev.chunks.length) {
               stack.splice(diverge_s, stack.length - diverge_s); // remove from `diverge_s` onward
               if (stack.length > 0) {
                  //
                  // The stack should never end with a leaf node after we've clipped 
                  // off diverging entries. The only way that would ever happen is if 
                  // there were two consecutive QualifiedDecls in the re-chunked item, 
                  // which would imply a mistake made while re-chunking.
                  //
                  let back = stack[stack.length - 1].node;
                  console.assert(back instanceof instructions.container || back instanceof instructions.union_switch);
               }
            }
         }
         
         let value = new value_path();
         for(let entry of stack) {
            if (entry.chunk instanceof rechunked.chunks.qualified_decl) {
               for(let desc of entry.chunk.descriptors)
                  value.access_member(desc);
               continue;
            }
            console.assert(entry.node !== null);
            if (entry.node instanceof instructions.union_switch) {
               continue;
            }
            if (entry.node instanceof instructions.union_case) {
               continue;
            }
            if (entry.node instanceof instructions.transform) {
               value.replace(entry.node.transformed_desc);
               continue;
            }
            if (entry.node instanceof instructions.array_slice) {
               value.access_array_element(entry.node.loop_index_desc);
               continue;
            }
            console.assert(false, "unreachable");
         }
         
         for(let j = diverge_c; j < item.chunks.length; ++j) {
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
            
            let chunk = item.chunks[j];
            console.assert(!!chunk);
            
            const is_last_chunk = j == item.chunks.length - 1;
            
            if (chunk instanceof rechunked.chunks.condition) {
               let node;
               if (parent instanceof instructions.union_switch) {
                  //
                  // We popped a "case" node off the stack, but are reusing the "switch" 
                  // "switch" node. We want to create just a new "case."
                  //
                  // NOTE: We wouldn't need a `switch_matched` bool if we instead had a 
                  // separate node class for "case" nodes, as we could just do a type 
                  // check on `parent`.
                  //
                  node = parent;
                  console.assert(node instanceof instructions.union_switch);
               } else {
                  //
                  // We're not reusing a "switch" node, so we need to create a new one 
                  // of those, and then create a "case" node for it.
                  //
                  // First, resolve the condition.
                  //
                  let lhs = new value_path();
                  {
                     let lhs_src = chunk.lhs;
                     let common_c = 0;
                     let common_s = 0;
                     //
                     // If we're branching on some member of an array of structs i.e. 
                     // `foo[0:2].bar == 2`, then we need to map the array index within 
                     // the condition to the generated VAR_DECL for our array loop.
                     //
                     // First, find a common stem between the condition and the current 
                     // stack.
                     //
                     for(; common_c < lhs_src.chunks.length; ++common_s, ++common_c) {
                        if (common_c >= j)
                           break;
                        let a = item.chunks[common_c];
                        let b = lhs_src.chunks[common_c];
                        if (b instanceof rechunked.chunks.condition)
                           ++common_s;
                        if (!a.compare(b))
                           break;
                     }
                     //
                     // Once we've found the common stem, access values via the stack 
                     // nodes.
                     //
                     for(let i = 0; i < common_s; ++i) {
                        let chunk = stack[i].chunk;
                        let node  = stack[i].node;
                        if (chunk instanceof rechunked.chunks.qualified_decl) {
                           for(let desc of chunk.descriptors)
                              lhs.access_member(desc);
                        } else if (chunk instanceof rechunked.chunks.transform) {
                           lhs.replace(node.transformed_desc);
                        } else if (chunk instanceof rechunked.chunks.array_slice) {
                           lhs.access_array_element(node.loop_index_desc);
                        }
                     }
                     //
                     // Finally, access values within the non-common tail of the condition.
                     //
                     for(; common_c < lhs_src.chunks.length; ++common_c) {
                        let chunk = lhs_src.chunks[common_c];
                        if (chunk instanceof rechunked.chunks.qualified_decl) {
                           for(let desc of chunk.descriptors)
                              lhs.access_member(desc);
                        } else if (chunk instanceof rechunked.chunks.array_slice) {
                           if (chunk.count != 1)
                              throw new Error("unable to resolve loop variable for array slice in condition");
                           lhs.access_array_element(chunk.start);
                        } else {
                           console.assert(false, "unreachable");
                        }
                     }
                  }
                  
                  node = new instructions.union_switch();
                  node.condition_operand = lhs;
                  parent.instructions.push(node);
                  stack.push({ chunk: chunk, node: node });
               }
               
               let bran = new instructions.union_case();
               node.by_value[chunk.rhs] = bran;
               stack.push({ chunk: chunk, node: bran });
               continue;
            }
            console.assert(!!parent);
            console.assert(!(parent instanceof instructions.union_switch));
            
            if (chunk instanceof rechunked.chunks.array_slice) {
               if (is_last_chunk) {
                  //
                  // This is an array of non-transformed primitive values (i.e. values that 
                  // are not arrays themselves, and have no members).
                  //
                  if (chunk.count == 1) {
                     //
                     // Don't generate a for-loop for a single-element array, nor for a 
                     // single-element slice of an array.
                     //
                     let node = new instructions.single();
                     node.value = value.clone();
                     node.value.access_array_element(chunk.start)
                     parent.instructions.push(node);
                     stack.push({ chunk: chunk, node: null });
                     continue;
                  }
               }
               let node = new instructions.array_slice();
               node.array.value = value.clone();
               node.array.start = chunk.start;
               node.array.count = chunk.count;
               value.access_array_element(node.loop_index_desc);
               
               parent.instructions.push(node);
               stack.push({ chunk: chunk, node: node });
               continue;
            }
            if (chunk instanceof rechunked.chunks.qualified_decl) {
               for(let desc of chunk.descriptors)
                  value.access_member(desc);
               
               if (is_last_chunk) {
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
                  let node = new instructions.single();
                  node.value = value;
                  parent.instructions.push(node);
               }
               stack.push({ chunk: chunk, node: null });
               continue;
            }
            if (chunk instanceof rechunked.chunks.transform) {
               let node = new instructions.transform(chunk.types);
               node.to_be_transformed_value = value.clone();
               value.replace(node.transformed_desc);
               
               parent.instructions.push(node);
               stack.push({ chunk: chunk, node: node });
               continue;
            }
            console.assert(false); // unreachable
         }
      }
   }
   return root;
}