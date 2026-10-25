package CoverageTests
  import Modelica.SIunits.{Voltage, Current};
  import Icons = Modelica.Icons;
  import Modelica.Blocks.*;

  encapsulated partial model Base
    Real x;
  equation
    x = 0;
  end Base;

  type Color = enumeration(Red, Green, Blue);
  type AnyColor = enumeration(:);
  type NoColor = enumeration();

  function derTest = der(SomeFunc, t);

  model Extended
    extends Base(x = 1) annotation(Icon);
  protected
    Real y[3];
    parameter Integer n = 3;
  public
    Real m[2, 2] = [1, 2; 3, 4];
    Real xs[3] = {i ^ 2 for i in 1:n};
    Real last = y[end];
    Real any = y[:];
    Real k = 1:2:10;
    Real g = if n > 0 then 1 elseif n < 0 then -1 else 0;
    Real h = (1 + 2) * 3 ^ 2;
  initial algorithm
    for i in 1:3 loop
      y[i] := 0;
    end for;
  algorithm
    while n > 0 loop
      n := n - 1;
      if n == 1 then
        break;
      elseif n < 0 then
        return;
      end if;
    end while;
    when n > 0 then
      y[1] := 1;
    elsewhen n < 0 then
      y[1] := -1;
    end when;
    (y[1], y[2]) := Sub.pair(n);
  equation
    connect(y[1], y[2]);
    when n > 0 then
      y[1] = 1;
    elsewhen n < 0 then
      y[1] = -1;
    end when;
    if n > 0 and not (n < 0) or n == 0 then
      y[3] = 1;
    end if;
  end Extended;

  model Redeclared
    redeclare replaceable model M = Base constrainedby Base;
    redeclare Real z = 1;
  end Redeclared;

  model UsesReplaceable
    replaceable model M2 = Base constrainedby Base;
    replaceable Real w;
  end UsesReplaceable;

  function ExtFunc
    input Real a;
    output Real b;
  external "C" b = extFunc(a) annotation(Library = "ext");
  end ExtFunc;
end CoverageTests;
