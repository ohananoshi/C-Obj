## C-Obj

### Python-like data structures for C, built on one pluggable value type

C-Obj gives C a single value type, `cobj`, that can hold a number, a string, a list, a map, a set, an n-d array or any type you define. Every type is described by a `cobj_type`: a small table of properties (clone, drop, compare, hash, format, and for containers length and iteration). The built-in types and the bundled data structures use that same table, so a new data structure plugs in by providing one, and from then on it works everywhere the others do: inside lists, as map keys, in comparisons when printed, and in any loop.