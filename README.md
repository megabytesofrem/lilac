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
Lilac encourages the usage of Objective C/Smalltalk style messaging between immutable objects.

Protocols expose messages, and the `implement` keyword implements a protocol
on a type.

```
struct Win
  title: str
  width: i32
  height: i32
end

struct App
  win: Win
end

protocol App'
  message get-window (self: App) -> Window
end

implement App' on App
  message get-window (self: App) = self.win
end

protocol Win'
  message resize (self: Win, to-width: i32, and-height: i32) -> Win
  message titled (self: Win, text: str) -> Win
end

implement Win' on Win
  message resize (self: Win, to-width: i32, and-height: i32) = self with
    { width: to-width, height: and-height }

  message titled (self: Win, text: str) = self with { title: text }
end

-- later on
let curr-win: Win = [app get-window]
let win': Win = [[curr-win resize to-width:640 and-height:480] titled text:"A window"]
```

Lilac also provides a shorthand for protocols: extension methods.
Extension methods are desugared to protocol implementations

```
def i32.squared (self) -> self = self * self
def i32.cubed (self) -> self = self * self * self
```

Likewise, this shorthand also works on specific protocol extensions:
```
def Ctx.(Reader.ask) (self) -> Ctx = self.ctx
```

### Typeclasses
Typeclasses _are_ protocols, and their methods are messages.

```
protocol Functor f
  message map (self: f a, transform: \a -> b) -> f b
end

protocol Monad m
  message bind (self: m a, to: \a -> m b) -> m b
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
do
  as
  bs
end
```

## Codegen
Lilac supports two backends for code generation:
- **C**: C backend using `lilac_msg_send` in-place of `objc_msgSend`
- **Objective-C**: Objective-C `@protocol/@interface/@implementation` using native `objc_msgSend`

Neither backend is currently complete, though the Objective C one is more usable.