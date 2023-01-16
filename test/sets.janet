(import ../src/set)
(use ./helpers)

# Basics

(assert=
  (set/add (set/new 1 2) 3)
  (set/remove (set/new 1 2 3 4) 4))

(def x (set/new 1 2 3))
(def y (set/new 1 2 3))
(assert= x y)
(set/add x 4 5 6)
(set/remove y 1 2 3)
(assert= x y)

(assert= (set/add x 4 5 6) (set/new 1 2 3 4 5 6))
(assert= (set/remove x 1 2 3) (set/new))
(assert= (set/new) set/empty)

# Of

(assert= (set/new 1 2 3) (set/of [1 2 3]))
(assert= (set/new 1 2 3) (set/of @[1 2 3]))
(assert= (set/new 1 2 3) (set/of (coro (yield 1) (yield 2) (yield 3))))
(assert= (set/new 1 2 3) (set/of {:a 1 :b 2 :c 3}))

(assert= (set/new :a :b :c) (set/of-keys {:a 1 :b 2 :c 3}))
(assert= (set/new 0) (set/of-keys (coro (yield 1) (yield 2))))

# Length

(assert (= 1 (length (set/new nil nil nil))))

# Marshaling

(assert-round-trip (set/new 1 2 3))

# Union

(assert= (set/union (set/new 1) (set/new 2) (set/new 3)) (set/new 1 2 3))
(assert= (set/union (set/new 1 2 3) (set/new 2 3 4))
              (set/union (set/new 1) (set/new 2) (set/new 3) (set/new 4)))
(assert= (set/union) set/empty)
(assert-throws (set/union (set/new 1) 2) "bad slot #1, expected jimmy/set, got 2")

# Intersection

(assert= (set/intersection (set/new 1 2 3) (set/new 2 3 4) (set/new 3 4 5)) (set/new 3))
(assert= (set/intersection (set/new 1 2 3) (set/new 2 3 4)) (set/new 2 3))
(assert= (set/intersection (set/new 1 2 3) (set/new 2 3 4))
              (set/intersection (set/new 1 2 3 4 5) (set/new 2 3) (set/new 1 2 3 4 5 6)))
(assert= (set/intersection (set/new 1 2 3) (set/new 4 5 6)) set/empty)
(assert-throws (set/intersection (set/new 1) 2) "bad slot #1, expected jimmy/set, got 2")

# Difference

(assert= (set/difference (set/new 1 2 3) (set/new 1) (set/new 2)) (set/new 3))
(assert= (set/difference (set/new 1 2 3 4 5) (set/new 2 3))
              (set/difference (set/new 1 4 5 6 7) (set/new 6) (set/new 7)))
(assert= (set/difference (set/new 1 2 3) (set/new 1 2 3)) set/empty)

(assert-throws (set/difference 1 2) "bad slot #0, expected jimmy/set, got 1")
(assert-throws (set/difference (set/new 1) 2) "bad slot #1, expected jimmy/set, got 2")

# Operator overloading

(assert= (+ (set/new 1 2 3) (set/new 2 3 4)) (set/new 1 2 3 4))
(assert= (* (set/new 1 2 3) (set/new 2 3 4)) (set/new 2 3))
(assert= (- (set/new 1 2 3) (set/new 2 3 4)) (set/new 1))

# Subset

(assert (set/subset? (set/new 1 2) (set/new 1 2 3)))
(assert (not (set/subset? (set/new 1 2 3) (set/new 1 2))))
(assert (set/subset? (set/new 1 2 3) (set/new 1 2 3)))
(assert (set/strict-subset? (set/new 1 2) (set/new 1 2 3)))
(assert (not (set/strict-subset? (set/new 1 2 3) (set/new 1 2 3))))

# Superset

(assert (set/superset? (set/new 1 2 3) (set/new 1 2)))
(assert (not (set/superset? (set/new 1 2) (set/new 1 2 3))))
(assert (set/superset? (set/new 1 2 3) (set/new 1 2 3)))
(assert (set/strict-superset? (set/new 1 2 3) (set/new 1 2)))
(assert (not (set/strict-superset? (set/new 1 2 3) (set/new 1 2 3))))

# Tuple/array conversions

(assert= [1] (set/to-tuple (set/new 1)))
(assert (deep= @[1] (set/to-array (set/new 1))))

(assert (deep= @[1 2 3] (sorted (set/to-tuple (set/new 1 2 3)))))
(assert (deep= @[1 2 3] (sort (set/to-array (set/new 1 2 3)))))

# Map

(assert= (set/map (set/new 1 2 3) |(* $ 2)) (set/new 2 4 6))
(assert= (set/map (set/new 1 2 3) {1 10 2 15}) (set/new 10 15 nil))

# Filter-map

(assert= (set/filter-map (set/new 1 2 3) |(if (= $ 2) nil (* $ 2))) (set/new 2 6))

# Filter

(assert= (set/filter (set/new 1 2 3 4 5) odd?) (set/new 1 3 5))
(assert= (set/filter (set/new 1 2 3 4 5) (set/new 1 2)) (set/new 1 2))

# Reduce

(assert= (set/reduce (set/new 1 2 3 4 5) 0 +) 15)

# Count

(assert= (set/count (set/new 1 2 3 4 5) odd?) 3)
(assert= (set/count (set/new 1 2 3 4 5) (set/new 1 2)) 2)

# Callable

(assert= ((set/new 1 2 3 4 5) 1) true)
(assert= ((set/new 1 2 3 4 5) 6) false)
