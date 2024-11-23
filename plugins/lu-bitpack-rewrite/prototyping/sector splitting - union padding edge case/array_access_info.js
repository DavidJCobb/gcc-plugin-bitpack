class array_access_info {
   constructor(o) {
      this.start = o?.start || 0;
      this.count = o?.count;
      if (!this.count && this.count !== 0)
         this.count = 1;
   }
   
   compare(other) {
      if (this.start != other.start)
         return false;
      if (this.count != other.count)
         return false;
      return true;
   }
   
   to_string() {
      if (this.count == 1)
         return `[${this.start}]`;
      let end = this.start + this.count;
      return `[${this.start}:${end}]`;
   }
};