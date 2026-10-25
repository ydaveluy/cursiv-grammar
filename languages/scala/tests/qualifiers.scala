package com.example.qualifiers

/* a comment /* nested */ still going */
class Base {
  def greet: String = "base"
}

class Derived extends Base {
  override def greet: String = {
    val viaThis = this.toString
    val viaSuper = super[Base].greet
    viaSuper
  }
}

class Counter(private[this] var count: Int) {
  private[this] def bump(): Unit = {
    count = count + 1
  }

  def next(step: Int): Int = {
    bump()
    count = count + step
    count
  }
}

object Functions {
  type Reducer = (=> Int, Int*) => Int

  val combine: Reducer = (seed, xs) => seed
}
