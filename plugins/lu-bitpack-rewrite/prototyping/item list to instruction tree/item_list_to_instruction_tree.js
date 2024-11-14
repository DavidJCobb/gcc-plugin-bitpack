
function item_list_to_instruction_tree(items) {
   
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