(def functions
  (seq [[name binding] :pairs (require "./build/jimmy/native")
        :when (table? binding) :when (binding :doc)]
    (def [head & body] (string/split "\n\n" (binding :doc)))
    [name head (string/join body "\n\n")]))

(defn print-docs-for [namespace values methods]
  (printf "## `jimmy/%s`\n" namespace)
  (print "### Functions\n")
  (var is-first true)
  (each [name header body] (sort-by (fn [[name _ _]] name) functions)
    (when (string/has-prefix? (string namespace "/") name)
      (unless is-first
        (print "\n---\n"))
      (set is-first false)
      (printf "```janet\n%s\n```\n\n%s" header body)))
  (unless (empty? methods)
    (print "\n### Methods\n")
    (each method methods
      (printf "- %s" method)))
  (unless (empty? values)
    (print "\n### Values\n")
    (each value values
      (printf "- %s" value)))
  (print))

(print
`````
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

You can `(use jimmy)` to bring all types into scope as `set/new`, `map/new`, etc., or you can `(import jimmy/set)` to only bring set-related bindings into scope.

# API

`````)

(print-docs-for "set"
  ["`set/empty` is the empty set"]
  ["`:+` is an alias for `set/union`"
   "`:*` is an alias for `set/intersection`"
   "`:-` is an alias for `set/difference`"])

(print
`````
# Gotchas

Unlike Janet's tables and structs, Jimmy data structures can contain `nil`:

```
repl:1:> (import jimmy/set)
repl:2:> (set/new 1 2 3 nil)
<jimmy/set {3 1 nil 2}>
```

In order to support iteratation with Janet's `next` protocol for unordered containers, Jimmy boxes the corresponding C++ iterator into an abstract type to use as the key. This means that a simple `(each el set (print el))` will allocate memory for the abstract type containing the iterator. It's not a *lot* of memory, but it's short-lived garbage and Janet's GC currently does nothing to optimize for short-lived garbage. This probably won't cause performance problems, just something to bear in mind.

I cannot think of a reasonable way to support iteration over `map` values with a meaningful key, because `immer` does not exposes enough functionality to retrieve the next key given an extant key in anything less than linear time. `immer::map`'s `find` returns a *pointer* rather than an iterator that we could use to advance to implement this. Jimmy does not currently have bindings for `map` at all, though, so you can't tell.

Also, I don't know C++, which is sort of a flaw when trying to write bindings for a C++ library, so I would caution you to read the code with a very critical eye. Code review welcome!

# TODO

- [ ] add bindings for `immer::map`
- [ ] 1.0 release
- [ ] iterators do not keep their backing data structures alive
`````)
