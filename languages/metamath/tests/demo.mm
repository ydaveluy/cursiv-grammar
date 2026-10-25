$( demo.mm -- exercises what the corpus samples do not: an include, a
   block with local scope, a disjoint variable statement, essential
   hypotheses, and both proof forms. $)

$[ constants.mm $]

$c 0 = -> ( ) wff term |- $.
$v x y ph ps $.

$d x y ph ps $.

tx $f term x $.
ty $f term y $.
wp $f wff ph $.
wq $f wff ps $.

${
  $d x y $.
  wex $e wff ph $.
  wim $a wff ( ph -> ps ) $.
$}

weq $a wff x = y $.
ax-1 $a |- ( ph -> ( ps -> ph ) ) $.
df.ex_1 $a |- ( ph -> ph ) $.

thm1 $p |- ( ph -> ( ps -> ph ) ) $= ax-1 $.

thm2 $p |- ( ph -> ph ) $= ? $.

thm3 $p |- ( ph -> ( ps -> ph ) ) $=
  ( ax-1 ) AA?BB $.
