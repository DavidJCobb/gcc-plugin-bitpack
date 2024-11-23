
# Sector splitting

One of the more complex features of this plug-in is the ability to divide the serialized output into multiple "sectors," which have a fixed maximum size, and can be serialized to non-contiguous locations and in any (chronological) order. This means that before we generate serialization code, we must first take the values to be serialized and divide them into multiple sectors. We do this by working in terms of `codegen::serialization_item`s.

If a value would be located near the end of a sector, and would be too large to fit in that sector, then we want to "expand" it, if possible. For example, a `serialization_item` that represents one array will expand into multiple `serialization_item`s, one for each individual array element; and a `serialization_item` that represents a struct will expand into one `serialization_item` per struct member.


## Unions cannot be split across sectors

The logistics for splitting tagged unions across sectors are highly complex, when combined with the requirement that sectors be fixed-length. This makes split unions highly difficult to implement, to the point that it's arguably not worth the complexity. As such, the sector-splitting algorithm refuses to split unions.

Consider the following struct:

```c
static struct TestStruct {
   uint32_t tag;
   union __attribute__((lu_bitpack_union_external_tag("tag"))) {
      __attribute__((lu_bitpack_tagged_id(0))) uint8_t  a[2];
      __attribute__((lu_bitpack_tagged_id(1))) uint32_t b;
   } data;
} sTestStruct;
```

This would produce the following serialization items, if fully expanded without sector splitting:

| Offset | Bitcount | End | Serialization item |
| -: | -: | -: | :- |
|  0 | 32 | 32 | `sTestStruct|tag` |
| 32 | 16 | 48 | `sTestStruct|data|(sTestStruct.tag == 0) a[0:2]` |
| 48 | 16 | 64 | `sTestStruct|data|(sTestStruct.tag == 0) {pad:16}` |
| 32 | 32 | 64 | `sTestStruct|data|(sTestStruct.tag == 1) b` |

If we limit sectors to being 6 bytes (48 bits) large, though, then the sectors would have to split as follows:

* Sector 0
  * `sTestStruct|tag`
* Sector 1
  * `sTestStruct|data|(sTestStruct.tag == 0) a[0:2]`
  * `sTestStruct|data|(sTestStruct.tag == 0) {pad:16}`
  * `sTestStruct|data|(sTestStruct.tag == 1) b`

The reason for this is because `sTestStruct.data.a` and `sTestStruct.data.b` would be positioned 32 bits into sector 0 if not split, but `sTestStruct.data.b` is itself 32 bits long: its end would be located at bit 64. Because `b` doesn't fit, it has to be pushed to the next sector. However, our goal is to avoid variable-length data; ergo for consistency `a` must be dragged forward as well. (It should be noted that this is an order-independent phenomenon. If the larger member came before the smaller one, the smaller one would still be dragged forward.)

In this case, the logistics are fairly simple: all of the members that have to be pushed to the next sector start at the same offset. Bitfields and arbitrary bitcounts offer opportunities for more complexity, however. Consider:

```c
static struct TestStruct {
   uint32_t tag;
   union __attribute__((lu_bitpack_union_external_tag("tag"))) {
      __attribute__((lu_bitpack_tagged_id(0))) struct {
         __attribute__((lu_bitpack_bitcount(5))) uint8_t x;
         __attribute__((lu_bitpack_bitcount(4))) uint8_t y;
      } a;
      __attribute__((lu_bitpack_tagged_id(1))) struct {
         uint8_t x;
      } b;
   } data;
} sTestStruct;
```

This would produce the following serialization items, if fully expanded without sector splitting:

| Offset | Bitcount | End | Serialization item |
| -: | -: | -: | :- |
|  0 | 32 | 32 | `sTestStruct|tag` |
| 32 |  5 | 37 | `sTestStruct|data|(sTestStruct.tag == 0) a.x` |
| 37 |  4 | 41 | `sTestStruct|data|(sTestStruct.tag == 0) a.y` |
| 32 |  8 | 40 | `sTestStruct|data|(sTestStruct.tag == 1) b.x` |
| 40 |  1 | 41 | `sTestStruct|data|(sTestStruct.tag == 1) {pad:1}` |

If we limit sectors to being 5 bytes (40 bits) large, then we have to push `sTestStruct.data.a.y` to the next sector, because otherwise it would be located at [37, 41], overflowing sector 0. However, `sTestStruct.data.b.x` overlaps that area, being located at [32, 40], and it's indivisible, so it, too, would need to be pushed forward. But then `sTestStruct.data.a.x` also needs to be pushed forward, now, since its range of [32, 37] overlaps [32, 40].

To figure out which values you'd need to push to the next sector, you'd have to repeatedly re-process the list of serialization items without making changes, to figure out the full range of overlapping values within the union that need to be pushed. However, recall that serialization items are not initially expanded, and note also that no algorithm has been designed for undoing the expansion of a group of serialization items. In order to know what portions of the union must be pushed to the next sector, we must *fully* expand it, including expanding any nested structs that could only be serialized via whole-struct functions if left unexpanded.