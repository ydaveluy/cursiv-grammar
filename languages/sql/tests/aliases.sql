-- An alias spelled `AS <soft keyword>`: the join keywords and RAISE are names
-- after AS, and a bare alias may not read as the next join operator.
SELECT 1 FROM t AS NATURAL;
SELECT 1 FROM t AS LEFT;
SELECT 1 FROM t AS RAISE;
SELECT 1 FROM t AS TEMP;
SELECT a FROM t AS x NATURAL JOIN u;
SELECT a FROM t x LEFT JOIN u ON t.a = u.a;
SELECT a FROM t NATURAL JOIN u;
SELECT a FROM t AS LEFT LEFT JOIN u ON LEFT.a = u.a;
