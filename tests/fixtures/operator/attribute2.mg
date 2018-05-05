
a = {}
assert a == {}
a.a = {}
assert a == {"a": {}}
a.a.a = [null]
assert a == {"a": {"a": [null]}}
a.a.a[0] = [null]
assert a == {"a": {"a": [[null]]}}
a.a.a[0][0] = {}
assert a == {"a": {"a": [[{}]]}}
a.a.a[0][0].a = {}
assert a == {"a": {"a": [[{"a": {}}]]}}
a.a.a[0][0].a.a = {}
assert a == {"a": {"a": [[{"a": {"a": {}}}]]}}
a.a.a[0][0].a.a.a = [null]
assert a == {"a": {"a": [[{"a": {"a": {"a": [null]}}}]]}}
a.a.a[0][0].a.a.a[0] = 123
assert a == {"a": {"a": [[{"a": {"a": {"a": [123]}}}]]}}
