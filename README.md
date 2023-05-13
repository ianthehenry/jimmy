# jimmy

Janet bindings for [immer](https://github.com/arximboldi/immer), a library of immutable data structures.

```
(use jimmy)

(def good-numbers (set/new 1 2 3 4 5))
(print good-numbers)
#| {2 3 5 1 4}

# actually, five kinda sucks
(print (set/remove good-numbers 5))
#| {2 3 1 4}

# but it was persistent all along!!
(print good-numbers)
#| {2 3 5 1 4}

# sets support union/intersect/difference, and
# operators are standing by
(print (- good-numbers (set/new 1 2)))
#| {3 5 4}
```

You can `(use jimmy)` to bring all types into scope as `set/new`, `map/new`, etc., or you can `(import jimmy/set)` to only bring a specific submodule into scope.

**Note: the `jimmy/map` module is extremely incomplete.**

# API

## `jimmy/set`

### Functions

```janet
(set/add set & xs)
```

Returns a new set containing all of the elements from the original set and all of the subsequent arguments.

---

```janet
(set/count set pred)
```

Returns the number of elements in the set that match the given predicate. `pred` can be any callable value, not just a function.

Note that the arguments are in a different order than Janet's `count` function.

---

```janet
(set/difference set & sets)
```

Returns a set that is the first set minus all of the latter sets.

---

```janet
(set/filter set pred)
```

Returns a set containing only the elements for which the predicate returns a truthy value. `pred` can be any callable value, not just a function.

Note that the arguments are in the opposite order of Janet's `filter` function.

---

```janet
(set/filter-map set f)
```

Like `set/map`, but excludes `nil`. `f` can be any callable value, not just a function.

---

```janet
(set/intersection & sets)
```

Returns a set that is the intersection of all of its arguments. Naively folds when given more than two arguments.

---

```janet
(set/map set f)
```

Returns a new set derived from the given transformation function. `f` can be any callable value, not just a function.

Note that the arguments are in the opposite order of Janet's `map` function.

---

```janet
(set/new & xs)
```

Returns a persistent immutable set containing only the listed elements.

---

```janet
(set/of iterable)
```

Returns a set of all the values in an iterable data structure.

---

```janet
(set/of-keys iterable)
```

Returns a set of all the keys in an interable data structure.

---

```janet
(set/reduce set init f)
```

Returns a reduction of the elements in the set, which will be traversed in arbitrary order. `f` can be any callable value, not just a function.

Note that the arguments are in a different order than Janet's `reduce` function.

---

```janet
(set/remove set & xs)
```

Returns a new set containing all of the elements from the original set except any of the subsequent arguments.

---

```janet
(set/strict-subset? a b)
```

Returns true if `a` is a strict subset of `b`.

---

```janet
(set/strict-superset? a b)
```

Returns true if `a` is a strict superset of `b`.

---

```janet
(set/subset? a b)
```

Returns true if `a` is a subset of `b`.

---

```janet
(set/superset? a b)
```

Returns true if `a` is a superset of `b`.

---

```janet
(set/to-array set)
```

Returns an array of all of the elements in the set, in no particular order.

---

```janet
(set/to-tuple set)
```

Returns a tuple of all of the elements in the set, in no particular order.

---

```janet
(set/union & sets)
```

Returns a set that is the union of all of its arguments.

### Methods

- `:+` is an alias for `set/union`
- `:*` is an alias for `set/intersection`
- `:-` is an alias for `set/difference`

### Values

- `set/empty` is the empty set

## `jimmy/map`

### Functions

```janet
(map/keys map)
```

Returns an iterator over the keys in the map.

---

```janet
(map/new & kvs)
```

Returns a persistent immutable map containing the listed entries.

---

```janet
(map/pairs map)
```

Returns an iterator over the key-value pairs in the map.

---

```janet
(map/values map)
```

Returns an iterator over the values in the map.

### Values

- `map/empty` is the empty map

## `jimmy/vec`

### Functions

```janet
(vec/count vec pred)
```

Returns the number of elements in the vector that match the given predicate. `pred` can be any callable value, not just a function.

Note that the arguments are in a different order than Janet's `count` function.

---

```janet
(vec/filter vec pred)
```

Returns a vector containing only the elements for which the predicate returns a truthy value. `pred` can be any callable value, not just a function.

Note that the arguments are in the opposite order of Janet's `filter` function.

---

```janet
(vec/filter-map vec f)
```

Like `vec/map`, but excludes `nil`. `f` can be any callable value, not just a function.

---

```janet
(vec/first vec)
```

Returns the first element of the vector.

---

```janet
(vec/last vec)
```

Returns the last element of the vector.

---

```janet
(vec/map vec f)
```

Returns a new vector derived from the given transformation function. `f` can be any callable value, not just a function.

Note that the arguments are in the opposite order of Janet's `map` function.

---

```janet
(vec/new & xs)
```

Returns a persistent immutable vector containing only the listed elements.

---

```janet
(vec/of iterable)
```

Returns a vector of all the values in an iterable data structure.

---

```janet
(vec/pop vec)
```

Returns a new vector with the last element removed.

---

```janet
(vec/popn vec)
```

Returns a new vector with the last n elements removed.

---

```janet
(vec/push vec & xs)
```

Returns a new vector containing all of the elements from the original and all of the subsequent arguments.

---

```janet
(vec/put vec n val)
```

Returns a new vector with nth element set to val.

---

```janet
(vec/reduce vec init f)
```

Returns a reduction of the elements in the vector. `f` can be any callable value, not just a function.

Note that the arguments are in a different order than Janet's `reduce` function.

---

```janet
(vec/take vec n)
```

Returns a new vector containing the first n elements of vec, or all of vec if n >= (length vec).

---

```janet
(vec/to-array vec)
```

Returns an array of all of the elements in the vector.

---

```janet
(vec/to-tuple vec)
```

Returns a tuple of all of the elements in the vector.

### Values

- `vec/empty` is the empty vector

# Gotchas

Janet's iteration protocol is not flexible enough for Jimmy to support `eachk` or `eachp` or the `:keys` and `:pairs` directive in `loop`-family macros.

Unlike Janet's tables and structs, Jimmy data structures can contain `nil`:

```
repl:1:> (use jimmy)
repl:2:> (set/new 1 2 3 nil)
<jimmy/set {3 1 nil 2}>
repl:3:> (map/new nil :value :key nil)
<jimmy/map {nil :value :key nil}>
```

Iterating over a map with `each` will iterate over key-value pairs, not just values. To iterate over values, use:

```
(each value (map/values map)
  (print value))
```

You should not use `eachk` or `eachp` with Jimmy maps, because `next` returns an iterator, not a key. To iterate over the keys in a map, use:

```
(each value (map/keys map)
  (print value))
```

In order to support iteratation with Janet's `next` protocol for unordered containers, Jimmy boxes the corresponding C++ iterator into an abstract type to use as the key. This means that a simple `(each el set (print el))` will allocate memory for the abstract type containing the iterator. It's not a *lot* of memory, but it's short-lived garbage and Janet's GC currently does nothing to optimize for short-lived garbage. This probably won't cause performance problems, just something to bear in mind.

Also, I don't know C++, which is sort of a flaw when trying to write bindings for a C++ library, so I would caution you to read the code with a very critical eye. Code review welcome!

# TODO

- [ ] add bindings for `immer::map`
- [ ] 1.0 release
