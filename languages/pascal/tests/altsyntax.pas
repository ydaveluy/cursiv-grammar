program AltSyntaxDemo(output);

type
  IntFunc = function(x: integer): integer;
  Notifier = procedure(n: integer);
  Vec = array (.1..3.) of integer;

var
  v: Vec;
  s: set of 1..5;
  op: IntFunc;
  cb: Notifier;

begin
  v(.1.) := 10;
  v(.2.) := 20;
  v(.3.) := 30;
  s := (.1, 2, 3.);
  writeln(v(.1.) + v(.2.) + v(.3.));
end.
