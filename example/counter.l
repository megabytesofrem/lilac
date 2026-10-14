class Counter conforms (Object) {
  field _value: I32 = 0

  override init (self) -> Counter {
    return Counter { _value: 0 };
  }

  def value (self) -> I32 {
    return self._value;
  }

  def incrementCounter (self) -> Void {
    self._value += 1;
  }

  def incrementCounterBy (self) ?byValue:(I32) -> Void {
    self._value += byValue;
  }
}

class Main {
  field counter: ~Counter;

  def main (self) -> Void {
    counter = [[Counter alloc] init];

    // Increment the counter 5 times
    for (let i: I32 = 1; i < 5; i += 1) {
      [counter incrementCounter];
    }

    // Display the resulting value
    Printf("final counter value: %d\n", [counter value]);
  }
}