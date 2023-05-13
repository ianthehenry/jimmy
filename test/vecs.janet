(import ../src/vec)
(use ./helpers)

# Basics

(assert=
  (vec/push (vec/new 1 2) 3)
  (vec/pop (vec/new 1 2 3 4)))

(def x (vec/new 1 2 3))
(def y (vec/new 1 2 3))
(assert= x y)
(vec/push x 4 5 6)
(vec/popn y 3)
(assert= x y)

(assert= (vec/push x 4 5 6) (vec/new 1 2 3 4 5 6))
(assert= (vec/popn x 3) (vec/new))
(assert= (vec/new) vec/empty)

# Of

(assert= (vec/new 1 2 3) (vec/of [1 2 3]))
(assert= (vec/new 1 2 3) (vec/of @[1 2 3]))
(assert= (vec/new 1 2 3) (vec/of (coro (yield 1) (yield 2) (yield 3))))

# Length

(assert (= 3 (length (vec/new nil nil nil))))
(assert (= 0 (length (vec/new))))
(assert (= 6 (length (vec/of [1 2 3 4 5 6]))))

# Marshaling

(assert-round-trip (vec/new 1 2 3))

# Put

(assert= x (vec/put x 0 1))
(assert= (vec/new 2 2 3) (vec/put x 0 2))
(assert= (vec/new 1 0 3) (vec/put x 1 0))
(assert-throws (vec/put x 3 0) "expected integer key in range [0, 3), got 3")

# Tuple/array conversions

(assert= [1] (vec/to-tuple (vec/new 1)))
(assert (deep= @[1] (vec/to-array (vec/new 1))))

(assert (deep= @[1 2 3] (sorted (vec/to-tuple (vec/new 1 2 3)))))
(assert (deep= @[1 2 3] (sort (vec/to-array (vec/new 1 2 3)))))

# Map

(assert= (vec/map (vec/new 1 2 3) |(* $ 2)) (vec/new 2 4 6))
(assert= (vec/map (vec/new 1 2 3) {1 10 2 15}) (vec/new 10 15 nil))

# Filter-map

(assert= (vec/filter-map (vec/new 1 2 3) |(if (= $ 2) nil (* $ 2))) (vec/new 2 6))

# Filter

(assert= (vec/filter (vec/new 1 2 3 4 5) odd?) (vec/new 1 3 5))
(assert= (vec/filter (vec/new 1 2 3 4 5) {1 true 2 true}) (vec/new 1 2))

# Reduce

(assert= (vec/reduce (vec/new 1 2 3 4 5) 0 +) 15)

# Count

(assert= (vec/count (vec/new 1 2 3 4 5) odd?) 3)
(assert= (vec/count (vec/new 1 2 3 4 5) {1 true 2 true}) 2)

# Callable

(assert= ((vec/new 1 2 3 4 5) 1) 2)
(assert-throws ((vec/new 1 2 3 4 5) 5) "expected integer key in range [0, 5), got 5")

# Iteration

(assert= (next (vec/new 1 2 3)) 0)
(assert= (next (vec/new 1 2 3) 0) 1)
(assert= (next (vec/new 1 2 3) 1) 2)
(assert= (next (vec/new 1 2 3) 2) nil)

(assert= [1 2 3] (tuple/slice (sorted (seq [x :in (vec/new 2 3 1)] x))))
