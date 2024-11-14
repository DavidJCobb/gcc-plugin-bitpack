
function item_list_to_instruction_tree(items) {
   function segment_lists_are_equal(a, b) {
      if (a.length != b.length)
         return false;
      for(let i = 0; i < a.length; ++i)
         if (a[i].to_string() != b[i].to_string()) // lazy
            return false;
      return true;
   }
   
   let container = class container {
      constructor() {
         this.condition_lhs = null;
         this.items_by_rhs  = {}; // unordered_map<intmax_t, Array<branch_tree_node | serialization_item>
         this.depth         = -1;
      }
      
      // returns number of items grabbed
      grab_from(items) {
         let i = 0;
         let current_value = null;
         for(; i < items.length; ++i) {
            let item = items[i];
            if (item.conditions.length <= this.depth)
               break;
            if (this.depth >= 0) {
               let ic_here = item.conditions[this.depth];
               if (!segment_lists_are_equal(ic_here.segments, this.condition_lhs))
                  break;
               if (ic_here.value !== current_value) {
                  if (this.items_by_rhs[ic_here.value])
                     //
                     // The instructions associated with a given value must be contiguous.
                     //
                     break;
                  this.items_by_rhs[ic_here.value] = [];
                  current_value = ic_here.value;
               }
            }
            if (!this.items_by_rhs[current_value])
               this.items_by_rhs[current_value] = [];
            if (item.conditions.length > this.depth + 1) {
               let next_cnd = item.conditions[this.depth + 1];
               
               let node = new container();
               this.items_by_rhs[current_value].push(node);
               node.depth = this.depth + 1;
               node.condition_lhs = next_cnd.segments;
               i += node.grab_from(items.slice(i));
               --i; // so we don't skip next
               continue;
            }
            this.items_by_rhs[current_value].push(item);
         }
         return i;
      }
   };
   let root = new container();
   root.grab_from(items);
   
   let transform_container = class transform_container {
      constructor() {
         this.type  = null;
         this.items = [];
         this.depth = -1;
      }
      
      // returns number of items grabbed
      grab_from(items) {
         let i = 0;
         let current_value = null;
         for(; i < items.length; ++i) {
            let item = items[i];
            if (this.depth >= 0 && !(item instanceof serialization_item))
               break;
            
            
            if (item.conditions.length <= this.depth)
               break;
            if (this.depth >= 0) {
               let ic_here = item.conditions[this.depth];
               if (!segment_lists_are_equal(ic_here.segments, this.condition_lhs))
                  break;
               if (ic_here.value !== current_value) {
                  if (this.items_by_rhs[ic_here.value])
                     //
                     // The instructions associated with a given value must be contiguous.
                     //
                     break;
                  this.items_by_rhs[ic_here.value] = [];
                  current_value = ic_here.value;
               }
            }
            if (!this.items_by_rhs[current_value])
               this.items_by_rhs[current_value] = [];
            if (item.conditions.length > this.depth + 1) {
               let next_cnd = item.conditions[this.depth + 1];
               
               let node = new container();
               this.items_by_rhs[current_value].push(node);
               node.depth = this.depth + 1;
               node.condition_lhs = next_cnd.segments;
               i += node.grab_from(items.slice(i));
               --i; // so we don't skip next
               continue;
            }
            this.items_by_rhs[current_value].push(item);
         }
         return i;
      }
   };
   
   function transform_leaves(tree) {
      for(let list of Object.values(tree.items_by_rhs)) {
         for(let i = 0; i < list.length; ++i) {
         }
         
      }
   }
   transform_leaves(root);
   
   console.log(root);
   
   //
   // TODO: Now that items are split by branch, walk the branch tree and 
   // convert transform-type and array-type options to their proper types.
   //
   
   return root;
}