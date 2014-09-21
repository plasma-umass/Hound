LDFLAGS=-g -L. -m32
#CXXFLAGS=-g -DHOUND -D'CUSTOM_PREFIX(x)=plug\#\#x' -DDEBUG -I./port -I../../heaplayers/heaplayers -I../../heaplayers/heaplayers/util #-Ilibunwind/include
CXXFLAGS=-m32 -g -O2 -DHOUND -D'CUSTOM_PREFIX(x)=plug\#\#x' -DDEBUG -finline-functions -I./port -I../../heaplayers/heaplayers -I../../heaplayers/heaplayers/util #-Ilibunwind/include
CFLAGS=$(CXXFLAGS)
##############################################################################

PLUG_OBS=site.o AOHeap.o AOCommon.o plugheap.o output.o traphandler.o port/CallStack.o port/capture_callsite.o port/libplug.o port/pagetable.o port/phkmalloc_linux.o gnuwrapper.o

ARCH_OBS=AOCommon.o output.o traphandler.o port/CallStack.o port/capture_callsite.o port/pagetable.o port/phkmalloc_linux.o port/libarch.o gnuwrapper.o

##############################################################################

all: 	libplug.so libmemprof.so

##############################################################################

libprofinj.o: libprofinj.cpp profinj.hpp

libplug.so: $(PLUG_OBS) $(DEPS) AOHeap.hpp
	$(CXX) $(LDFLAGS) -rdynamic -shared -o $@ $(PLUG_OBS) -ldl

libarch.so: $(ARCH_OBS) $(DEPS)
	$(CXX) $(LDFLAGS) -rdynamic -shared -o $@ $(ARCH_OBS) -ldl

libmemprof.so:	libmemprof.o
	$(CXX) $(LDFLAGS) -rdynamic -shared -o $@ $^ -ldl

libprofinj.so: libprofinj.o port/capture_callsite.o 
	$(CXX) $(LDFLAGS) -shared -o $@ $^ port/CallStack.o -ldl

libplug.a: $(OBS) $(DEPS)
	ar rcs $@ $(OBS)

dlmalloc.o:	../heaplayers/allocators/dlmalloc282/dlmalloc.c
	$(CC) $(CFLAGS) -DDEBUG -DUSE_DL_PREFIX -c -o $@ $<

##############################################################################

clean:
	rm -f *~ *.o port/*~ port/*.o libplug.so

################################################################# End of File.
