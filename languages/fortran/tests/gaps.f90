! Constructs the flang/PyCropML corpus does not otherwise exercise:
! computed and assigned GOTO, a named DO with CYCLE/EXIT of that name, a
! statement function, and a user-defined operator interface.
program gaps
  interface operator(.dot.)
    function dotprod(a, b)
      real :: a(3), b(3)
      real :: dotprod
    end function dotprod
  end interface

  integer :: choice, target1
  real :: v1(3), v2(3), d
  square(x) = x * x

  choice = 2
  goto (10, 20, 30) choice
10 continue
  goto 40
20 continue
  goto 40
30 continue
40 continue

  assign 50 to target1
  goto target1
50 continue

  outer: do i = 1, 5
    if (i == 2) cycle outer
    if (i == 4) exit outer
  end do outer

  v1 = (/1.0, 0.0, 0.0/)
  v2 = (/0.0, 1.0, 0.0/)
  d = v1 .dot. v2
  print *, square(3), d
end program gaps
