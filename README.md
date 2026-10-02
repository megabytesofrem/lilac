# lilac
Tiny functional programming language inspired by Objective-C/Smalltalk designed for systems programming. 

## Hello World
```
class Main {
  def main () -> IO () = do {
    Console show: "Hello world!".
  }.
}
```

## Foreign Messages
Lilac does not support free-form C function calls, so functions written in C must be exposed as messages.

```
protocol MyC {
  -- C functions can *only* be static
  foreign static message "add_two":add-two (_:first: i32, _:second: i32) -> i32
}

-- Use it like so 
MyC add-two first: 2 second: 4
```

## Typeclasses as Protocols
Typeclasses are represented as protocols, which expose messages that an object must conform to (as well as optional messages).

An object can conform to multiple protocols at once, thus we do not need MTL style transformers since we can simply make `Ctx` conform to `Reader` and `State` simultaneously in this below example.

```
protocol Functor' f {
  message map (self: f a, transform: \(a -> b)) -> f b
}

protocol Monad' m {
  message bind (self: m a, to: \(a -> m b)) -> m b
  static message pure (value: a) -> m a
}

...

class Ctx conforms (Reader Ctx, State Ctx) {
  override ask self: Ctx -> Ctx = self ctx.
  override put self: Ctx newValue: Ctx -> Ctx = self with { ctx: newValue }. 
}
```

## Codegen
Lilac supports two backends for code generation:
- **C**: C backend using `lilac_msg_send` in-place of `objc_msgSend`
- **Objective-C**: Objective-C `@protocol/@interface/@implementation` using native `objc_msgSend`

Neither backend is currently complete, though the Objective C one is more usable.
See `NOTES.md` for further implementation details.