#lang htdp/bsl

; Every construct this language accepts that the campaign's own sample does
; not reach: struct definitions, lambda-named functions, cond/if/and/or,
; every literal kind, every check- form, and every require form.

(define pi 3.14159)

(define-struct point (x y))

(define area-of
  (lambda (r) (* pi (* r r))))

(define (classify n)
  (cond
    [(< n 0) 'negative]
    [(= n 0) 'zero]
    [else 'positive]))

(define (sign n)
  (if (< n 0) -1 1))

(define (both-positive a b)
  (and (> a 0) (> b 0)))

(define (either-positive a b)
  (or (> a 0) (> b 0)))

'()
#true
#false
"a string with \"quotes\" inside"
#\a
#\space

(check-expect (sign 5) 1)
(check-random (sign 5) 1)
(check-within (area-of 1) 3.14159 0.01)
(check-member-of (sign 5) 1 -1 0)
(check-satisfied 5 positive?)
(check-error (/ 1 0))
(check-error (/ 1 0) "division by zero")

(require racket/list)
(require (planet "file.rkt"))
(require (planet "file.rkt" ("a.rkt" "b.rkt")))
(require (planet "file.rkt" ("user" "pkg.plt" 1 0)))
