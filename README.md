# jit-aot-compilers-course
A project about developing JIT/AOT compilers in virtual machines

#### Task 1
Make ir and API for factorial.

Minimal IR set:
```
Param n
Const c
Mul a, b
Add a, b
CmpLE a, b
BrCond cond, block_true, block_false
Br block
Ret val
```

### How to run (with unit tests)
```
cmake -S . -B build -G "Ninja" -DBUILD_TESTS=ON 
cmake --build build
./build/tests
```