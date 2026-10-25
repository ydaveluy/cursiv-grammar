import Some.Module;

private inherit Base.Program : base;

constant PI = 3.14, E = 2.71;

int global_count = 0;
static string *names;

class;

mapping(string:int) counts;

function(int, int) op;

int helper(int x);

array(int) make_array(int n, int ...extra) {
    int i = 0;
    array a = ({ 1, 2, 3, @extra });
    multiset m = (< 1, 2, 3 >);
    mapping d = ([ "a": 1, "b": 2, ]);

    while (i < n) {
        i++;
    }

    do {
        i--;
    } while (i > 0);

    for (int j = 0, int k = 1; j < 3; j++) {
        if (j == 0) {
            continue;
        } else {
            break;
        }
    }

    switch (n) {
        case 1..3:
            write("small");
        default:
            write("large");
    }

    int v = 0;
    foreach (a : v) {
        write(v);
    }

    int t1 = n > 0 ? 1 : -1;
    int t2 = (int) 3.5;
    int t3 = ~t1;
    int t4 = -t2;
    int t5 = !t3;
    ++i;
    i++;

    catch (i / n);
    gauge { int g = i + 1; };

    sscanf("1 2", "%d %d", int sx, int sy);

    function f = lambda(int fa, int fb) { return fa + fb; };

    object c = class { int v2; }();
    object named = class Foo { int w; }();

    base->init();

    int count2 += 5;
    int shifted <<= 2;

    int last = a[1..2][0];

    int diff = n - i;
    diff -= 1;
    diff = diff - n * 2;
    diff = -diff;

    return a;
}
