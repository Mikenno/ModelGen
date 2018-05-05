
import math

func f1()
proc p1()

assert bool(null) == false
assert bool(false) == false
assert bool(true) == true

assert bool(-1) == true
assert bool(0) == false
assert bool(1) == true
assert bool(2) == true

assert bool(-1.0) == true
assert bool(-0.01) == true
assert bool(0.0) == false
assert bool(0.01) == true
assert bool(1.0) == true

assert bool("") == false
assert bool(" ") == true
assert bool("test") == true

assert bool(()) == false
assert bool((1,)) == true
assert bool((1, 2)) == true

assert bool([]) == false
assert bool([1]) == true
assert bool([1, 2]) == true

assert bool({}) == false
assert bool({"a": 1}) == true
assert bool({"a": 1, "b": 2}) == true

assert bool(print) == true
assert bool(range) == true

assert bool(f1) == true
assert bool((func())) == true

assert bool(p1) == true

assert bool(math) == true
