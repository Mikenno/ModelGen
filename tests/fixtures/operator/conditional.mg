
assert (null ?? null) == null
assert (null ?? 0) == 0
assert (0 ?? null) == 0
assert (0 ?? 0) == 0
assert (0 ?? 1) == 0
assert (1 ?? 0) == 1
assert (1 ?? 1) == 1
assert (null ?? 2) == 2
assert (0 ?? 2) == 0
assert (1 ?? 2) == 1

assert (null ?: null) == null
assert (null ?: 0) == 0
assert (0 ?: null) == null
assert (0 ?: 0) == 0
assert (0 ?: 1) == 1
assert (1 ?: 0) == 1
assert (1 ?: 1) == 1
assert (null ?: 2) == 2
assert (0 ?: 2) == 2
assert (1 ?: 2) == 1

assert (null ? null : null) == null
assert (null ? null : 0) == 0
assert (0 ? 0 : null) == null
assert (0 ? 0 : 0) == 0
assert (0 ? 0 : 1) == 1
assert (1 ? 1 : 0) == 1
assert (1 ? 1 : 1) == 1
assert (null ? null : 2) == 2
assert (0 ? 0 : 2) == 2
assert (1 ? 1 : 2) == 1

assert (null ?? null) == null
assert (null ?? []) == []
assert ([] ?? null) == []
assert ([1] ?? null) == [1]
assert ([] ?? []) == []
assert ([] ?? [1]) == []
assert ([1] ?? []) == [1]
assert ([1] ?? [2]) == [1]

assert (null ?: null) == null
assert (null ?: []) == []
assert ([] ?: null) == null
assert ([1] ?: null) == [1]
assert ([] ?: []) == []
assert ([] ?: [1]) == [1]
assert ([1] ?: []) == [1]
assert ([1] ?: [2]) == [1]
