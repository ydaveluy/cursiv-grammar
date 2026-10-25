// Statements: if/else (dangling-else bound to the nearest if), both for-loop
// init shapes, while, do-while, switch/case/default with fallthrough, break,
// continue, return, try/catch, a nested block and a bare `;`.

void run()
{
    ;

    int total = 0;
    for (int i = 0; i < 10; i++)
    {
        if (i > 5)
            if (i == 7)
                total += i;
            else
                total -= i;
    }

    int j = 0;
    for (; j < total; j = j + 1, total = total - 1)
    {
        continue;
    }

    while (total > 0)
    {
        total--;
        if (total == 3)
            break;
    }

    do
    {
        total++;
    }
    while (total < 5);

    switch (total)
    {
        case 1:
        case 2:
            total += 1;
            break;
        default:
            total = 0;
    }

    try
    {
        total = total / total;
    }
    catch
    {
        total = -1;
    }

    {
        int scoped = total;
        total = scoped;
    }

    return;
}

int compute()
{
    return 42;
}
