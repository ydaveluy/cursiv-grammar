MATCH (a:Person {name: "Ann", age: 0x1F})
WHERE a.age IS NOT NULL AND NOT a.name STARTS WITH "X" AND a.name ENDS WITH "n"
  AND a.name CONTAINS "n" AND (a.age > 0 XOR a.age < 0)
WITH a, a.age AS age
OPTIONAL MATCH (a)-[r:KNOWS|LIKES*1..3]->(b:Person)
WHERE b.age IS NULL
WITH a, b, r
MERGE (c:Person {name: "Cy"})
  ON CREATE SET c.created = true, c += {seen: 1}, c:New
  ON MATCH SET c.seen = c.seen + 1
WITH a, b, c
UNWIND [1, 2, 3] AS n
WITH a, b, c, collect(n) AS ns
CALL db.labels() YIELD label
WITH a, b, c, ns, label
SET a:Verified
REMOVE a:Old
DETACH DELETE c
RETURN a, `weird name`, ns[0], ns[0..1], ns[..1], ns[1..],
       CASE WHEN age > 10 THEN "big" ELSE "small" END,
       CASE label WHEN "X" THEN 1 ELSE 0 END,
       ALL(x IN ns WHERE x > 0),
       ANY(x IN ns WHERE x > 0),
       NONE(x IN ns WHERE x < 0),
       SINGLE(x IN ns WHERE x = 1),
       [x IN ns WHERE x > 0 | x * x],
       [p = (a)-->(b) WHERE b.age > 0 | p],
       count(*),
       2 ^ 3,
       EXISTS { (a)-->(b) },
       EXISTS { MATCH (a)-->(b) RETURN a },
       a:Person,
       $name, $0,
       "escaped: \n\t\"quote\" A \101",
       'single \'quoted\''
