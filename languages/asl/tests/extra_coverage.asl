[] = Delete_Timer[timer_id]

handle3 = handle1 and handle2->R7."owns"

define structure item_struct
  qty    Integer
  active Boolean
enddefine

{all_items} is item_struct
{big_items} = {all_items} where qty greater-than 10
{ordered_items} = {all_items} ordered by qty
