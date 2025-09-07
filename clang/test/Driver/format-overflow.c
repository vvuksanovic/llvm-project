// RUN: %clang -g -O1 -c -Wformat %s -Xclang -verify

#define NULL ((void*)0)

int sprintf(char * buffer, const char *format, ...);
int vsprintf(char * s, const char * format, __builtin_va_list arg);
int scanf(const char *fmt, ...);

const char *return_null(void) {
    return NULL;
}

void my_print (const char * format, ...) {
  char buffer[10];
  __builtin_va_list args;
  __builtin_va_start (args, format);
  vsprintf (buffer, format, args);
  __builtin_va_end (args);
}

void test() {
  char dest[5];
  int a, b, c;

  // Check for null string format
  sprintf(dest, NULL, a); // expected-warning {{null format string}}

  int bool1, bool2, bool3;
  scanf("%d%d%d", &bool1, &bool2, &bool3);
  const char *fmt = bool1 ? NULL : bool2 ? NULL : bool3 ? NULL : NULL;
  sprintf(dest, fmt, a); // expected-warning {{null format string}}

  sprintf(dest, return_null(), a); // expected-warning {{null format string}}
}
