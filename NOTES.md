## A few notes

Functional core, imperative shell. A majority of the code-base *should* be purely functional and Lilac
employs the existence of concepts from category theory (and Haskell) such as functors, applicatives, monads -
minus monad transformers: they stack naturally by protocols.

```
struct Ctx
  config: Config
  db: DBConnection
end

implement Reader Ctx on Ctx
  message ask (self: Ctx) = self.ctx
end

implement State DBConnection on Ctx
  message put (self: Ctx, new-state: DBConnection) = self with { db: new-state }
  ...
end

def main () -> IO () = do
  let ctx  = Ctx { config: .., db: [DBConnection open-connection-on:4444] } in
  let cfg  = [ctx ask].config in
  let db   = [ctx ask].db in
  let ctx' = [ctx put new-state:[db insert-row-named:"foo" value:"bar"]] in
  [ctx' return]
end
```


Message/selector call syntax:
```
[String by-appending-string: "Hello world"]   -- static message on type
[prime-numbers filtered-by: |n| [n is-even]]  -- single argument
[contents write-to-file-named: "file.txt" encoding: utf-8] -- multi-argument
```

Lilac's equivalent to NSObject:
```
protocol Object'
  message superclass (self: impl Object') -> impl Class'
  message is-equal (self: impl Object') -> bool
  message hash (self: impl Object') -> i64
end
```


Message dispatch is routed using Lilacs equivalent to `objc_msgSend` - `lilac_msg_send`
```c
lilac_msg_send(contents, SEL("write-to-file-named:encoding"), "file.txt", "utf-8");
```

For reference, this is Apple's implementation:
https://developer.apple.com/documentation/objectivec/objc_msgsend