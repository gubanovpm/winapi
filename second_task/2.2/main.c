#include "stdio.h"
#include "Windows.h"

//fastcall func in asm which stdcall func C

void __stdcall func(int i1, int i2, int i3, float f1, float f2, float f3) {
  printf("Passed int32:\n");
  printf("\t%d; %d; %d\n", i1, i2, i3);
  
  printf("Passed float:\n");
  printf("\t%f; %f; %f\n", f1, f2, f3);
  
  printf("Int32 sum : %d\n", i1 + i2 + i3);
  printf("Float sum : %f\n", f1 + f2 + f3);
}

extern int __fastcall f_add(int i1, int i2, int i3, float f1, float f2, float f3);

int main() {
  f_add(1, 20, 300, 1., 20.5, 300.125);
  return 0;
}
