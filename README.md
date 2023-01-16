# jimmy

Janet bindings for [immer](https://github.com/arximboldi/immer), a library of immutable data structures.

```
(import jimmy/set)

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

Currently Jimmy only exposes the `set` container, and the API is woefully incomplete. But I hope this can serve as a useful example of how to write abstract types in Janet.

I don't know C++, though, which is sort of a flaw when trying to write bindings for a C++ library, so I would caution you to read the code with a very critical eye. Code review welcome!

# API

## Functions

```janet
(set/add set & xs)
```

Returns a new set containing all of the elements from the original set and all of the subsequent arguments.

---

```janet
(set/count set pred)
```

Returns the number of elements in the set that match the given predicate. `pred` can be any callable value, not just a function.

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

## Values

- `set/empty` is the empty set.

## Methods

- `:+` is an alias for `set/union`
- `:*` is an alias for `set/intersection`
- `:-` is an alias for `set/difference`

# Gotchas

Unlike Janet's tables and structs, Jimmy data structures can contain `nil`:

```
repl:1:> (import jimmy/set)
repl:2:> (set/new 1 2 3 nil)
<jimmy/set {3 1 nil 2}>
```

In order to support iteratation with Janet's `next` protocol for unordered containers, Jimmy boxes the corresponding C++ iterator into an abstract type to use as the key. This means that a simple `(each el set (print el))` will allocate memory for the abstract type containing the iterator. It's not a *lot* of memory, but it's short-lived garbage and Janet's GC currently does nothing to optimize for short-lived garbage. This probably won't cause performance problems, just something to bear in mind.

I cannot think of a reasonable way to support iteration over `map` values with a meaningful key, because `immer` does not exposes enough functionality to retrieve the next key given an extant key in anything less than linear time. `immer::map`'s `find` returns a *pointer* rather than an iterator that we could use to advance to implement this. Jimmy does not currently have bindings for `map` at all, though, so you can't tell.

# TODO

- [ ] add bindings for `immer::map`
- [ ] 1.0 release
- [ ] iterators do not keep their backing data structures alive
