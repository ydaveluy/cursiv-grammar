"Literal arrays, nested literal arrays, dynamic arrays and dictionaries."
| numbers |
numbers := #(1 2.5 -3 16rFF #foo #at:put: (nested 1 2) 'text').
^{numbers. #{1. 'one'}}
