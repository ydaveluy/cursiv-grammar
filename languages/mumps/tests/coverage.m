%coverage(a,b,c)
 new a,b,c
 kill a,b
 merge a=b,c=d
 break
 break:a>0
 hang 5
 view test
 xecute "w 1"
 close
 else
 goto
 job
 lock
 open
 tcommit
 trestart
 trollback
 tstart
 use
 set x=1,y=2
 set ^glb(1,2)="value"
 set &extrinsic=1
 if a'>b write "ok"
 if a'<b write "ok2"
 set r=a_b_c
 set f=$extract("abcde",2,4)
 do sub(a,b)
 .set nested=1
 ..set deeper=2
 quit
sub(p1,p2)
 write "hi"
 quit
