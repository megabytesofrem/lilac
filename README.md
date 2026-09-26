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

-- Function calls are ML style
def insert-item-if-not-existing[a] (l: List[a], v: a) =
  if not List.contains-elem l v
    then List.insert-elem l v
    else List.get l v


-- Generic function
def id-of[a](a': a) = a'
```

### Objective C style OOP
Lilac encourages the usage of Objective C/Smalltalk style messaging between immutable objects

```
struct Window
  private width: i32
  private height: i32
end

struct App
  private win: Window
end

protocol App'
  message get-window (self: App) -> Window
end

implement App' on App
  message get-window (self: App) = self.win
end

protocol Window'
  message resize (self: Window, to-width: i32, height: i32) -> unit
end

implement Window' on Window
  message resize (self: Window, to-width: i32, height: i32) = block
    self.width = to-width
    self.height = to-height
  end
end

-- Use it like so
def handle_app (self: App) -> unit = block
  @(@(self get-window) resize to-width:640 height:480)
end
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