#include <cstdio>
using namespace std;

#include <unistd.h>

int main() {
  void *curr = sbrk(4096);
  fprintf(stderr, "%p\n", curr);
}
