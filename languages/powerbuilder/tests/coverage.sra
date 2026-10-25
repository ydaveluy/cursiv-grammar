RELEASE 12.5;
forward
global type cov_app from application
end type
end forward

global type cov_app from application
end type
global cov_app cov_app

type variables
private:
integer ii_count
constant integer MAXROWS = 100
end variables

forward prototypes
global function boolean of_alias(int a) library "user32.dll"
end prototypes

type ue_custom_event from event_object
event type long ue_custom ( )
end type

function boolean of_test(int ai_x, ref string as_y, readonly long al_z, ...)
integer li_i
decimal{2} ld_amt
date ld_today
time lt_now
integer la_arr[1 to 10]
integer la_grid[1 to 3, 1 to 5]
integer la_dyn[]
integer la_vals[3] = {1, 2, 3}
li_i = StopSign!
ld_today = 2024-01-15
lt_now = 12:30:00
li_i ++
li_i --

choose case li_i
case 1, 2
	li_i = 1
case 3 to 5
	li_i = 2
case is > 10
	li_i = 3
case else
	li_i = 0
end choose

for li_i = 1 to 10 step 2
	li_i = li_i + 1
next

do while li_i < 5
	li_i = li_i + 1
loop

do until li_i >= 10
	li_i = li_i + 1
loop

do
	li_i = li_i - 1
loop while li_i > 0

try
	li_i = 1 / li_i
catch (exception le_x)
	li_i = -1
finally
	li_i = 0
end try

goto lbl_skip
li_i = 1
lbl_skip:
li_i = 2

if li_i > 0 then
	li_i = 1
elseif li_i < 0 then
	li_i = -1
else
	li_i = 0
end if

if li_i > 0 then li_i = 1 else li_i = 2

throw le_x

declare cur_1 cursor for sqlstmt;
open cur_1;
fetch cur_1 into :as_y;
close cur_1;
prepare sqlstmt from :as_y using sqlca;
execute immediate as_y;
execute dynamic sqlstmt using sqlca;
connect using sqlca;
disconnect;
rollback;

select count(kod) into :li_i from cov_table where kod = :ai_x;
insert into cov_table (kod, val) values (:ai_x, :as_y);
update cov_table set kod = :ai_x where kod = :ai_x;
delete from cov_table where kod = :ai_x;

::of_ancestor(ai_x)
call super::of_ancestor
post of_ancestor()
trigger event ue_custom()
event ue_custom()

li_i = describe(as_y)
describe as_y

@as_y = :as_y;

return true
end function
