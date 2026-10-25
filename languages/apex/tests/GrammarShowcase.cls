public with sharing class GrammarShowcase {
    public Integer value;
    private final List<String> names = new List<String>();
    @TestVisible static Boolean flag;

    public Integer Count {
        get { return value; }
        set { value = value; }
    }

    public GrammarShowcase() {
        this.value = 0;
    }

    public GrammarShowcase(Integer v) {
        value = v;
    }

    static {
        flag = true;
    }

    {
        value = 1;
    }

    public interface Greeter {
        String greet(String name);
    }

    public enum Level { LOW, MEDIUM, HIGH }

    public class Inner implements Greeter {
        public String greet(String name) {
            return 'Hello, ' + name;
        }
    }

    public virtual class Base {
        public virtual Integer compute() {
            return 0;
        }
    }

    public class Derived extends Base {
        public override Integer compute() {
            return 1;
        }
    }

    public void run(Object obj) {
        Integer a = 1;
        Integer b = 2;
        Integer c = (a << 2) >> 1;
        Integer d = a >>> 1;
        Boolean eq = a == b;
        Boolean ne = a <> b;
        Boolean cmp = a < b && b <= c || c > d;
        Integer coalesced = a ?? b;
        Integer chained = a ?? b ?? c;
        String s = obj instanceof String ? 'yes' : 'no';
        Object cast = (Object) a;
        Object explicitNew = new <String>GrammarShowcase();

        Set<String> letters = new Set<String>{'a', 'b'};
        Map<String, Integer> counts = new Map<String, Integer>{'a' => 1, 'b' => 2};
        List<Integer> emptyList = new List<Integer>();
        Set<String> emptySet = new Set<String>{};
        Map<String, String> emptyMap = new Map<String, String>{};
        Integer[] legacy = new Integer[5];
        List<?> anyList = emptyList;

        if (flag) {
            System.debug('flag set');
        } else if (a > b) {
            System.debug('a>b');
        } else {
            System.debug('else');
        }

        for (Integer i = 0; i < 10; i++) {
            if (i == 5) continue;
            if (i == 9) break;
        }

        for (String name : letters) {
            System.debug(name);
        }

        Integer i = 0;
        while (i < 3) {
            i++;
        }
        do {
            i--;
        } while (i > 0);

        outer:
        for (Integer x = 0; x < 2; x++) {
            for (Integer y = 0; y < 2; y++) {
                if (x == y) continue outer;
            }
        }

        List<Account> accounts = [SELECT Id, Name FROM Account WHERE Name != null LIMIT 10];
        try {
            insert accounts;
            update accounts;
            upsert accounts Id;
        } catch (DmlException | NullPointerException ex) {
            System.debug(ex);
        } finally {
            System.debug('done');
        }
        delete accounts;
        undelete accounts;

        String maybe = obj?.toString();
        obj.super();
        Object sup = obj.super.hashCode();
        Object qnew = obj.new;

        System.runAs(obj) {
            System.debug('inside runAs');
        }
    }

    // The three spellings of `[` and `<>` that must not collide.
    void bracketsAndDiamond() {
        Integer[] legacy = new Integer[3];
        List<String> diamond = new List<>();
        Boolean ne = (1 <> 2);
        List<Account> found = [SELECT Id FROM Account WHERE Name = 'x'];
        Integer first = legacy[0];
    }
}
