package com.example.features

import scala.collection.mutable.{Map => MMap, Set}
import scala.language._

sealed trait Shape {
  def area: Double
}

case class Circle(radius: Double) extends Shape {
  def area: Double = 3.14159 * radius * radius
}

case class Rectangle(width: Double, height: Double) extends Shape {
  def area: Double = width * height
}

class Counter(private var count: Int) {
  def this() = this(0)

  def increment(step: Int = 1): Unit = {
    count = count + step
  }

  def value: Int = count
}

object Shapes {
  def describe(shape: Shape): String = shape match {
    case Circle(r) if r > 0 => "circle"
    case Rectangle(w, h) => "rectangle"
    case _ => "unknown"
  }

  def totalArea(shapes: List[Shape]): Double = {
    var total = 0.0
    for (shape <- shapes; a = shape.area if a > 0) {
      total += a
    }
    total
  }

  def sumAll(xs: Int*): Int = xs match {
    case Nil => 0
    case head :: tail => head + sumAll(tail: _*)
  }

  val pair: (Int, String) = (1, "one")

  def identity[A](x: A): A = x

  def firstOf[A <: AnyRef](xs: List[A]): Option[A] =
    if (xs.isEmpty) None else Some(xs.head)

  type Handler = Int => Int

  val doubleIt: Handler = (x: Int) => x * 2

  lazy val greeting: String = "hello"

  implicit val defaultCounter: Counter = new Counter()

  def withImplicit(implicit c: Counter): Int = c.value

  trait Named { self =>
    def name: String
    def introduce: String = "I am " + self.name
  }

  val anon = new Named {
    def name: String = "anon"
  }

  def existential(xs: List[T] forSome { type T }): Int = xs.size

  def structural(x: { def close(): Unit }): Unit = x.close()

  val triple = """
    |multi
    |line
    |""".stripMargin

  try {
    sumAll()
  } catch {
    case e: Exception => 0
  } finally {
    ()
  }
}
