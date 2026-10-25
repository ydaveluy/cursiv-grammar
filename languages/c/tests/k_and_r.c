/* Old-style (K&R) function definitions: no return type means int, and the
   parameter types are declared separately between the parentheses and the
   body. */
sum(a, b)
int a;
int b;
{
  return a + b;
}

main()
{
  return sum(1, 2);
}

/* A parameter list of `...` alone, with no named parameter before it. */
void trace(...);
