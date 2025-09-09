// RUN: %clang -g -O1 -c -Wformat %s -Xclang -verify

typedef __SIZE_TYPE__ size_t;

extern int snprintf (char*, size_t, const char*, ...);
extern char* strcpy (char*, const char*);
extern char* strncpy (char*, const char*, unsigned long);

struct S
{
  int x;
  char a9[9];
  char a5[5];
  int y;
};


void test_assign_nowarn (struct S* s)
{
  int i = 0;

  {
    char a9[9] = "1234";
    snprintf (s[i].a5, sizeof (s[i].a5), "%s", a9);
  }

  {
    ++i;
    char a8[8] = "123";
    snprintf (s[i].a5, sizeof (s[i].a5), "%s\n", a8);
  }

  {
    ++i;
    char a7[7] = "12";
    snprintf (s[i].a5, sizeof (s[i].a5), "[%s]", a7);
  }

  {
    ++i;
    char a6[6] = "1";
    snprintf (s[i].a5, sizeof (s[i].a5), "[%s]\n", a6);
  }
}


void test_strcpy_nowarn (struct S* s)
{
  int i = 0;

  strcpy (s[i].a9, "1234");
  snprintf (s[i].a5, sizeof (s[i].a5), "%s", s[i].a9);

  ++i;
  strcpy (s[i].a9, "123");
  snprintf (s[i].a5, sizeof (s[i].a5), "%s\n", s[i].a9);

  ++i;
  strcpy (s[i].a9, "12");
  snprintf (s[i].a5, sizeof (s[i].a5), "[%s]", s[i].a9);

  ++i;
  strcpy (s[i].a9, "1");
  snprintf (s[i].a5, sizeof (s[i].a5), "[%s]\n", s[i].a9);
}


void test_warn (struct S* s)
{
  int i = 1;
  strcpy (s[i].a9, "12345678");
  snprintf (s[i].a5, sizeof (s[i].a5), "%s", s[i].a9);    // expected-warning {{'snprintf' will always be truncated}}

  ++i;
  strcpy (s[i].a9, "1234567");
  snprintf (s[i].a5, sizeof (s[i].a5), "%s", s[i].a9);    // expected-warning {{'snprintf' will always be truncated}}

  ++i;
  strcpy (s[i].a9, "123456");
  snprintf (s[i].a5, sizeof (s[i].a5), "%s", s[i].a9);    // expected-warning {{'snprintf' will always be truncated}}

  ++i;
  strcpy (s[i].a9, "12345");
  snprintf (s[i].a5, sizeof (s[i].a5), "%s", s[i].a9);    // expected-warning {{'snprintf' will always be truncated}}

  ++i;
  strcpy (s[i].a9, "1234");
  snprintf (s[i].a5, sizeof (s[i].a5), "%s\n", s[i].a9);  // expected-warning {{'snprintf' will always be truncated}}

  ++i;
  strcpy (s[i].a9, "123");
  snprintf (s[i].a5, sizeof (s[i].a5), ">%s<", s[i].a9);  // expected-warning {{'snprintf' will always be truncated}}

  ++i;
  strncpy (s[i].a9, "123456", 4);
  snprintf (s[i].a5, sizeof (s[i].a5), ">%s<", s[i].a9);  // expected-warning {{'snprintf' will always be truncated}}
}
