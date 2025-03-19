
# XML output

When running the plug-in, you can specify the path to an XML file as `-fplugin-arg-lu_bitpack-xml-out=$(DESIRED_PATH)/test.xml`. If you do so, then every time the plug-in generates serialization code, it will write information about the computed bitpacking format to the specified XML file. (Note that the `.xml` file extension is required and case-sensitive.)

The root node is a `data` element which may contain the following elements:

* `config`
* `categories`
* `c-types`
* `sectors`

## Sections

### `config`

This node holds information about how the bitpacking code generation was configured. It may contain `option` elements with a `name` and `value` attribute.

<dl>
   <dt><code>max-sector-count</code></dt>
      <dd>If present, the <code>value</code> attribute is the maximum sector count.</dd>
   <dt><code>max-sector-bytecount</code></dt>
      <dd>If present, the <code>value</code> attribute is the maximum size of each sector, in bytes.</dd>
</dl>

### `categories`

This node contains one `category` child element for each category (`__attribute__((lu_bitpack_stat_category("name")))`) seen in the to-be-serialized data.

Each `category` has a `name` attribute indicating the category name, and otherwise has identical contents to a `stats` element.

### `c-types`

This node contains one child element for each noteworthy type in the to-be-serialized output. Child elements may use the node names `integral`, `struct`, `union`, or `unknown-type`. (The `unknown-type` node name is a fallback that should never appear in valid data.)

Type elements may additionally have the following attributes:

<dl>
   <dt><code>c-alignment</code></dt>
      <dd>The type's alignment in bytes></dd>
   <dt><code>c-sizeof</code></dt>
      <dd>The type's size in bytes.</dd>
   <dt><code>name</code></dt>
      <dd>The type's name as defined by a <code>typedef</code>.</dd>
   <dt><code>tag</code></dt>
      <dd>The type's tag, i.e. <code>struct TagHere { ... }</code>. In C, but not C++, names and tags have different behavior.</dd>
</dl>

#### Struct and union types

Struct and union types may have the following child elements:

<dl>
   <dt><code>instructions</code></dt>
      <dd>
         <p>A tree of instruction nodes. When the struct or union is serialized as a whole unit (i.e. not split across sectors), it will be serialized by invoking a generated function whose body maps to these instruction nodes. If the struct or union is never serialized as a whole unit, then this child element will not be present.</p>
      </dd>
   <dt><code>stats</code></dt>
      <dd>
         <p>Bitpacking statistics for this type.</p>
      </dd>
   <dt><code>members</code></dt>
      <dd>
         <p>A list of all referenceable members in the struct, as member elements (see below). Anonymous struct types (e.g. <code>struct { ...} foo</code>) may contain their own <code>&lt;members&gt;</code> elements as well.</p>
         <p>This is emitted regardless of whether the struct has a whole-struct function generated (i.e. regardless of whether an <code>instructions</code> element is also present). The general pattern that tools would want to use, when using the XML output, is to use <code>members</code> as the canonical reference for what data is in a given struct type, and then read <code>instructions</code> nodes from the various relevant places (starting with the sectors) to see when and how data is read into those members.</p>
      </dd>
   <dt><code>opaque-buffer-options</code></dt>
      <dd>
         <p>Indicates the opaque buffer bitpacking options applied to this type, and has the same attributes as a <code>buffer</code> value element (see below).</p>
      </dd>
   <dt><code>transform-options</code></dt>
      <dd>
         <p>Indicates the transform bitpacking options applied to this type, and has the same attributes as a <code>transformed</code> value element (see below).</p>
      </dd>
</dl>

#### Integral types

Integral types will have an `integral` node for the "canonical" type, which may have the following child elements:

<dl>
   <dt><code>typedef</code></dt>
      <dd><p>Indicates an existing <code>typedef</code> of the integral type. The <code>name</code> attribute is the typedef'd name.</p></dd>
</dl>

Integral types (`integral` and `typedef`) *may* also possess attributes describing integral bitpacking options (see `integer` value elements below). These attributes will be absent if the integral type is never *directly* used by a to-be-serialized `VAR_DECL` or `FIELD_DECL` with integer bitpacking options (e.g. because the type is always treated as a boolean, or because the type itself is never used but a typedef thereof is used).

### `top-level-values`

This node contains elements which describe the to-be-serialized variables. Each element is a value element (see below) with the following additional attributes:

<dl>
   <dt><code>name</code></dt>
      <dd>The identifier to be serialized</dd>
   <dt><code>dereference-count</code></dt>
      <dd>If present, the value indicates the number of times the identifier is dereferenced before serialization</dd>
   <dt><code>force-to-next-sector</code></dt>
      <dd>If present and set to <code>true</code>, then the identifier is forced to the start of the next sector, even if it could fit in the current sector</dd>
   <dt><code>type</code></dt>
      <dd>The variable's type, in C syntax. Absent if the type has no name.</dd>
   <dt><code>serialized-type</code></dt>
      <dd>The variable's serialized type, in C syntax. Absent if the type has no name. This can vary from the <code>type</code> if the variable is a transformed value, or if it's a dereferenced pointer.</dd>
</dl>

### `sectors`

This node contains one child `sector` element per sector in the serialized output. Each element may contain `instructions` and `stats` elements (one each).

Some bits at the end of a sector may be unused, if a value couldn't fit in the sector but also couldn't be sliced (and so had to be pushed to the next sector). Check the `sector>stats>bitcounts[total-packed]` attribute to see how many bits were actually used.


## Common elements

### `instructions`

An element representing the instructions produced by code generation, in order to serialize a type or sector.

The following element types are defined.

#### `loop`

Indicates a for-loop used to serialize an array or array slice.

The `counter-var` attribute indicates the name of the loop counter variable. The `start` and `count` attributes indicate the loop extents. The `array` attribute indicates the array to be serialized (in the same format as `value` attributes on value elements).

This element only represents the loop itself. Code to actually serialize the array elements will be represented by child elements.

Example:

```xml
<loop counter-var="__a" count="3" start="0" array="sTestStruct.data">
   <integer min="0" bitcount="32" type="u32" value="sTestStruct.data[__a]" />
</loop>
```

#### `padding`

Indicates zero-padding used to keep all permutations of a tagged union the same length in the serialized output. The `bitcount` attribute indicates the number of padding bits emitted.

#### `switch`, `case`, and `fallback-case`

Indicates a switch-case used to handle a tagged union. The `operand` attribute indicates the value that we're switching on. Each child element will typically be a `case` element with a `value` attribute; during serialization, we execute the instructions belonging to whatever `case` the `operand` matches. The last child element may be a `fallback-case` element, indicating instructions to run if the union tag doesn't match any defined case.

#### `transform`

Indicates a value that is transformed to another type. The `value` attribute indicates the to-be-transformed value. The `transformed-type` attribute names the type to which the value is transformed, and the `transformed-value` attribute indicates the name of a local variable of that type.

In the case of a transitive transform, `transformed-type` names the final type, and `transform-through` is a space-separated list of all previous transforms.

As with `loop` elements, this element only represents the transformation itself, and not serialization of the transformed value. Its child elements will handle that.

Example:

```xml
<!-- A through B to C -->
<transform transform-through="TypeB" transformed-value="__transformed_var_0" transformed-type="TypeC" value="sTypeA">
   <structure type="TypeC" value="__transformed_var_0" />
</transform>
```

#### Value elements

Each of these elements represent a single value to serialize. The value is identified by the `value` attribute.

A `value` attribute follows similar syntax to C accessors, e.g. `a.b.c[4][5][6].d`. This may include the use of loop counter variables, such as `list[__a]` or `list[__i_12]`, or the use of locals for `transform` nodes, such as `__transformed_var_0.member`. Loop counters and transformed locals are scoped to (read: unique within) the nearest enclosing `instructions` element.

These elements may also have a `type` attribute indicating the value's declared type in C. If a `typedef` was used, this attribute will name the typedef, not the original type. The attribute should include most qualifiers and other type information mimicking C syntax, e.g. `int` or `int[3]` or `const volatile float*`.

These elements may additionally have a `default-value` attribute or a `default-value-string` child node, indicating the element's default value, if it has one. The latter is used for string-type defaults; otherwise, the former is used. We currently support integer, float, and string defaults; unrecognized defaults that somehow make it through the codegen process without erroring are encoded as `default-value="???"`.

##### `boolean`

Indicates a boolean value serialized as a single bit.

##### `buffer`

Indicates an opaque buffer &mdash; that is, a value serialized via a call to `memcpy`. The `bytecount` attribute indicates the buffer size.

##### `integer`

Indicates an integer value. The bitcount and minimum value used for serialization are indicated by the `bitcount` and `min` attributes, such that the serialized value is the run-time value plus `min`, truncated to the given number of bits.

##### `omitted`

Indicates a value that is omitted from the bitpacked data. Omitted values will generally only be present if they also have a default value, as in that case, they will be forcibly set to their default as part of the generated "read" code.

##### `pointer`

Indicates a pointer that is serialized verbatim. (Pointers do not allow most bitpacking options, and instead always pack in full: on a 32-bit platform, a pointer will serialize as 32 bits.)

##### `string`

Indicates a string value. The `length` value indicates the length of the string data, not including an in-memory null terminator if one is required.

The `nonstring` attribute will be `true` if the value was affected by the `nonstring` or `lu_nonstring` attributes (applied either to the value directly or to its type). This indicates that the value doesn't require a null terminator in memory. In memory, `"ABCDE"` can fit in a `nonstring` value of type `char[5]`, but can only fit in a typical string of type `char[6]` or larger.

##### `structure`

Indicates a structure to be serialized whole by calling a whole-struct serialization function. If you find the corresponding `struct` element in the XML output, its `instructions` node will match the code for the whole-struct serialization function.

##### `transformed`

Indicates a value that is transformed and serialized as another type. The `transformed-type` attribute indicates the type to which the value is transformed, and the `pack-function` and `unpack-function` attributes are the identifiers of those functions used to carry out the transformation.

Remember that transitive transformations are possible. You will have to see if the transformed type is present in the XML output's `c-types` section and, if so, whether that type is itself transformed (and so on).

##### `union-external-tag`

Indicates an externally-tagged union. The `tag` attribute identifies the union tag.

##### `union-internal-tag`

Indicates an internally-tagged union. The `tag` attribute identifies the union tag.

##### `unknown`

Fallback node name that should never appear. Indicates that something went wrong with code generation, without causing an error.


#### Member elements

Member elements are the children of a struct or union `c-type` element's `members` node. These elements are generally the same as value elements, save for the following differences:

* The presence of the following children within the member element:
  * **`annotation`:** Indicates a miscellaneous annotation, whose content is the `text` attribute.
  * **`array-rank`:** Indicates that the value element is an array. One of these children will be present per array rank, each with an `extent` attribute.
  * **`category`:** Indicates membership in a bitpacking category (see the `name` attribute).
* An optimization: if a member element is an integral value, and there exists a `c-types > integral` or `c-types > integral > typedef` element representing the member's value type, then the following attributes will be omitted under the following conditions:
  * The `bitcount` attribute ordinarily present on `integer` member elements is omitted if any of the following conditions are true:
    * The member is a bitfield, and its `bitcount` attribute would be equivalent to the bitfield width.[^bool-typedef-edgecase]
    * The member is not a bitfield, and its `bitcount` attribute would be equivalent to the corresponding attribute on the corresponding `integral`/`typedef` type element.
  * The `min` and `max` attributes ordinarily present on `integer` member elements are omitted if their values would be identical to those of the corresponding attributes on the corresponding `integral`/`typedef` type element.
  * `category` and `annotation` child elements are omitted if they would be identical to any such elements on the corresponding `integral`/`typedef` type element.

[^bool-typedef-edgecase]: When writing tools that act on the XML, be mindful of an edge-case here. If the global bitpacking options specify that some integral typedef should be treated as a boolean, *and* any to-be-serialized struct has a bitfield member declared to that ype, *and* the bitfield width is greater than 1, *and* the member in question either does not specify bitpacking options which would change its bitcount *or* specifies bitpacking options that work out to the same bitcount as the bitfield width, *then* there will exist an `integral` or `typedef` type node for the boolean type, but the boolean type will lack a `bitcount` attribute.

### `stats`

An element representing stats related to some entity's presence in the serialized output. A stats element may contain the following child elements:

<dl>
   <dt><code>bitcounts</code></dt>
      <dd>
         <p>Indicates the entity's size. The <code>total-packed</code> attribute, if present, is the number of bits the entity consumes within the serialized output. The <code>total-unpacked</code> attribute, if present, is the number of bits the entity would consume were it to be <code>memcpy</code>'d instead of bitpacked.</p>
      </dd>
   <dt><code>counts</code></dt>
      <dd>
         <p>Indicates how many times the entity was seen in various places within the serialized output. The <code>total</code> attribute indicates the total count seen, while more specific information is exposed via child nodes.</p>
         <p>An <code>in-sector</code> child element indicates how many of the entity were seen in a given sector. The sector is identified via the zero-based <code>index</code> attribute, and the count is identified via the <code>count</code> attribute.</p>
         <p>An <code>in-top-level-value</code> child element indicates how many of the entity were seen in a given top-level value. The top-level value is identified via the <code>name</code> attribute, and the count is identified via the <code>count</code> attribute.</p>
      </dd>
</dl>

Example:

```xml
<stats>
   <bitcounts total-packed="32" total-unpacked="32" />
   <counts total="1">
      <in-sector count="1" index="0" />
      <in-top-level-value count="1" name="sTestStruct" />
   </counts>
</stats>
```