program VariantRecordDemo(output);

type
  ShapeKind = (circle, rectangle, triangle);
  Flag = 0..1;

  Shape = record
    name: string[20];
    case kind: ShapeKind of
      circle: (radius: real);
      rectangle: (width, height: real);
      triangle: (base, height2: real;
                 case isRight: Flag of
                   0: (hypotenuse: real);
                   1: (unused: integer))
  end;

  Choice = record
    case integer of
      0: (a: integer);
      1: (b: real)
  end;

var
  s: Shape;
  c: Choice;

begin
  s.kind := circle;
  s.radius := 2.5;
  c.a := 1;
  writeln(s.name);
end.
