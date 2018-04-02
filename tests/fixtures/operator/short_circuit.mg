
func test(value)
	print(value)
	return value

print(test(false))
print(test(true))
print()

print(1, test(false) and test(false))
print(2, test(true) and test(false))
print(3, test(false) and test(true))
print(4, test(true) and test(true))
print()

print(5, test(false) or test(false))
print(6, test(true) or test(false))
print(7, test(false) or test(true))
print(8, test(true) or test(true))
