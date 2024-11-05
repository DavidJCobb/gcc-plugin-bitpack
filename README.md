
# gcc-plugin-bitpack

An attempt at building a GCC plug-in that can generate code for bitpacking data structures in a C program. Initial development guided heavily by [rofirrim/gcc-plugins](https://github.com/rofirrim/gcc-plugins); makefiles borrowed from there as well. (GCC plug-ins, this one included, are compulsorily GPL'd or GPL-compatible, so it's fine.)

As of this writing, I'm currently targeting GCC 11.4.0.

## Feature goals

* Annotate struct members with information specifying how they should be bitpacked
  * Set a bitcount explicitly or compute one dynamically.
  * Booleans should encode as single-bit by default.
  * Integral fields should be able to compute a bitcount based on their min and max values. Where these are not set, they default to the minimum and maximum representable values in the integral's type.
  * For strings, indicate whether they're null-terminated in memory.
  * Ability to categorize struct members, and associate bitpacking options with these categories (i.e. predefining sets of bitpacking options).
* Automatically generate code to serialize structs to a bitpacked format, and read them back from that format.
  * Optional capacity limits
    * Optionally divide bitpacked data into "sectors" of a limited size. Serialization should pause at the end of one sector &mdash; even if this means serializing only part of a struct or array &mdash; and resume where it left off when beginning the next sector.
    * Sector size
    * Sector count
    * Overall size
  * Export an XML file describing the resulting format.
    * This is meant to aid with structs that may change, by facilitating the development of external tools that (guided by the XML) can read data in an older format and convert it to a newer one.
    * Currently possible via a command line argument: `-fplugin-arg-lu_bitpack-xml-out=$(DESIRED_PATH)/test.xml`
  * Export a human-readable report stating how much space would be consumed by a non-bitpacked representation (i.e. blind `memcpy`ing) versus how much is consumed by the bitpacked representation.
    * This is meant to aid with retrofitting this bitpacking scheme into programs that did not previously use it: use the report to figure out what parts of a struct are more "expensive," and then study them and the code that operates on them in detail, to figure out how you can tighten the bitpacking options for those fields.

## Distant goals

* Some form of GCC version-independence
  * Study GCC commit history and figure out what range of GCC versions should be compatible with the plug-in given the data structures and functions we access?
  * Wrap GCC internals somehow, to facilitate supporting a larger range of versions by adapting differently-versioned data structures to a common interface?
  * What about the ABI?

## Non-goals

* Presence bits (think "`std::optional`"): the ability to pack data using the smallest possible representation. My current use case entails packing data into a limited storage space in such a manner that the largest possible data must fit; ergo bitpacked data must always be uniform in size, so we can check statically whether we have enough room.


## Documentation

### Generated XML

* The serialized sectors contain `value` elements whose `path` attributes indicate the value being serialized. These are *mostly* in C syntax (e.g. `foo.bar[3]`), except that for array slices, we use Python slice notation (i.e. `foo[start:end]` to indicate serialization of all array elements in the range `[start, end)`).
  * All struct types have their members listed as `struct-type` elements and their children. You can correlate the `value` nodes in the sectors with the struct definitions to know what's being serialized when.
* TODO: document everything else