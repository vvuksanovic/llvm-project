// RUN: %clang -g -O1 -c -Wformat %s 2>&1 | FileCheck %s

#define NULL ((void*)0)
typedef __typeof(sizeof(int)) size_t;

int snprintf(char * buffer, size_t bufsz, const char *format, ...);
int vsnprintf(char * s, size_t n, const char * format, __builtin_va_list arg);
int scanf(const char *fmt, ...);

const char *return_null(void) {
    return NULL;
}

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(*array))

void test() {
  char dest[5];
  int a, b, c;

  int bool1, bool2, bool3;
  scanf("%d%d%d", &bool1, &bool2, &bool3);

  // Check for null string format
  snprintf(dest, ARRAY_SIZE(dest), NULL, a); // CHECK: [[@LINE]]:3: Null format string

  const char *fmt = bool1 ? NULL : bool2 ? NULL : bool3 ? NULL : NULL;
  snprintf(dest, ARRAY_SIZE(dest), fmt, a); // CHECK: [[@LINE]]:3: Null format string

  snprintf(dest, ARRAY_SIZE(dest), return_null(), a); // CHECK: [[@LINE]]:3: Null format string

  // Check for format truncation

  // For literals
  snprintf(dest, ARRAY_SIZE(dest), "abcd");
  snprintf(dest, ARRAY_SIZE(dest), "abcde"); // CHECK: [[@LINE]]:3: Output will be truncated
                                                                    // Writing at least 6 bytes to 5 byte destination

  // For integers
  snprintf(dest, ARRAY_SIZE(dest), "%d", 123);
  snprintf(dest, ARRAY_SIZE(dest), "%d", 1234);
  snprintf(dest, ARRAY_SIZE(dest), "%d", 12345);  // CHECK: [[@LINE]]:3: Output will be truncated
                                                                        // Writing at least 6 bytes to 5 byte destination
  snprintf(dest, ARRAY_SIZE(dest), "%d", 123456); // CHECK: [[@LINE]]:3: Output will be truncated
                                                                        // Writing at least 7 bytes to 5 byte destination


  int d = bool3 ? 12345 : 654;
  snprintf(dest, ARRAY_SIZE(dest), "%d", d); // Fine, minimum is 4 bytes ("654\0").
  int e = bool3 ? 12345 : 56789;
  snprintf(dest, ARRAY_SIZE(dest), "%d", e); // CHECK: [[@LINE]]:3: Output will be truncated
                                                                  // Writing at least 6 bytes to 5 byte destination

  // For strings
  // Added _ to format to avoid optimizations
  snprintf(dest, ARRAY_SIZE(dest), "_%s", "123"); // Fine, wiriting 5 bytes to destination of size 5.
  snprintf(dest, ARRAY_SIZE(dest), "_%s", "1234"); // CHECK: [[@LINE]]:3: Output will be truncated. 
                                                                       // Writing at least 6 bytes to 5 byte destination
  snprintf(dest, ARRAY_SIZE(dest), "_%s", "12345"); // CHECK: [[@LINE]]:3: Output will be truncated
                                                                          //  Writing at least 7 bytes to 5 byte destination

  snprintf(dest, ARRAY_SIZE(dest), "%%%%%%%%"); // Fine, wiriting 5 bytes to destination of size 5.
  snprintf(dest, ARRAY_SIZE(dest), "%%%%%%%%%%"); // CHECK: [[@LINE]]:3: Output will be truncated. 
                                                                       // Writing at least 6 bytes to 5 byte destination

  int CurrentLen;
  snprintf(dest, ARRAY_SIZE(dest), "abcd%n", &CurrentLen); // Fine, wiriting 5 bytes to destination of size 5.
  snprintf(dest, ARRAY_SIZE(dest), "abc%n%d", &CurrentLen, 1); // CHECK: [[@LINE]]:3: Output will be truncated. 
                                                                       // Writing at least 6 bytes to 5 byte destination

  snprintf(dest, 3, "%X", 0xAB); // Fine, wiriting 3 bytes to destination of size 3.
  snprintf(dest, 2, "%X", 0xAB); // CHECK: [[@LINE]]:3: Output will be truncated. 
                                                                       // Writing at least 6 bytes to 5 byte destination

  snprintf(dest, 5, "%+d", 123); // Fine, wiriting 5 bytes to destination of size 5.
  snprintf(dest, 4, "%+d", 123); // CHECK: [[@LINE]]:3: Output will be truncated. 
                                                                       // Writing at least 5 bytes to 4 byte destination

  snprintf(dest, 5, "%#x", 0xAB); // Fine, wiriting 5 bytes to destination of size 5.
  snprintf(dest, 4, "%#x", 0xAB); // CHECK: [[@LINE]]:3: Output will be truncated. 
                                                                       // Writing at least 5 bytes to 4 byte destination

  snprintf(dest, ARRAY_SIZE(dest), "%4d", 123); // Fine
  snprintf(dest, ARRAY_SIZE(dest), "%5d", 123);  // CHECK: [[@LINE]]:3: Output will be truncated. 
  snprintf(dest, ARRAY_SIZE(dest), "%.4d", 123);
  snprintf(dest, ARRAY_SIZE(dest), "%.5d", 123);  // CHECK: [[@LINE]]:3: Output will be truncated. 
}
