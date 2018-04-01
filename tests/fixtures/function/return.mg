
func no_return()
	x = 1

func test1(x)
	func test11(x, y)
		return (func(x, y, z) return (x + y) * z)(x, y, 3)
	return test11(x, 2)

func test2()
	func test21(x, y)
		print(x, "+", y, "=", x + y)
		return x + y
	return test21

print(no_return())
print(test1(1))
print(test2()(1, 2))
