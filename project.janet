(declare-project :name "jimmy")

(declare-native
  :name "jimmy/native"
  :source ["src/jimmy.cpp"]
  :headers ["src/set.cpp" "src/map.cpp" "src/vec.cpp"]
  :cppflags ["-Iimmer" "-std=c++14"])

(declare-source
  :source [
    "src/set.janet"
    "src/map.janet"
    "src/vec.janet"
    "src/util.janet"
    "src/init.janet"
  ]
  :prefix "jimmy")
