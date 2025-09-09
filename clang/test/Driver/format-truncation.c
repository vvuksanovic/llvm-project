// RUN: %clang -g -O1 -c -Wformat-truncation %s -Xclang -verify

#define NULL ((void*)0)
typedef __typeof(sizeof(int)) size_t;

int snprintf(char * buffer, size_t bufsz, const char *format, ...);
int vsnprintf(char * s, size_t n, const char * format, __builtin_va_list arg);
int scanf(const char *fmt, ...);

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(*array))

void test_format_truncation(void) {
  char dest[30];
  int a, b, c;

  int bool1, bool2, bool3;
  scanf("%d%d%d", &bool1, &bool2, &bool3);

  // Test literals:
  snprintf(dest, 5, "abcd");  // okay
  snprintf(dest, 5, "abcde"); // expected-warning 2 {{'snprintf' will always be truncated; specified size is 5, but format string expands to at least 6}}
  snprintf(dest, 5, "%%%%%%%%%%"); // expected-warning 2 {{'snprintf' will always be truncated; specified size is 5, but format string expands to at least 6}} 

  // Test integers:
  snprintf(dest, 5, "%d", 1234);   // okay
  snprintf(dest, 5, "%d", 12345);  // expected-warning {{'snprintf' will always be truncated; specified size is 5, but format string expands to at least 6}}

  snprintf(dest, 5, "%*d", 5, 1);  // expected-warning {{'snprintf' will always be truncated}}
  unsigned Width = bool1 ? 5 : 10;
  snprintf(dest, 5, "%*d", Width, 1); // expected-warning {{'snprintf' will always be truncated}}

  int d = bool1 ? 12345 : 654;
  snprintf(dest, 5, "%d", d); // okay, minimum value for d is 4 bytes ("654\0").
  int e = bool2 ? 12345 : 56789;
  snprintf(dest, 5, "%d", e); // expected-warning {{'snprintf' will always be truncated; specified size is 5, but format string expands to at least 6}}
  int f = bool2 ? -1234 : -56789;
  snprintf(dest, 5, "%d", f); // expected-warning {{'snprintf' will always be truncated; specified size is 5, but format string expands to at least 6}}
  
  // Test strings:
  // Added _ to format string to avoid optimizations
  snprintf(dest, 5, "_%s", "1234");     // expected-warning {{'snprintf' will always be truncated; specified size is 5, but format string expands to at least 6}}
  snprintf(dest, 5, "_%s_", "123");     // expected-warning {{'snprintf' will always be truncated; specified size is 5, but format string expands to at least 6}}
  snprintf(dest, 5, "_%.3s", "123456"); // fine, string is limited by the precision

  // Test %n directive:
  int CurrentLen;
  snprintf(dest, 5, "abcd%n", &CurrentLen);      // Fine, writing 5 bytes to destination of size 5.
  snprintf(dest, 5, "abc%n%d", &CurrentLen, 10); // expected-warning {{'snprintf' will always be truncated; specified size is 5, but format string expands to at least 6}} 

  // Test floats:
  snprintf(dest, 3, "%X", 0xAB); 
  //                                                    // Fine, writing 3 bytes to destination of size 3.
  snprintf(dest, 2, "%X", 0xAB); // expected-warning {{'snprintf' will always be truncated}} 
                                                          // Writing at least 6 bytes to 5 byte destination

  snprintf(dest, 5, "%+d", 123);  
                                                        // Fine, writing 5 bytes to destination of size 5.
  snprintf(dest, 4, "%+d", 123); // expected-warning {{'snprintf' will always be truncated}} 
                                                        // Writing at least 5 bytes to 4 byte destination

  snprintf(dest, 5, "%#x", 0xAB); 
  //                                                      // Fine, writing 5 bytes to destination of size 5.
  snprintf(dest, 4, "%#x", 0xAB); // expected-warning {{'snprintf' will always be truncated}} 
                                                            // Writing at least 5 bytes to 4 byte destination

  snprintf(dest, 5, "%4d", 123); 
  //                                                                      // Fine
  snprintf(dest, 5, "%5d", 123);  // expected-warning 2 {{'snprintf' will always be truncated}}
  snprintf(dest, 5, "%.4d", 123);  
  snprintf(dest, 5, "%.5d", 123);  // expected-warning 2 {{'snprintf' will always be truncated}}
  snprintf(dest, 1, "%.0d", 0); 
  //                                                  // special case: zero should not be printed when precision is 0
  snprintf(dest, 1, "%.d", 0);   // TODO: Bug in llvm // expected-warning {{'snprintf' will always be truncated}}
  //                                                  // same as above .0 is implicit

  snprintf(dest, 5, "%p", NULL); // expected-warning {{'snprintf' will always be truncated}} 
  //                                                  // null pointer is printed as "(nil)"
  snprintf(dest, 6, "%p", NULL); 
  //                                                  // null pointer is printed as "(nil)"

  snprintf(dest, 14, "%p", &a); // expected-warning {{'snprintf' will always be truncated}} 
  //                                                  // 64bit pointer is treated as 14 chars long
  snprintf(dest, 15, "%p", &a); 
  //                                                  // 64bit pointer is treated as 14 chars long

  snprintf(dest, 8, "%f", 1.5); // expected-warning 2 {{'snprintf' will always be truncated}} 
  //                                                  // default precision is 6, prints 1.500000
  snprintf(dest, 9, "%f",  1.5); 
  //                                                  // default precision is 6, prints 1.500000
  snprintf(dest, 8, "%.5f",  1.5); 
  //                                                  // lower precision to 5, prints 1.50000

  snprintf(dest, 7, "%.1e", 1.5); // expected-warning {{'snprintf' will always be truncated}} 
  //                                                  // prints 1.5e+00
  snprintf(dest, 8, "%.1e", 1.5); 
  //                                                  // prints 1.5e+00

  snprintf(dest, 6, "%a", 0x1.0p2); // expected-warning 2 {{'snprintf' will always be truncated}} 
  //                                                  // prints 0x1p+3
  snprintf(dest, 7, "%a", 0x1.0p2); 
  //                                                  // prints 0x1p+3

}
