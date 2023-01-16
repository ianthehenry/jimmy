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
