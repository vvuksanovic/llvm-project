// RUN: %clang -g -O1 -c -Wformat-overflow -Wformat-truncation %s -Xclang -verify

#define NULL ((void*)0)
typedef __typeof(sizeof(int)) size_t;

int snprintf(char * buffer, size_t bufsz, const char *format, ...);
int vsnprintf(char * s, size_t n, const char * format, __builtin_va_list arg);
int sprintf(char * buffer, const char *format, ...);
int vsprintf(char * s, const char * format, __builtin_va_list arg);
int scanf(const char *fmt, ...);

void my_printf(int _, const char* fmt, ...) __attribute__ ((format (printf, 2, 3)));

const char *return_null(void) {
    return NULL;
}

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(*array))

void test(void) {
  char dest[5];
  int a;

  int bool1, bool2, bool3;
  scanf("%d%d%d", &bool1, &bool2, &bool3);

  // Check for null string format
  snprintf(dest, ARRAY_SIZE(dest), NULL, a); // expected-warning {{null format string}}

  const char *fmt1 = bool1 ? NULL : bool2 ? NULL : bool3 ? NULL : NULL;
  snprintf(dest, ARRAY_SIZE(dest), fmt1, a); // expected-warning {{null format string}}

  snprintf(dest, ARRAY_SIZE(dest), return_null(), a); // expected-warning {{null format string}}

  // Check for null string format
  sprintf(dest, NULL, a); // expected-warning {{null format string}}

  const char *fmt2 = bool1 ? NULL : bool2 ? NULL : bool3 ? NULL : NULL;
  sprintf(dest, fmt2, a); // expected-warning {{null format string}}

  sprintf(dest, return_null(), a); // expected-warning {{null format string}}

  my_printf(0, NULL, a); // expected-warning {{null format string}}
}
