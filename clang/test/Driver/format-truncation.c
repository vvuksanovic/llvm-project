// RUN: %clang -g -O1 -c -Wformat-truncation -mllvm -format-string-level=1 %s -Xclang -verify

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

  // For literals
  snprintf(dest, ARRAY_SIZE(dest), "abcd");  
  snprintf(dest, ARRAY_SIZE(dest), "abcde"); // expected-warning {{'snprintf' will always be truncated}}
                                                                    // Writing at least 6 bytes to 5 byte destination

  // For integers
  snprintf(dest, ARRAY_SIZE(dest), "%d", 123);    
  snprintf(dest, ARRAY_SIZE(dest), "%d", 1234);   
  snprintf(dest, ARRAY_SIZE(dest), "%d", 12345);  // expected-warning {{'snprintf' will always be truncated}}
                                                                        // Writing at least 6 bytes to 5 byte destination
  snprintf(dest, ARRAY_SIZE(dest), "%d", 123456); // expected-warning {{'snprintf' will always be truncated}}
                                                                        // Writing at least 7 bytes to 5 byte destination


  int d = bool3 ? 12345 : 654;
  snprintf(dest, ARRAY_SIZE(dest), "%d", d);   
                                                                    // Fine, minimum is 4 bytes ("654\0").
  int e = bool3 ? 12345 : 56789;
  snprintf(dest, ARRAY_SIZE(dest), "%d", e); // expected-warning {{'snprintf' will always be truncated}}
                                                                  // Writing at least 6 bytes to 5 byte destination

  // For strings
  // Added _ to format to avoid optimizations
  snprintf(dest, ARRAY_SIZE(dest), "_%s", "123"); 
                                                                       // Fine, writing 5 bytes to destination of size 5.
  snprintf(dest, ARRAY_SIZE(dest), "_%s", "1234"); // expected-warning {{'snprintf' will always be truncated}}
                                                                       // Writing at least 6 bytes to 5 byte destination
  snprintf(dest, ARRAY_SIZE(dest), "_%s_", "12"); 
                                                                       // Fine, writing 5 bytes to destination of size 5.
  snprintf(dest, ARRAY_SIZE(dest), "_%s_", "123"); // expected-warning {{'snprintf' will always be truncated}}
                                                                          //  Writing at least 6 bytes to 5 byte destination
  snprintf(dest, ARRAY_SIZE(dest), "_%.3s", "123456"); 
                                                                            // Fine, string is limited by the precision

  snprintf(dest, ARRAY_SIZE(dest), "%%%%%%%%");  
  //                                                                     // Fine, writing 5 bytes to destination of size 5.
  snprintf(dest, ARRAY_SIZE(dest), "%%%%%%%%%%"); // expected-warning {{'snprintf' will always be truncated}} 
                                                                       // Writing at least 6 bytes to 5 byte destination

  int CurrentLen;
  snprintf(dest, ARRAY_SIZE(dest), "abcd%n", &CurrentLen); 
//                                                                                // Fine, writing 5 bytes to destination of size 5.
  snprintf(dest, ARRAY_SIZE(dest), "abc%n%d", &CurrentLen, 10); // expected-warning {{'snprintf' will always be truncated}} 
                                                                       // Writing at least 6 bytes to 5 byte destination

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

  snprintf(dest, ARRAY_SIZE(dest), "%4d", 123); 
  //                                                                      // Fine
  snprintf(dest, ARRAY_SIZE(dest), "%5d", 123);  // expected-warning {{'snprintf' will always be truncated}}
  snprintf(dest, ARRAY_SIZE(dest), "%.4d", 123);  
  snprintf(dest, ARRAY_SIZE(dest), "%.5d", 123);  // expected-warning {{'snprintf' will always be truncated}}
  snprintf(dest, 1, "%.0d", 0); 
  //                                                  // special case: zero should not be printed when precision is 0
  snprintf(dest, 1, "%.d", 0); 
  //                                                  // same as above .0 is implicit

  snprintf(dest, ARRAY_SIZE(dest), "%p", NULL); // expected-warning {{'snprintf' will always be truncated}} 
  //                                                  // null pointer is printed as "(nil)"
  snprintf(dest, 6, "%p", NULL); 
  //                                                  // null pointer is printed as "(nil)"

  snprintf(dest, 14, "%p", &a); // expected-warning {{'snprintf' will always be truncated}} 
  //                                                  // 64bit pointer is treated as 14 chars long
  snprintf(dest, 15, "%p", &a); 
  //                                                  // 64bit pointer is treated as 14 chars long

  snprintf(dest, 8, "%f", 1.5); // expected-warning {{'snprintf' will always be truncated}} 
  //                                                  // default precision is 6, prints 1.500000
  snprintf(dest, 9, "%f",  1.5); 
  //                                                  // default precision is 6, prints 1.500000
  snprintf(dest, 8, "%.5f",  1.5); 
  //                                                  // lower precision to 5, prints 1.50000

  snprintf(dest, 7, "%.1e", 1.5); // expected-warning {{'snprintf' will always be truncated}} 
  //                                                  // prints 1.5e+00
  snprintf(dest, 8, "%.1e", 1.5); 
  //                                                  // prints 1.5e+00

  snprintf(dest, 6, "%a", 0x1.0p2); // expected-warning {{'snprintf' will always be truncated}} 
  //                                                  // prints 0x1p+3
  snprintf(dest, 7, "%a", 0x1.0p2); 
  //                                                  // prints 0x1p+3

}
