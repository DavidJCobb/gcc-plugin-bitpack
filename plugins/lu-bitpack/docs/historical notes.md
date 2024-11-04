
# Historical notes

This project was inspired by a situation where I needed to bitpack data on low-end hardware, with the additional requirement that data be broken up into "sectors" of consistent size with header and footer data. Sectors were not guaranteed to be stored contiguously and could be located in arbitrarily-placed buffers.

The need to divide data by sector meant that stateful bitpacking wasn't viable: how do I track, at run-time, that I'm halfway into some potentially nested struct or array? It didn't seem practical, especially in C, the language I was using for that situation.

I built a Frankenstein-style proof of concept wherein I wrote struct definitions in XML (so I could annotate them with arbitrary bitpacking information); a custom-made C++ program would consume these XML definitions and produce both C-language struct definitions and C-language code to invoke a set of bitstream functions I wrote. Essentially, it was a bunch of cursed nonsense like

```c
struct Foo {
#include "generated/struct-definitions/Foo.h"
};
```

It worked, and I was able to figure out the logistics involved in splitting (potentially nested) structs or arrays across sector boundaries. However, it wasn't scalable or maintainable; it's obviously far, far better to have the data definitions *in situ* in the C codebase and simply have some tool that can read them and do the code generation. XML would've been better as an *output* format (in order to build tooling that can upgrade a set of bitpacked data from one version to another) rather than an *input* format.


## The old approach in detail

This is the approach I used to generate C source code from XML struct definitions.

* Index all types and their members' bitpacking options (load XML definitions)
* For each struct type, generate a function that will serialize an entire instance of that type.
* Create a list of `serialization_item` instances (the "item list") &mdash; one for each top-level struct to serialize.
  * Each `serialization_item` consists of a pointer to an indexed struct, a pointer to an indexed struct member, a list of zero or more array indices, and a string representing the fully-qualified path to the described member.
* For each item in the item list:
  * Try to fit the item into the current sector. (In particular, if it's a struct type or an array thereof, then we invoke one of the functions we generated earlier to serialize the thing whole.)
  * If the item doesn't fit, then try to "expand" it.
    * If expansion is possible (i.e. if the item is a struct or array), then it will produce multiple `serialization_item` instances; replace the current item with those (splicing them into the item list), and then retry from the first of them.
      * For example, given a member `int foo[3][2][3]`, expanding `foo` produces `foo[0]`, `foo[1]`, and `foo[2]`; and expanding `foo[0]` produces `foo[0][0]` and `foo[0][1]`.
      * Similarly, given a member `struct foo_type { int a; int b; } c[2]`, expanding `c[0]` produces `c[0].a` and `c[0].b`. Both of these serialization items refer to the `foo_type` struct and its members `a` and `b`; the only way to know that we got here from `c[0]` is via the path string (e.g. `"c[0].a"`). This is fine, since stitching strings together is our goal.
    * If expansion is not possible, then start a new sector.

As you can imagine, the use of string paths here complicates using this approach in our GCC plug-in.


## The new approach, in this plug-in

* Start the zeroth sector.
* For each top-level variable to serialize:
  * **[A]** For each member of the current variable:
    * If the member is a struct, then...
      * If the member can fit entirely in the current sector, then bitpack it whole.
      * Else if the member can't fit, then...
        * Recursively execute **[A]** treating this member as the new current variable.
    * Else if the member is a non-struct or array of structs, then...
      * If the member can fit entirely in the current sector, then bitpack it whole.
      * Else if the member can't fit, then...
        * If the member isn't an array, then start a new sector, and then recursively execute **[A]** treating the member as the new current variable.
        * Else if the member is an array...
          * **[B]** While there are any array elements left to serialize:
            * Compute the number of elements *n* that can fit in the current sector's available space.
            * Bitpack the next *n* elements.
            * If there are no more elements in the array to serialize, then break this **[B]** loop.
            * Recursively execute **[A]** treating the next element as the new current variable. The effect of this will be to split the element across sector boundaries if indeed it can be split, and start a new sector either way.

Rather than indexing the bitpacking options and data for all structs in advance, we instead extract those options from struct types as those types are encountered, caching them for fast retrieval should we encounter more instances of the same struct type later.

Similarly, we don't split structs across sector boundaries (with the `serialization_item` lists) in advance of code generation. Instead, we perform the sector splits as we generate code.

In essence, we've folded three passes into one: we index bitpacking options and handle sector splitting as we generate the code.