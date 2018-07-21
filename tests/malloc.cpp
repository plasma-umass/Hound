#include <malloc.h>
#include <stdio.h>

int main() {
  for (int i = 0; i < 40960; i++) {
    malloc(4096);
  }
}
