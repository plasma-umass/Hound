#include "CallStack.hpp"

void CALL_STACK::FormatCallStack(char *buf, PVOID *functions, size_t buf_sz, int depth) {
  size_t rem = buf_sz;

  buf[0] = '\0';

  for(int i = 0; i < depth; i++) {
    if(functions[i]) {
      Dl_info info;
      dladdr(functions[i],&info);
      rem -= snprintf(buf+buf_sz-rem,rem,"<frame><addr>0x%08x</addr><sym>%s</sym></frame>\n",functions[i],
		      info.dli_sname);
    }
  }
}
