(import /set)

(defn assert-equal [a b]
  (assert (= a b))
  (assert (= (hash a) (hash b))))

(assert-equal
  (set/add (set/new 1 2) 3)
  (set/remove (set/new 1 2 3 4) 4))

(def x (set/new 1 2 3))
(def y (set/new 1 2 3))
(assert-equal x y)
(set/add x 4 5 6)
(set/remove y 1 2 3)
(assert-equal x y)

(assert-equal (set/add x 4 5 6) (set/new 1 2 3 4 5 6))
(assert-equal (set/remove x 1 2 3) (set/new))
(assert-equal (set/new) set/empty)

(assert (= 1 (length (set/new nil nil nil))))
