# lilac
Tiny easily embeddable functional programming language inspired by Objective-C

## Hello World
```
-- IO *is* a monad
def main () -> IO () = do
  -- send a message to Console, the equivalent to Smalltalk's Transcript
  [Console show: "Hello world!"]
  [IO pure]
end
```

## Typeclasses as Protocols
Typeclasses are represented as protocols, which expose messages that an object must conform to (as well as optional messages).

An object can conform to multiple protocols at once, thus we do not need MTL style transformers since we can simply make `Ctx` conform to `Reader` and `State` simultaneously in this below example.

```
protocol Functor' f
  message map (self: f a, transform: \(a -> b)) -> f b
end

protocol Monad' m
  message bind (self: m a, to: \(a -> m b)) -> m b
  static message pure (value: a) -> m a
end

...

implement Reader Ctx on Ctx
  message ask (self: Ctx) = self.ctx
end

implement State DBConnection on Ctx
  message put (self: Ctx, new-state: DBConnection) = self with { db: new-state }
  ...
end
```

## Codegen
Lilac supports two backends for code generation:
- **C**: C backend using `lilac_msg_send` in-place of `objc_msgSend`
- **Objective-C**: Objective-C `@protocol/@interface/@implementation` using native `objc_msgSend`

Neither backend is currently complete, though the Objective C one is more usable.
See `NOTES.md` for further implementation details.