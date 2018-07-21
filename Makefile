
HEAP_LAYERS       = src/vendor/Heap-Layers/heaplayers.h
ALL_SUBMODULES    = $(HEAP_LAYERS)
CONFIG            = Makefile

HEAP_LAYERS_FLAGS =  -Ivendor/Heap-Layers -Ivendor/Heap-Layers/heaps -Ivendor/Heap-Layers/locks -Ivendor/Heap-Layers/threads -Ivendor/Heap-Layers/wrappers

LDFLAGS           = -g -L. -m32
#CXXFLAGS         = -g -DHOUND -DDEBUG -I./port $(HEAP_LAYERS_FLAGS) #-Ilibunwind/include
CXXFLAGS          = -m32 -g -O2 -DHOUND -DDEBUG -finline-functions -I./port $(HEAP_LAYERS_FLAGS) #-Ilibunwind/include
CFLAGS            = $(CXXFLAGS)

# quiet output, but allow us to look at what commands are being
# executed by passing 'V=1' to make, without requiring temporarily
# editing the Makefile.
ifneq ($V, 1)
MAKEFLAGS       += -s
endif

.SUFFIXES:
.SUFFIXES: .cc .cpp .S .c .o .d .test

##############################################################################

PLUG_OBS=site.o AOHeap.o AOCommon.o plugheap.o output.o traphandler.o port/CallStack.o port/capture_callsite.o port/libplug.o port/pagetable.o port/phkmalloc_linux.o

ARCH_OBS=AOCommon.o output.o traphandler.o port/CallStack.o port/capture_callsite.o port/pagetable.o port/phkmalloc_linux.o port/libarch.o gnuwrapper.o

##############################################################################

all: 	libplug.so libmemprof.so

##############################################################################

$(ALL_SUBMODULES):
	@echo "  GIT   $@"
	git submodule update --init
	touch -c $@

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
