#ifndef HOUND_LOG_H__
#define HOUND_LOG_H__

static inline int log2 (size_t sz)
{
	int retval;
	sz = (sz << 1) - 1;
	__asm {
	  bsr eax, sz
		mov retval, eax
		}
	return retval;
}

template<int n>
struct StaticLog;

template<>
struct StaticLog<1> {
	enum { VALUE = 0 };
};

template<>
struct StaticLog<2> {
	enum { VALUE = 1 };
};

template<int n>
struct StaticLog<n> {
	enum { VALUE = StaticLog<n/2>::VALUE + 1 };
};

#endif
