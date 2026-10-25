xquery version "4.0" encoding "UTF-8";

(: nested (: comment :) still a comment :)

declare namespace ex = "http://example.com/ex";
declare default element namespace "http://example.com/default";
declare default function namespace "http://example.com/fn";
declare boundary-space strip;
declare default collation "http://www.w3.org/2005/xpath-functions/collation/codepoint";
declare base-uri "http://example.com/base/";
declare construction preserve;
declare ordering ordered;
declare default order empty greatest;
declare copy-namespaces preserve, inherit;
declare decimal-format ex:df decimal-separator = "." grouping-separator = ",";
declare context item as node() external;
declare option ex:opt "value";
declare record ex:Point(x as xs:double, y as xs:double := 0.0);
declare type ex:IntList as xs:integer;
declare %public function ex:add($a as xs:integer, $b as xs:integer := 1) as xs:integer {
  $a + $b
};
declare variable $ex:limit as xs:integer external := 10;

$a instance of xs:integer;
$a treat as xs:integer;
$a castable as xs:integer;
$a cast as xs:integer?;
$a instance of enum("red", "green", "blue");
$a instance of (xs:integer | xs:string);
$a instance of function(*);
$a instance of function(xs:integer, xs:integer) as xs:integer;
$a instance of map(*);
$a instance of map(xs:string, xs:integer);
$a instance of array(*);
$a instance of array(xs:integer);
$a instance of record(*);

$m +:= $n;
$m otherwise $n;
$a => upper-case() => normalize-space();
$a -> upper-case();
$a =!> upper-case();

for tumbling window $w in 1 to 10
  start $s at $p when true()
  only end $e at $q when true()
return $w;

for sliding window $w in 1 to 10
  start $s when true()
  end $e when true()
return $w;

for member $m in [1, 2, 3] return $m;
for key $k value $v in map { "a": 1 } return $k;
for value $v in map { "a": 1 } return $v;

let $($a, $b) := (1, 2) return $a;
let ${$x, $y} := map { "x": 1, "y": 2 } return $x;
let $[$p, $q] := [1, 2] return $p;

for $x in (1, 2, 3) group by $g := $x collation "http://www.w3.org/2005/xpath-functions/collation/codepoint" return $g;
for $x in (3, 1, 2) stable order by $x descending empty least collation "http://www.w3.org/2005/xpath-functions/collation/codepoint" return $x;

some $x in (1, 2) satisfies $x > 1;
every $x in (1, 2) satisfies $x > 0;

try { 1 div 0 } catch err:FOAR0001 | err:FOAR0002 { "caught" } finally { trace("done") };

typeswitch ($a)
  case $i as xs:integer | xs:decimal return "num"
  default $d return "other";

$a?name;
$a?*;
$a?("k");
.?name;

element #ex:a { "text" };
attribute #ex:a { "text" };
namespace #ex { "http://example.com/ex" };
processing-instruction #ex { "data" };
document { <root/> };

insert node <x/> as first into <root/>;
insert nodes (<x/>, <y/>) after <root/>;
delete node <x/>;
delete nodes (<x/>, <y/>);
replace value of node <x/> with "y";
replace node <x/> with <y/>;
rename node <x/> as <y/>;
copy $x := <a/> modify insert node <b/> into $x return $x;

(# ex:pragma content #) { 1 + 1 };

validate lax { <a/> };
validate type ex:Point { <a/> };

`a {$x} b`;
``[literal `text`]``;

<a xmlns:ex="http://example.com/ex" attr="v{1+1}v" attr2='a''b'>
  text {{escaped}} <![CDATA[raw <data>]]> <!--a comment--> <?pi data?>
  <child/>
  {1 + 1}
</a>;

<!--standalone comment-->;
<?standalone-pi data?>;

0x1F;
0b1010;
1.5e10;
.5;
1.
