# Exercises what example1.sieve does not: a numeric size with a K/M/G
# quantifier, a multi-line "text:" body, a three-way anyof, and a block
# holding more than one command.
require ["fileinto", "reject", "vacation", "notify"];

if size :over 100K
    {
    if anyof (header :contains "Subject" "URGENT",
              header :contains "Subject" "ASAP",
              header :contains "X-Priority" "1")
        {
        fileinto ["Priority"];
        }
    else
        {
        reject text:
Your message exceeded the 100K size limit and was not delivered.
Please resend as a smaller attachment or a shared link.
.
;
        }
    }
elsif header :is "From" "vip@example.com"
    {
    notify :method "mailto" :message text:
A message from a VIP contact just arrived.
.
;
    keep;
    }
else
    {
    vacation :days 3 :subject "Out of office" text:
I am currently away from my desk. I will reply to your
message as soon as I am back.
.
;
    }
