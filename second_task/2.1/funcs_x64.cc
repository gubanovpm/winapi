#include <iostream>

struct MyClass {
  int n;
  int add(int a) { return n + a; }
  int sub(int a) { return n - a; }
  static int minus(int a) { return -a; }
  virtual int xxx(int a) { return 0; }
  virtual int mul(int a) { return n * a; }
  MyClass(int n): n(n) {}
};

struct MyDerived: MyClass {
  MyDerived(int n) : MyClass(n) {}
  virtual int mul(int a) { return n * 0; }
  virtual int zzz(int a) { return 0; }
};

typedef int(__vectorcall ANY_CLASS_METHOD_WITH_ONE_PARAM)(void* _this, int a);
typedef int(__cdecl ANY_CLASS_STATIC_METHOD_WITH_ONE_PARAM)(int a);

int main() {
  MyClass x(5);
  printf("Direct:\n");
  printf("\t5+3 = %d\n", x.add(3));
  printf("\t5-3 = %d\n", x.sub(3));

  auto p_temp = &MyClass::add;
  ANY_CLASS_METHOD_WITH_ONE_PARAM* pMethodAdd = *(ANY_CLASS_METHOD_WITH_ONE_PARAM**)&p_temp;
  p_temp = &MyClass::sub;
  ANY_CLASS_METHOD_WITH_ONE_PARAM* pMethodSub = *(ANY_CLASS_METHOD_WITH_ONE_PARAM**)&p_temp;

  printf("By pointers:\n");

  ANY_CLASS_METHOD_WITH_ONE_PARAM* pMethod = pMethodAdd;
  int y = pMethod(&x, 3);
  printf("\t5+3 = %d\n", y);

  pMethod = pMethodSub;
  y = pMethod(&x, 3);
  printf("\t5-3 = %d\n", y);

  ANY_CLASS_STATIC_METHOD_WITH_ONE_PARAM* pStaticMethod = &x.minus;
  printf("Static:\n");
  printf("\t-6 = %d\n", pStaticMethod(6));

  p_temp = &MyClass::mul;
  ANY_CLASS_METHOD_WITH_ONE_PARAM* pVirtualMethodMul = *(ANY_CLASS_METHOD_WITH_ONE_PARAM**)&p_temp;

  printf("Virtual:\n");
  pMethod = pVirtualMethodMul;
  y = pMethod(&x, 3);
  printf("\t5*3 = %d\n", y);

  MyDerived z(5);
  y = pMethod(&z, 3);
  printf("\t5*0 = %d\n", y);

  return 0;
}
