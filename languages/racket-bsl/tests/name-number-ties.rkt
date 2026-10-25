; A bare digit run is a same-length NAME and NUMBER both. A longer NAME
; still wins by longest match (2x, 1+, 3rd); an equal-length tie (100, 3/2,
; 5.5) goes to NUMBER, the only reading a useful AST can give a bare
; numeral.

(define (2x n) (* n 2))

(define (compare 1+ 3rd)
  (if (> 1+ 3rd) 1+ 3rd))

(check-expect (2x 4) 8)
(check-expect (compare 5 3) 5)
(check-expect 100 100)
(check-expect 3/2 3/2)
(check-expect 5.5 5.5)
