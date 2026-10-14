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