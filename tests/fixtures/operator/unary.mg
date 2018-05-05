
assert -0 == 0
assert -(-0) == 0
assert -(-(-0)) == 0

assert -1 == -1
assert -(-1) == 1
assert -(-(-1)) == -1

assert -0.0 == -0.0
assert -(-0.0) == 0.0
assert -(-(-0.0)) == -0.0

assert -1.0 == -1.0
assert -(-1.0) == 1.0
assert -(-(-1.0)) == -1.0

assert +1 == 1
assert +1.0 == 1.0

assert not true == false
assert not not true == true
assert not not not true == false
