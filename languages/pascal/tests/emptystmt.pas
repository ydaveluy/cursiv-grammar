{ A statement slot may read nothing: a body, a branch, a case arm. }
program emptystmt;
label 1;
begin
  while x do ;
  if a then else b;
  if a then if b then else c;
  case x of 1: ; 2: foo end;
  repeat until x;
  for i := 1 to 2 do ;
  with r do ;
  1: ;
end.
