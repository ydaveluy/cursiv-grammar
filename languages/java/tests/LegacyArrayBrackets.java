public class LegacyArrayBrackets {
    int matrix[][];

    static int[] pair(int a, int b) {
        int result[] = {a, b};
        return result;
    }

    static int oldStyleReturn()[] {
        return new int[] {1, 2};
    }

    static void loopOverRows(int rows[][]) {
        for (int row[] : rows) {
            System.out.println(row[0]);
        }
    }

    static void tryWithLegacyResource() throws Exception {
        try (java.io.InputStream in[] = System.in) {
        }
    }
}
