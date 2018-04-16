
func f(value)
	print(value)
	return value

print(1 ?? 2)
print(1 ?? null)
print(null ?? 2)
print(null ?? null)
print()

print(f(1) ?? f(2))
print()
print(f(1) ?? f(null))
print()
print(f(null) ?? f(2))
print()
print(f(null) ?? f(null))
print()
