int global;

__attribute__((noinline)) int foo(int a, int b) { return a / b * 2; }

int main() {
  int x = 0;
  int y = 1;

  int c1 = x + y;
  global += 2;
  foo(c1, global);

  int c2 = x + y;
  global += 2;
  foo(c2, global);

  return c1;
}