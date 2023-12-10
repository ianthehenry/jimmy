(import ../src/map)
(use ./helpers)

# Basics

(assert= (map/new 1 2 3 4) (map/new 3 4 1 2))
(assert-throws (map/new 1 2 3) "expected even number of arguments")

# Deep equality

(assert= (map/new 1 [2 3] 4 [5 6]) (map/new 1 [2 3] 4 [5 6]))
(assert-not= (map/new 1 [2 3] 4 @[5 6]) (map/new 1 [2 3] 4 @[5 6]))

# Length

(assert (= 1 (length (map/new nil 1))))
(assert (= 1 (length (map/new 1 nil))))

# Marshaling

(assert-round-trip (map/new 1 2 3 [1 2]))

# Callable

(assert= ((map/new 1 2 3 4) 1) 2)
(assert= ((map/new 1 2 3 4) 3) 4)
(assert-throws ((map/new 1 2 3 4) 5) "key 5 not found")
(assert= ((map/new 1 2 3 4) 5 :default) :default)

# Iteration

(def iterator (next (map/new 1 2 3 4)))
(assert-throws (get (map/new 1 2 3 4) iterator) "foreign iterator")

(assert= [[1 2] [3 4]] (tuple/slice (sorted (seq [x :in (map/new 1 2 3 4)] x))))
(assert= [] (tuple/slice (values map/empty)))

(def iterator (next (map/new 1 2 3 4)))
(assert= [[1 2] [3 4]] (tuple/slice (sorted
  (seq [x :in iterator] x))))

(def m (map/new 1 2 3 4))
(assert= [[1 2] [3 4]] (tuple/slice (sorted (seq [x :in m] x))))
(assert= [1 3] (tuple/slice (sorted (seq [x :in (map/keys m)] x))))
(assert= [2 4] (tuple/slice (sorted (seq [x :in (map/values m)] x))))
(assert= [[1 2] [3 4]] (tuple/slice (sorted (seq [x :in (map/pairs m)] x))))
