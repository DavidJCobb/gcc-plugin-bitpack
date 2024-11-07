
# `lu::tuples`

Helper library for working with `std::tuple` types.

## Contents

### Constants

#### `contains_type`

Check if a `std::tuple` specialization contains a given type. Only exact matches are considered.

```c++
using haystack = std::tuple<int, float>;
using needle   = int;
static_assert(lu::tuples::contains_type<haystack, needle>);
```

#### `contains_type_matching_predicate`

Check if a `std::tuple` specialization contains any type matching a predicate (passed as a non-type template parameter) that can be executed at compile-time.

```c++
static_assert(
   lu::tuples::contains_type_matching_predicate<
      std::tuple<int, float>,
      []<typename Current>() -> bool {
         return std::is_same_v<Current, int>;
      }
   >
);
```

#### `find_first_matching_type`

Check if a `std::tuple` specialization contains any type matching a predicate (passed as a non-type template parameter) that can be executed at compile-time. If so, return the index of that type; otherwise, return `(size_t)-1`.

```c++
using haystack = std::tuple<int, float>;

constexpr const size_t index = lu::tuples::find_first_matching_type<
   haystack,
   []<typename Current>() -> bool {
      return std::is_same_v<Current, float>;
   }
>;

static_assert(std::is_same_v<
   std::tuple_element_t<index, haystack>,
   float
>);
```

#### `first_index_of_type`

Check if a `std::tuple` specialization contains any type matching a predicate that can be executed at compile-time. If so, return the index of that type; otherwise, return `(size_t)-1`.

```c++
using haystack = std::tuple<int, float>;
using needle   = float;

constexpr const size_t index = lu::tuples::first_index_of_type<haystack, needle>;

static_assert(std::is_same_v<
   std::tuple_element_t<index, haystack>,
   needle
>);
```


### Types

#### `concat`

Concatenate an arbitrary number of tuple types together, producing a new `std::tuple` specialization. The difference between this and `std::tuple_cat` is that this takes tuple types and produces a tuple type, whereas `std::tuple_cat` takes tuple instances and produces a tuple instance.

```c++
using concatted = lu::tuples::concat<
   std::tuple<int, float>,
   std::float<char, std::string>
>;

static_assert(std::is_same_v<
   concatted,
   std::tuple<int, float, char, std::string>
>);
```

#### `filter_types`

Given a `std::tuple` *source* and a predicate function (passed as a non-type template parameter), produce a new `std::tuple` type containing only the element types in *source* that pass the predicate.

```c++
using source = std::tuple<bool, char, int, float>;

using filtered = lu::tuples::filter_types<
   source,
   []<typename Current>() -> bool {
      return std::is_floating_point_v<Current>;
   }
>;

static_assert(std::is_same_v<filtered, std::tuple<float>>);
```

#### `prepend`

Prepend a new element type to a `std::tuple` type.

```c++
using prepended = lu::tuples::prepend<std::tuple<float>, int>;

static_assert(std::is_same_v<
   std::tuple_element_t<0, prepended>,
   int
>);
```

#### `transform_types`

Given a `std::tuple<A, B>` type and a transform struct, produce the type `std::tuple<C, D>` where C and D are the result of running A and B through the transform. The transform should be defined as a struct templated on an input type, such that `transform_t<Input>::type` is an output type.

An example using a custom transform:

```c++
template<typename T>
struct wrap_transform {
   using type = std::tuple<T>;
};

using transformed = lu::tuples::transform_types<
   std::tuple<bool, char, int>,
   wrap_transform
>;

static_assert(std::is_same_v<
   transformed,
   std::tuple<std::tuple<bool>, std::tuple<char>, std::tuple<int>>
>);
```

An example using the STL:

```c++
using transformed = lu::tuples::transform_types<
   std::tuple<char, int>,
   std::make_unsigned
>;

static_assert(std::is_same_v<
   transformed,
   std::tuple<unsigned char, unsigned int>
>);
```

#### `unpack_types_into`

Unpack a tuple's element types into the parameter list of some other template.

```c++
#include <iostream>

template<typename... Args>
struct destination_type {
   static constexpr const size_t count = sizeof...(Args);
};

using unpacked = lu::tuples::unpack_types_into<
   std::tuple<int, float>,
   destination_type
>;

void print_count() {
   printf("Type count: %u\n", (int)unpacked::count);
}
```


### Functions

#### `for_each_type`

Run a functor on each element type in a `std::tuple`. The functor is passed as a function argument.

```c++
#include <iostream>
#include <string>

using haystack = std::tuple<int, float>;

void print_each_size(const std::string& prefix) {
   lu::tuples::for_each_type<haystack>([&prefix]<typename Current>() {
      printf("%s%u\n", prefix.c_str(), sizeof(Current));
   });
}
```