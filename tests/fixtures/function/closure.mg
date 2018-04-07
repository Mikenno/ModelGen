
func add(x)
	return func(y) return x + y

add1 = add(1)
add5 = add(5)

print(add1(3))
print(add5(3))
