/* A statement asm with qualifiers, output/input constraints and a clobber
   list, and a file-scope asm. */
int add_one(int x) {
  int result;
  __asm__ volatile("addl $1, %1; movl %1, %0"
                    : "=r"(result)
                    : "r"(x)
                    : "cc", "memory");
  return result;
}

asm(".global my_symbol\n\t"
    "my_symbol: .long 42");
