# lilac
Tiny easily embeddable programming language.

## Syntax

### Variables
```
let a-constant: i32 = 123
mut a-counter-var: i32 = 1
a-counter-var += 1 -- Increment a counter
```

### Functions
```
def simple-sum (a: i32, b: i32) -> i32 = a + b
def insert-to-list (l: List[i32], v: i32) = List.insert-item(l, v)

-- Generic function
def id-of[a](a`: a) = a`
```

### Control flow
```
if cond
 then then-expr
 else else-expr
```

```
-- for loops do not require a 'block'
for i in range-start..range-end do
end

-- loop until a condition is met
until cond
end

-- while loops do not exist; because while true is ugly
```

### Blocks
By default Lilac functions are single expressions, but Lilac provides
blocks to enable complex functions.

```
block
  stmt
  stmt
end

-- Example of blocks
def complex-function () -> unit = block
  
end
```