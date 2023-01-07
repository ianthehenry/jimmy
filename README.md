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
```

Currently Jimmy only exposes the `set` container, and the API is woefully incomplete. But I hope this can serve as a useful example of how to write abstract types in Janet.

I don't know C++, though, which is sort of a flaw when trying to write bindings for a C++ library, so I would caution you to read the code with a very critical eye. Code review welcome!

# Gotchas

Unlike Janet tables and structs, Jimmy data structures can contain `nil`:

```
repl:1:> (import jimmy/set)
repl:2:> (set/new 1 2 3 nil)
<jimmy/set {3 1 nil 2}>
```

In order to support iteratation with Janet's `next` protocol for unordered containers, Jimmy boxes the corresponding C++ iterator into an abstract type to use as the key. This means that a simple `(each el set (print el))` will allocate memory for the abstract type containing the iterator. It's not a *lot* of memory, but it's short-lived garbage and Janet's GC currently does nothing to optimize for short-lived garbage. This probably won't cause performance problems, just something to bear in mind.

I cannot think of a reasonable way to support iteration over `map` values with a meaningful key, because `immer` does not exposes enough functionality to retrieve the next key given an extant key in anything less than linear time. `immer::map`'s `find` returns a *pointer* rather than an iterator that we could use to advance to implement this. Jimmy does not currently have bindings for `map` at all, though, so you can't tell.
