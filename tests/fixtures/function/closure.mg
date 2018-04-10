
func add(x)
	return func(y) return x + y

add1 = add(1)
add5 = add(5)

print(add1(3))
print(add5(3))

add1.x = 10
add5.x = 50

print(add1(3))
print(add5(3))
