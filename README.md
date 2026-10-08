# Lilac

A modern programming language inspired by Objective-C. Lilac is Objective-C's big sister.

## Why another language?
Lilac is designed to be a versatile and modern language for low-level systems programming, based on a micro-runtime (`lilac_msgsend`/`lilac_retain`).

## Syntax

### Messages and Calls
- Classical C function call for FFI (the equivalent of `NSLog`): `printf!("Hello world!);`
- Smalltalk/Objective C messaging: `String byJoining: #["hello", "world"] with: ',';`

### Classes
```groovy
class Person conforms (Object) {
  field name: String
  field age: String

  // describe:self -> String is provided by conforming to 'Object'
  override describe (self) -> String {
    [String byJoining: #[name, " and ", age, "years old"] with: ' '];
  }

  def canDrive (self) -> Bool { self.age >= 18 }
}
```

### ADTs
Lilac gives the developer the power of Rust ADTs.

```swift
enum Result[A] {
  Ok { 
    value: A 
  },

  Err {
    message: String
  }
}

protocol ResultOf {
  isAnError (self) -> Bool,
  isSuccessful (self) -> Bool
}

implement ResultOf for Result[A] {
  override isAnError (self) -> Bool { 
    match self {
      Ok { .. } => false,
      Err { ..} => true
    }
  }

  override isSuccessful (self) -> Bool { return ![self isAnError]; }
}
```

### Higher Kinded Types
```swift
protocol FunctorOf [F: (* -> *) -> *, A: * -> *, B: * -> *] {
  map (self) action:(Fn(F[A]) -> F[B]) -> F[B] 
}

implement FunctorOf for Result[A] {
  override map (self) action:(Fn(Result[A]) -> Result[A]) -> Result[A] {
    match self {
      Ok { value } => [self action],
      Err { .. } => ...
    }
  }
}
```

### Dropping down to C: Static Dispatch
To support bare-metal development, Lilac allows calling and defining bang-functions (functions which are
marked as static dispatch).

Bang-functions are called with seperate syntax from methods and are used for interoperability with C
and performance critical applications.

```groovy
class PageTable {
  // Static dispatch C function
  static def AllocatePage!(start_addr: *U32, page_size: U32) -> *Void { }
}

// Called like so:
PageTable.AllocatePage!(0xcafe, 40000);
```
