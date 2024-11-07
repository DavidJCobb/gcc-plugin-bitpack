
# `array`

An array of types (the template parameters).

```c++
using valid_types = lu::type_containers::array<
   bool,
   char,
   float,
   int
>;
```

## Contents

### Constants

#### `contains_matching_type<Predicate>`

Produces `true` if the array contains a type matching the predicate, or `false` otherwise. The predicate is a non-type template parameter e.g.

```c++
static_assert(
   lu::type_containers::array<bool, char>::contains_matching_type<
      []<typename Current>() -> bool {
         return std::is_same_v<Current, char>;
      }
   >
);
```

#### `count`

Produces the number of types in the array.

#### `index_of<T>`

The index of `T` in the array, or `(size_t)-1`.

#### `index_of_matching_type<Predicate>`

If the array contains a type matching the predicate, produces the index of the first such type. Otherwise, produces `(size_t)-1`. The predicate is a non-type template parameter e.g.

```c++
using type_array = lu::type_containers::array<bool, char>;
static_assert(
   type_array::index_of_matching_type<
      []<typename Current>() -> bool {
         return std::is_same_v<Current, char>;
      }
   > == 1
);
```

### Functions

#### `for_each`

Executes a templated lambda (received as a function argument) for each type in the array. If you pass extra arguments, then those will be forwarded to the lambda.

```c++
using type_array = lu::type_containers::array<bool, char>;

void print_array_info() {
   size_t total_sizes = 0;
   type_array::for_each([&total_sizes]<typename Current>() {
      total_sizes += sizeof(Current);
   });
   printf("Total size: %u\n\n", (int)total_sizes);
   
   //
   // Example with extra arguments forwarded to the lambda:
   //
   type_array::for_each(
      [&total_sizes]<typename Current>(std::string_view prefix) {
         printf("%s%u\n", prefix.data(), (int)sizeof(Current));
      },
      "Size of element: "
   );
}
```

#### `for_each_until_false`

Executes a templated lambda (received as a function argument) for each type in the array, stopping early if the lambda returns a falsy value. The lambda works the same way as with `for_each`, including the ability to receive extra arguments.

#### `for_each_until_true`

Executes a templated lambda (received as a function argument) for each type in the array, stopping early if the lambda returns a truthy value. The lambda works the same way as with `for_each`, including the ability to receive extra arguments.

#### `reduce<Result, Predicate>`

Executes a templated lambda (received as a template parameter) for each type in the array. The lambda receives a function argument of type `Result`, and must return a value of type `Result`, with the return value from lambda invocation *n* being fed as the argument to lambda invocation *n + 1*. The return value from the last lambda invocation is the return value of `reduce`.

You can optionally pass a start argument to `reduce`, which will be the argument fed into lambda invocation 0. If you don't pass a start argument, then `Result` must be default-constructible.

```c++
using type_array = lu::type_containers::array<int, float>;

void print_array_info() {
   auto total_size = type_array::reduce<
      size_t,
      []<typename Current>(size_t prev) -> size_t {
         return prev + sizeof(Current);
      }
   >();
   printf("Total size: %u\n\n", (int)total_size);
   
   size_t base = 1000;
   auto adjusted_size = type_array::reduce<
      size_t,
      []<typename Current>(size_t prev) -> size_t {
         return prev + sizeof(Current);
      }
   >(base);
   printf("Total size plus 1000: %u\n\n", (int)total_size);
}
```

#### `size`

Returns the number of types in the array.

### Types

#### `as_tuple`

Produces a `std::tuple` type whose elements are the elements in the array.

#### `filter_types<Predicate>`

Produces a `lu::type_containers::array` type whose elements are those elements from the source array for which `Predicate` returns true. The predicate is passed as a non-type template parameter and is expected to be a templated lambda.

```c++
using type_array = lu::type_containers::array<bool, char, int, float>;

using filtered_array = type_array::filter_types<
   []<typename Current>() -> bool {
      return std::is_integral_v<Current>;
   }
>;

static_assert(std::is_same_v<
   filtered_array,
   lu::type_containers::array<bool, char, int>
>);
```

#### `get_matching_type<Predicate>`

Produces the first type in the array that matches the predicate. The predicate is of the same form as used for `index_of_matching_type`.

#### `nth_type<T>`

Produces the N-th type in the array.

```c++
using first_type = my_array::nth_type<0>;
```

#### `transform_types<Transform>`

Produces a `lu::type_containers::array` type whose elements are those elements from the source array transformed by `Transform`, which is a struct that takes one template parameter (the input type) and has a `type` member that is the output type.

An example using a custom transform:

```c++
template<typename T>
struct wrap_transform {
   using type = std::tuple<T>;
};

using source_array = lu::type_containers::array<bool, char, int>;
using result_array = source_array::transform_types<wrap_transform>;

static_assert(std::is_same_v<
   result_array,
   lu::type_containers::array<std::tuple<bool>, std::tuple<char>, std::tuple<int>>
>);
```

An example using the STL:

```c++
using source_array = lu::type_containers::array<char, int>;
using result_array = source_array::transform_types<std::make_unsigned>;

static_assert(std::is_same_v<
   result_array,
   lu::type_containers::array<unsigned char, unsigned int>
>);
```

