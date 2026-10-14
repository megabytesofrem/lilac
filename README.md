# Lilac

A modern programming language inspired by Objective-C.

## Why another language?
Lilac is designed to be a versatile and modern language for low-level systems programming, based on a micro-runtime (`lilac_msgsend`/`lilac_retain`).

## Building
The language can be built using CMake: `cmake --build build`, and the tests can be ran using `ctest --test-dir build`.


## Syntax

### Messages and Calls
- Classical C function call for FFI (the equivalent of `NSLog`): `Print!("Hello world!);`
- Smalltalk/Objective C messaging: `[String byJoining: #["hello", "world"] with: ','];`

### Classes
Classes define fields, and methods, as well as any methods overriden from a conformance.
Fields are private by design to maintain the Smalltalk object model that Lilac borrows from Objective-C.

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

### Protocols
Protocols define methods that a class can implement by conformance. They behave identically to Objective-C protocols.

`impl T` and `x isa T` are used to require and test for protocol conformance.

```groovy
protocol Shape {
  method isQuadrilateral (self) -> Bool

  method hasFiniteSides (self) -> Bool
  method getSides (self) -> Int
}

class Circle conforms (Shape) {
  field radius: Int

  override isQuadrilateral (self) -> Bool { return no; }
  override hasFiniteSides (self) -> Bool { return no; }
  override getSides (self) -> Int { return 0; }
}

class Rectangle conforms (Shape) {
  field width: Int
  field height: Int

  override isQuadrilateral (self) -> Bool { return yes; }
  override hasFiniteSides (self) -> Bool { return yes; }
  override getSides (self) -> Int { return 4; }
}

// A more complex example
class ShapeSorter conforms (Object) {
  field rects: #[impl Shape] = #[]
  field circles: #[impl Shape] = #[]

  field allShapes: #[impl Shape] = #[
    Circle { radius: 10 },
    Circle { radius: 16 },
    Rectangle { width: 64, height: 100 },
  ]

  def sortShapes (self) -> Void {
    foreach (let shape: impl Shape in allShapes) {
      if (shape isa Rectangle) {
        [rects appendElement: shape as Rectangle];
      }

      if (shape isa Circle) {
        [circles appendElement: shape as Circle];
      }
    }
  }
}

```

### ADTs
Lilac gives the developer the power of Rust ADTs.

```swift
enum Result<A> {
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

implement ResultOf for Result<A> {
  override isAnError (self) -> Bool { 
    match self {
      Ok { .. } => false,
      Err { ..} => true
    }
  }

  override isSuccessful (self) -> Bool { return ![self isAnError]; }
}
```

### Pattern Matching
Lilac is a modern language: so of course it supports pattern matching.

```swift
enum Result<A> {
  Ok { 
    value: A 
  },

  Err {
    message: String
  }
}

class DBAccessor {
  field db: ~DBConnection

  def tryConnect (self) toDB:db:(DBConnection) -> Result<DBConn> { ..  }

  def insertIntoRow (self) ?named:(String) ?value:(String) -> Result<DBConn> { .. }
  
  // later on
  def attemptDBWrite (self) ofValue:value:(String) -> Result<DBConn> {
    let writeResult = [db insertIntoRow named: "users" value: "charlotte"];
    match writeResult {
      Ok { value }    => {
        return Ok { value: value };
      }
      Err { message } => {
        let formatted = [String byAppending: #["[Error] ", message ]];
        return Err { message: formatted };
      }
    }
  }
}
```

### Dropping down to C: Static Dispatch
To support bare-metal development, Lilac allows calling and defining procedural functions.

```groovy
class PageTable {
  // Static dispatch C function
  static def AllocatePage!(start_addr: *U32, page_size: U32) -> *Void { }
}

// Called like so:
PageTable.AllocatePage!(0xcafe, 40000);
```

### Implementation Details
Lilac does not target the Executable and Linking Format (ELF), as ELF lacks the ability to express dynamic
selector based messaging. Lilac compiles to freestanding Mach-O using the `x86_64-apple-darwin` flag in `clang`, and provides `lilac-dyld` to parse and read Mach-O binaries and libraries on a Linux host.

Build command: `clang --target=x86_64-unknown-darwin -ffreestanding -nostdlib -fuse-ld=lld main.c -o main`