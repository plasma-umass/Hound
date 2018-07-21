// -*- C++ -*-

#ifndef HOUND_MEMTRACE_H
#define HOUND_MEMTRACE_H

#include <dlfcn.h>
#include <fcntl.h>
#include <grp.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <utime.h>
#include <unistd.h>
#include <sys/xattr.h>
//#include <linux/aio.h>
//#include <linux/futex.h>
#include <linux/unistd.h>
#include <linux/sysctl.h>
#include <linux/reboot.h>
#include <sys/epoll.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/msg.h>
#include <sys/poll.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/sendfile.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <sys/vfs.h>
#include <sys/wait.h>

#include "heaplayers.h"
//#include "vamcommon.h"
// extract the stuff we need from vamcommon.h
#include <stdarg.h>

#include "../AOCommon.hpp"

extern "C" int __initialized_memtracer;

class MemTracer {
public:

	void dbprintf(const char * fmt, ...) {
	  /*
		va_list ap;
		va_start(ap, fmt);

		int n = vfprintf(stderr, fmt, ap);
	  */
	}

/*
	void exit(int status) {
		MemTracerLock l(_lock);

		if (_real_exit == NULL)
			init_all();

		dbprintf("Current allocated with mmap = %d\n", currentAllocatedMmap);
		dbprintf("Current allocated with sbrk = %d\n", currentAllocatedSbrk);
		dbprintf("Max allocated overall = %d\n", maxAllocated);

		(*_real_exit)(status);
	}
*/
	time_t time(time_t * t) {
		if (_real_time == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(t, sizeof(*t));

		time_t rv = (*_real_time)(t);

		if (need_to_reprotect)
			reprotectBuf(t, sizeof(*t));

		return rv;
	}

	int stime(const time_t * t) {
		if (_real_stime == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(t, sizeof(*t));

		int rv = (*_real_stime)(t);

		if (need_to_reprotect)
			reprotectBuf(t, sizeof(*t));

		return rv;
	}

	int gettimeofday(struct timeval * tv, struct timezone * tz) {
		if (_real_gettimeofday == NULL)
			init_all();

		bool need_to_reprotect1 = unprotectBuf(tv, sizeof(*tv));
		bool need_to_reprotect2 = unprotectBuf(tz, sizeof(*tz));

		int rv = (*_real_gettimeofday)(tv, tz);
		dbprintf("gettimeofday(%p, %p) returned %d\n", tv, tz, rv);

		if (need_to_reprotect1)
			reprotectBuf(tv, sizeof(*tv));
		if (need_to_reprotect2)
			reprotectBuf(tz, sizeof(*tz));

		return rv;
	}

	int settimeofday(const struct timeval * tv, const struct timezone * tz) {
		if (_real_settimeofday == NULL)
			init_all();

		bool need_to_reprotect1 = unprotectBuf(tv, sizeof(*tv));
		bool need_to_reprotect2 = unprotectBuf(tz, sizeof(*tz));

		int rv = (*_real_settimeofday)(tv, tz);

		if (need_to_reprotect1)
			reprotectBuf(tv, sizeof(*tv));
		if (need_to_reprotect2)
			reprotectBuf(tz, sizeof(*tz));

		return rv;
	}

	int getresuid(uid_t * ruid, uid_t * euid, uid_t * suid) {
		if (_real_getresuid == NULL)
			init_all();

		bool need_to_reprotect1 = unprotectBuf(ruid, sizeof(*ruid));
		bool need_to_reprotect2 = unprotectBuf(euid, sizeof(*euid));
		bool need_to_reprotect3 = unprotectBuf(suid, sizeof(*suid));

		int rv = (*_real_getresuid)(ruid, euid, suid);

		if (need_to_reprotect1)
			reprotectBuf(ruid, sizeof(*ruid));
		if (need_to_reprotect2)
			reprotectBuf(euid, sizeof(*euid));
		if (need_to_reprotect3)
			reprotectBuf(suid, sizeof(*suid));

		return rv;
	}

	int getresgid(gid_t * rgid, gid_t * egid, gid_t * sgid) {
		if (_real_getresgid == NULL)
			init_all();

		bool need_to_reprotect1 = unprotectBuf(rgid, sizeof(*rgid));
		bool need_to_reprotect2 = unprotectBuf(egid, sizeof(*egid));
		bool need_to_reprotect3 = unprotectBuf(sgid, sizeof(*sgid));

		int rv = (*_real_getresgid)(rgid, egid, sgid);

		if (need_to_reprotect1)
			reprotectBuf(rgid, sizeof(*rgid));
		if (need_to_reprotect2)
			reprotectBuf(egid, sizeof(*egid));
		if (need_to_reprotect3)
			reprotectBuf(sgid, sizeof(*sgid));

		return rv;
	}

	int sigpending(sigset_t * set) {
		if (_real_sigpending == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(set, sizeof(*set));

		int rv = (*_real_sigpending)(set);

		if (need_to_reprotect)
			reprotectBuf(set, sizeof(*set));

		return rv;
	}

	int sigprocmask(int how, const sigset_t * set, sigset_t * oldset) {
		if (_real_sigprocmask == NULL)
			init_all();

		bool need_to_reprotect1 = unprotectBuf(set, sizeof(*set));
		bool need_to_reprotect2 = unprotectBuf(oldset, sizeof(*oldset));

		int rv = (*_real_sigprocmask)(how, set, oldset);

		if (need_to_reprotect1)
			reprotectBuf(set, sizeof(*set));
		if (need_to_reprotect2)
			reprotectBuf(oldset, sizeof(*oldset));

		return rv;
	}

	int sched_setscheduler(pid_t pid, int policy, const struct sched_param * p) {
		if (_real_sched_setscheduler == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(p, sizeof(*p));

		int rv = (*_real_sched_setscheduler)(pid, policy, p);

		if (need_to_reprotect)
			reprotectBuf(p, sizeof(*p));

		return rv;
	}

	int sched_setparam(pid_t pid, const struct sched_param * p) {
		if (_real_sched_setparam == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(p, sizeof(*p));

		int rv = (*_real_sched_setparam)(pid, p);

		if (need_to_reprotect)
			reprotectBuf(p, sizeof(*p));

		return rv;
	}

	int sched_getparam(pid_t pid, struct sched_param * p) {
		if (_real_sched_getparam == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(p, sizeof(*p));

		int rv = (*_real_sched_getparam)(pid, p);

		if (need_to_reprotect)
			reprotectBuf(p, sizeof(*p));

		return rv;
	}

	pid_t wait(int * status) {
		if (_real_wait == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(status, sizeof(*status));

		pid_t rv = (*_real_wait)(status);

		if (need_to_reprotect)
			reprotectBuf(status, sizeof(*status));

		return rv;
	}

	pid_t wait3(int * status, int options, struct rusage * rusage) {
		if (_real_wait3 == NULL)
			init_all();

		bool need_to_reprotect1 = unprotectBuf(status, sizeof(*status));
		bool need_to_reprotect2 = unprotectBuf(rusage, sizeof(*rusage));

		pid_t rv = (*_real_wait3)(status, options, rusage);

		if (need_to_reprotect1)
			reprotectBuf(status, sizeof(*status));
		if (need_to_reprotect2)
			reprotectBuf(rusage, sizeof(*rusage));

		return rv;
	}

	pid_t wait4(pid_t pid, int * status, int options, struct rusage * rusage) {
		if (_real_wait4 == NULL)
			init_all();

		bool need_to_reprotect1 = unprotectBuf(status, sizeof(*status));
		bool need_to_reprotect2 = unprotectBuf(rusage, sizeof(*rusage));

		pid_t rv = (*_real_wait4)(pid, status, options, rusage);

		if (need_to_reprotect1)
			reprotectBuf(status, sizeof(*status));
		if (need_to_reprotect2)
			reprotectBuf(rusage, sizeof(*rusage));

		return rv;
	}

	int waitid(idtype_t idtype, id_t id, siginfo_t * infop, int options) {
		if (_real_waitid == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(infop, sizeof(*infop));

		int rv = (*_real_waitid)(idtype, id, infop, options);

		if (need_to_reprotect)
			reprotectBuf(infop, sizeof(*infop));

		return rv;
	}

	pid_t waitpid(pid_t pid, int * status, int options) {
		if (_real_waitpid == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(status, sizeof(*status));

		pid_t rv = (*_real_waitpid)(pid, status, options);

		if (need_to_reprotect)
			reprotectBuf(status, sizeof(*status));

		return rv;
	}

	int statfs(const char * path, struct statfs * buf) {
		if (_real_statfs == NULL)
			init_all();

		size_t len = strlen(path);
		bool need_to_reprotect1 = unprotectBuf(path, len);
		bool need_to_reprotect2 = unprotectBuf(buf, sizeof(*buf));

		int rv = (*_real_statfs)(path, buf);

		if (need_to_reprotect1)
			reprotectBuf(path, len);
		if (need_to_reprotect2)
			reprotectBuf(buf, sizeof(*buf));

		return rv;
	}

	int statfs64(const char * path, struct statfs64 * buf) {
		if (_real_statfs64 == NULL)
			init_all();

		size_t len = strlen(path);
		bool need_to_reprotect1 = unprotectBuf(path, len);
		bool need_to_reprotect2 = unprotectBuf(buf, sizeof(*buf));

		int rv = (*_real_statfs64)(path, buf);

		if (need_to_reprotect1)
			reprotectBuf(path, len);
		if (need_to_reprotect2)
			reprotectBuf(buf, sizeof(*buf));

		return rv;
	}

	int fstatfs(int fd, struct statfs * buf) {
		if (_real_fstatfs == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(buf, sizeof(*buf));

		int rv = (*_real_fstatfs)(fd, buf);

		if (need_to_reprotect)
			reprotectBuf(buf, sizeof(*buf));

		return rv;
	}

	int fstatfs64(int fd, struct statfs64 * buf) {
		if (_real_fstatfs64 == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(buf, sizeof(*buf));

		int rv = (*_real_fstatfs64)(fd, buf);

		if (need_to_reprotect)
			reprotectBuf(buf, sizeof(*buf));

		return rv;
	}

	int link(const char * oldpath, const char * newpath) {
		if (_real_link == NULL)
			init_all();

		size_t len1 = strlen(oldpath);
		size_t len2 = strlen(newpath);
		bool need_to_reprotect1 = unprotectBuf(oldpath, len1);
		bool need_to_reprotect2 = unprotectBuf(newpath, len2);

		int rv = (*_real_link)(oldpath, newpath);

		if (need_to_reprotect1)
			reprotectBuf(oldpath, len1);
		if (need_to_reprotect2)
			reprotectBuf(newpath, len2);

		return rv;
	}

	int symlink(const char * oldpath, const char * newpath) {
		if (_real_symlink == NULL)
			init_all();

		size_t len1 = strlen(oldpath);
		size_t len2 = strlen(newpath);
		bool need_to_reprotect1 = unprotectBuf(oldpath, len1);
		bool need_to_reprotect2 = unprotectBuf(newpath, len2);

		int rv = (*_real_symlink)(oldpath, newpath);

		if (need_to_reprotect1)
			reprotectBuf(oldpath, len1);
		if (need_to_reprotect2)
			reprotectBuf(newpath, len2);

		return rv;
	}

	int unlink(const char * pathname) {
		if (_real_unlink == NULL)
			init_all();

		size_t len = strlen(pathname);
		bool need_to_reprotect = unprotectBuf(pathname, len);

		int rv = (*_real_unlink)(pathname);

		if (need_to_reprotect)
			reprotectBuf(pathname, len);

		return rv;
	}

	int rename(const char * oldpath, const char * newpath) {
		if (_real_rename == NULL)
			init_all();

		size_t len1 = strlen(oldpath);
		size_t len2 = strlen(newpath);
		bool need_to_reprotect1 = unprotectBuf(oldpath, len1);
		bool need_to_reprotect2 = unprotectBuf(newpath, len2);

		int rv = (*_real_rename)(oldpath, newpath);

		if (need_to_reprotect1)
			reprotectBuf(oldpath, len1);
		if (need_to_reprotect2)
			reprotectBuf(newpath, len2);

		return rv;
	}

	int chmod(const char * path, mode_t mode) {
		if (_real_chmod == NULL)
			init_all();

		size_t len = strlen(path);
		bool need_to_reprotect = unprotectBuf(path, len);

		int rv = (*_real_chmod)(path, mode);

		if (need_to_reprotect)
			reprotectBuf(path, len);

		return rv;
	}

	int readlink(const char * path, char * buf, size_t bufsiz) {
		if (_real_readlink == NULL)
			init_all();

		size_t len = strlen(path);
		bool need_to_reprotect1 = unprotectBuf(path, len);
		bool need_to_reprotect2 = unprotectBuf(buf, bufsiz);

		int rv = (*_real_readlink)(path, buf, bufsiz);

		if (need_to_reprotect1)
			reprotectBuf(path, len);
		if (need_to_reprotect2)
			reprotectBuf(buf, bufsiz);

		return rv;
	}

	int creat(const char * pathname, mode_t mode) {
		if (_real_creat == NULL)
			init_all();

		size_t len = strlen(pathname) + 1;
		bool need_to_reprotect = unprotectBuf(pathname, len);

		int rv = (*_real_creat)(pathname, mode);
		dbprintf("creat(%s, %x) returned %d\n", pathname, mode, rv);

		if (need_to_reprotect)
			reprotectBuf(pathname, len);

		return rv;
	}

	int open(const char * pathname, int flags, mode_t mode) {
		if (_real_open == NULL)
			init_all();

		size_t len = strlen(pathname) + 1;
		bool need_to_reprotect = unprotectBuf(pathname, len);

		int rv = (*_real_open)(pathname, flags, mode);
		dbprintf("open(\"%s\", %x, %x) returned %d\n", pathname, flags, mode, rv);

		if (need_to_reprotect)
			reprotectBuf(pathname, len);

		return rv;
	}

	int open64(const char * pathname, int flags, mode_t mode) {
		if (_real_open64 == NULL)
			init_all();

		size_t len = strlen(pathname) + 1;
		bool need_to_reprotect = unprotectBuf(pathname, len);

		int rv = (*_real_open64)(pathname, flags, mode);
		dbprintf("open64(%p: \"%s\", %x, %x) returned %d\n", pathname, pathname, flags, mode, rv);
#if 1
		if (need_to_reprotect)
			reprotectBuf(pathname, len);
#endif
		return rv;
	}

	FILE * fopen(const char * path, const char * mode) {
		if (_real_fopen == NULL)
			init_all();

		size_t len1 = strlen(path) + 1;
		size_t len2 = strlen(mode) + 1;
		bool need_to_reprotect1 = unprotectBuf(path, len1);
		bool need_to_reprotect2 = unprotectBuf(mode, len2);

		FILE * rv = (*_real_fopen)(path, mode);
		dbprintf("fopen(\"%s\", \"%s\") returned %d\n", path, mode, rv);

		if (need_to_reprotect1)
			reprotectBuf(path, len1);
		if (need_to_reprotect2)
			reprotectBuf(mode, len2);

		return rv;
	}

	FILE * fopen64(const char * path, const char * mode) {
		if (_real_fopen64 == NULL)
			init_all();

		size_t len1 = strlen(path) + 1;
		size_t len2 = strlen(mode) + 1;
		bool need_to_reprotect1 = unprotectBuf(path, len1);
		bool need_to_reprotect2 = unprotectBuf(mode, len2);

		FILE * rv = (*_real_fopen64)(path, mode);
		dbprintf("fopen64(\"%s\", \"%s\") returned %d\n", path, mode, rv);

		if (need_to_reprotect1)
			reprotectBuf(path, len1);
		if (need_to_reprotect2)
			reprotectBuf(mode, len2);

		return rv;
	}

	int access(const char * pathname, int mode) {
		if (_real_access == NULL)
			init_all();

		size_t len = strlen(pathname);
		bool need_to_reprotect = unprotectBuf(pathname, len);

		int rv = (*_real_access)(pathname, mode);
#if 1
		if (need_to_reprotect)
			reprotectBuf(pathname, len);
#endif
		dbprintf("access(%p, %x) = %d\n", pathname, mode, rv);
		return rv;
	}

	int chown(const char * path, uid_t owner, gid_t group) {
		if (_real_chown == NULL)
			init_all();

		size_t len = strlen(path);
		bool need_to_reprotect = unprotectBuf(path, len);

		int rv = (*_real_chown)(path, owner, group);

		if (need_to_reprotect)
			reprotectBuf(path, len);

		return rv;
	}

	int lchown(const char * path, uid_t owner, gid_t group) {
		if (_real_lchown == NULL)
			init_all();

		size_t len = strlen(path);
		bool need_to_reprotect = unprotectBuf(path, len);

		int rv = (*_real_lchown)(path, owner, group);

		if (need_to_reprotect)
			reprotectBuf(path, len);

		return rv;
	}

	int utime(const char * filename, const struct utimbuf * buf) {
		if (_real_utime == NULL)
			init_all();

		size_t len = strlen(filename);
		bool need_to_reprotect1 = unprotectBuf(filename, len);
		bool need_to_reprotect2 = unprotectBuf(buf, sizeof(*buf));

		int rv = (*_real_utime)(filename, buf);

		if (need_to_reprotect1)
			reprotectBuf(filename, len);
		if (need_to_reprotect2)
			reprotectBuf(buf, sizeof(*buf));

		return rv;
	}

	int utimes(const char * filename, const struct timeval * tvp) {
		if (_real_utimes == NULL)
			init_all();

		size_t len = strlen(filename);
		bool need_to_reprotect1 = unprotectBuf(filename, len);
		bool need_to_reprotect2 = unprotectBuf(tvp, sizeof(*tvp));

		int rv = (*_real_utimes)(filename, tvp);

		if (need_to_reprotect1)
			reprotectBuf(filename, len);
		if (need_to_reprotect2)
			reprotectBuf(tvp, sizeof(*tvp));

		return rv;
	}

	int llseek(unsigned int fd, unsigned long offset_high, unsigned long offset_low, loff_t * result, unsigned int whence) {
		if (_real_llseek == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(result, sizeof(*result));

		int rv = (*_real_llseek)(fd, offset_high, offset_low, result, whence);

		if (need_to_reprotect)
			reprotectBuf(result, sizeof(*result));

		return rv;
	}

	ssize_t read(int fd, void * buf, size_t count) {
		if (_real_read == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(buf, count);

		ssize_t rv = (*_real_read)(fd, buf, count);
		dbprintf("read(%d, %p, %u) returned %d\n", fd, buf, count, rv);

		if (need_to_reprotect)
			reprotectBuf(buf, count);

		return rv;
	}

	size_t fread(void * ptr, size_t size, size_t nmemb, FILE * stream) {
		if (_real_fread == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(ptr, size * nmemb);

		size_t rv = (*_real_fread)(ptr, size, nmemb, stream);
		dbprintf("fread(%p, %u, %u, %p) returned %u\n", ptr, size, nmemb, stream, rv);

		if (need_to_reprotect)
			reprotectBuf(ptr, size * nmemb);

		return rv;
	}

	ssize_t readv(int fd, const struct iovec * vector, int count) {
		if (_real_readv == NULL)
			init_all();

		bool need_to_reprotect[count];
		for (int i = 0; i < count; i++)
			need_to_reprotect[i] = unprotectBuf(vector[i].iov_base, vector[i].iov_len);

		ssize_t rv = (*_real_readv)(fd, vector, count);
		dbprintf("readv(%d, %p, %d) returned %d\n", fd, vector, count, rv);

		for (int i = 0; i < count; i++)
			if (need_to_reprotect[i])
				reprotectBuf(vector[i].iov_base, vector[i].iov_len);

		return rv;
	}

	ssize_t write(int fd, const void * buf, size_t count) {
		if (_real_write == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(buf, count);

		ssize_t rv = (*_real_write)(fd, buf, count);
		dbprintf("write(%d, %p, %d) returned %d\n", fd, buf, count, rv);

		if (need_to_reprotect)
			reprotectBuf(buf, count);

		return rv;
	}

	size_t fwrite(const void * ptr, size_t size, size_t nmemb, FILE * stream) {
		if (_real_fwrite == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(ptr, size * nmemb);

		size_t rv = (*_real_fwrite)(ptr, size, nmemb, stream);
		dbprintf("fwrite(%p, %u, %u, %p) returned %u\n", ptr, size, nmemb, stream, rv);

		if (need_to_reprotect)
			reprotectBuf(ptr, size * nmemb);

		return rv;
	}

	ssize_t writev(int fd, const struct iovec * vector, int count) {
		if (_real_writev == NULL)
			init_all();

		bool need_to_reprotect[count];
		for (int i = 0; i < count; i++)
			need_to_reprotect[i] = unprotectBuf(vector[i].iov_base, vector[i].iov_len);

		ssize_t rv = (*_real_writev)(fd, vector, count);
		dbprintf("writev(%d, %p, %d) returned %d\n", fd, vector, count, rv);

		for (int i = 0; i < count; i++)
			if (need_to_reprotect[i])
				reprotectBuf(vector[i].iov_base, vector[i].iov_len);

		return rv;
	}

	ssize_t pread(int fd, void * buf, size_t count, off_t offset) {
		if (_real_pread == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(buf, count);

		ssize_t rv = (*_real_pread)(fd, buf, count, offset);

		if (need_to_reprotect)
			reprotectBuf(buf, count);

		return rv;
	}

	ssize_t pwrite(int fd, const void * buf, size_t count, off_t offset) {
		if (_real_pwrite == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(buf, count);

		ssize_t rv = (*_real_pwrite)(fd, buf, count, offset);

		if (need_to_reprotect)
			reprotectBuf(buf, count);

		return rv;
	}

	int mkdir(const char * pathname, mode_t mode) {
		if (_real_mkdir == NULL)
			init_all();

		size_t len = strlen(pathname);
		bool need_to_reprotect = unprotectBuf(pathname, len);

		int rv = (*_real_mkdir)(pathname, mode);
#if 1
		if (need_to_reprotect)
			reprotectBuf(pathname, len);
#endif
		return rv;
	}

	int chdir(const char * path) {
		if (_real_chdir == NULL)
			init_all();

		size_t len = strlen(path);
		bool need_to_reprotect = unprotectBuf(path, len);

		int rv = (*_real_chdir)(path);

		if (need_to_reprotect)
			reprotectBuf(path, len);

		return rv;
	}

	int rmdir(const char * pathname) {
		if (_real_rmdir == NULL)
			init_all();

		size_t len = strlen(pathname);
		bool need_to_reprotect = unprotectBuf(pathname, len);

		int rv = (*_real_rmdir)(pathname);

		if (need_to_reprotect)
			reprotectBuf(pathname, len);

		return rv;
	}

	int setsockopt(int s, int level, int optname, const void * optval, socklen_t optlen) {
		if (_real_setsockopt == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(optval, optlen);

		int rv = (*_real_setsockopt)(s, level, optname, optval, optlen);

		if (need_to_reprotect)
			reprotectBuf(optval, optlen);

		return rv;
	}

	int getsockopt(int s, int level, int optname, void * optval, socklen_t * optlen) {
		if (_real_getsockopt == NULL)
			init_all();

		size_t len = *optlen;
		bool need_to_reprotect = unprotectBuf(optval, len);

		int rv = (*_real_getsockopt)(s, level, optname, optval, optlen);

		if (need_to_reprotect)
			reprotectBuf(optval, len);

		return rv;
	}

	int bind(int sockfd, const struct sockaddr * my_addr, socklen_t addrlen) {
		if (_real_bind == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(my_addr, addrlen);

		int rv = (*_real_bind)(sockfd, my_addr, addrlen);

		if (need_to_reprotect)
			reprotectBuf(my_addr, addrlen);

		return rv;
	}

	int connect(int sockfd, const struct sockaddr * serv_addr, socklen_t addrlen) {
		if (_real_connect == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(serv_addr, addrlen);

		int rv = (*_real_connect)(sockfd, serv_addr, addrlen);

		if (need_to_reprotect)
			reprotectBuf(serv_addr, addrlen);

		return rv;
	}

	int accept(int s, struct sockaddr * addr, socklen_t * addrlen) {
		if (_real_accept == NULL)
			init_all();

		size_t len = *addrlen;
		bool need_to_reprotect = unprotectBuf(addr, len);

		int rv = (*_real_accept)(s, addr, addrlen);

		if (need_to_reprotect)
			reprotectBuf(addr, len);

		return rv;
	}

	int getsockname(int s, struct sockaddr * name, socklen_t * namelen) {
		if (_real_getsockname == NULL)
			init_all();

		size_t len = *namelen;
		bool need_to_reprotect = unprotectBuf(name, len);

		int rv = (*_real_getsockname)(s, name, namelen);

		if (need_to_reprotect)
			reprotectBuf(name, len);

		return rv;
	}

	int getpeername(int s, struct sockaddr * name, socklen_t * namelen) {
		if (_real_getpeername == NULL)
			init_all();

		size_t len = *namelen;
		bool need_to_reprotect = unprotectBuf(name, len);

		int rv = (*_real_getpeername)(s, name, namelen);

		if (need_to_reprotect)
			reprotectBuf(name, len);

		return rv;
	}

	ssize_t send(int s, const void * buf, size_t len, int flags) {
		if (_real_send == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(buf, len);

		ssize_t rv = (*_real_send)(s, buf, len, flags);

		if (need_to_reprotect)
			reprotectBuf(buf, len);

		return rv;
	}

	ssize_t sendto(int s, const void * buf, size_t len, int flags, const struct sockaddr * to, socklen_t tolen) {
		if (_real_sendto == NULL)
			init_all();

		bool need_to_reprotect1 = unprotectBuf(buf, len);
		bool need_to_reprotect2 = unprotectBuf(to, tolen);

		ssize_t rv = (*_real_sendto)(s, buf, len, flags, to, tolen);

		if (need_to_reprotect1)
			reprotectBuf(buf, len);
		if (need_to_reprotect2)
			reprotectBuf(to, tolen);

		return rv;
	}

	ssize_t sendmsg(int s, const struct msghdr * msg, int flags) {
		if (_real_sendmsg == NULL)
			init_all();

		void * addr = msg->msg_name;
		size_t len = msg->msg_namelen;
		bool need_to_reprotect = unprotectBuf(addr, len);

		ssize_t rv = (*_real_sendmsg)(s, msg, flags);

		if (need_to_reprotect)
			reprotectBuf(addr, len);

		return rv;
	}

	ssize_t recv(int s, void * buf, size_t len, int flags) {
		if (_real_recv == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(buf, len);

		ssize_t rv = (*_real_recv)(s, buf, len, flags);

		if (need_to_reprotect)
			reprotectBuf(buf, len);

		return rv;
	}

	ssize_t recvfrom(int s, void * buf, size_t len, int flags, struct sockaddr * from, socklen_t * fromlen) {
		if (_real_recvfrom == NULL)
			init_all();

		size_t len2 = *fromlen;
		bool need_to_reprotect1 = unprotectBuf(buf, len);
		bool need_to_reprotect2 = unprotectBuf(from, len2);

		ssize_t rv = (*_real_recvfrom)(s, buf, len, flags, from, fromlen);

		if (need_to_reprotect1)
			reprotectBuf(buf, len);
		if (need_to_reprotect2)
			reprotectBuf(from, len2);

		return rv;
	}

	ssize_t recvmsg(int s, struct msghdr * msg, int flags) {
		if (_real_recvmsg == NULL)
			init_all();

		void * addr = msg->msg_name;
		size_t len = msg->msg_namelen;
		bool need_to_reprotect = unprotectBuf(addr, len);

		ssize_t rv = (*_real_recvmsg)(s, msg, flags);

		if (need_to_reprotect)
			reprotectBuf(addr, len);

		return rv;
	}

	int poll(struct pollfd * ufds, nfds_t nfds, int timeout) {
		if (_real_poll == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(ufds, nfds * sizeof(*ufds));
		dbprintf("poll(%p, %d, %d)\n", ufds, nfds, timeout);

		int rv = (*_real_poll)(ufds, nfds, timeout);
		dbprintf("poll(%p, %d, %d) returned %d\n", ufds, nfds, timeout, rv);

		if (need_to_reprotect)
			reprotectBuf(ufds, nfds * sizeof(*ufds));

		return rv;
	}

	int select(int n, fd_set * readfds, fd_set * writefds, fd_set * exceptfds, struct timeval * timeout) {
		if (_real_select == NULL)
			init_all();

		size_t len = n / 8;
		bool need_to_reprotect1 = unprotectBuf(readfds, len);
		bool need_to_reprotect2 = unprotectBuf(writefds, len);
		bool need_to_reprotect3 = unprotectBuf(exceptfds, len);
		bool need_to_reprotect4 = unprotectBuf(timeout, sizeof(*timeout));

		int rv = (*_real_select)(n, readfds, writefds, exceptfds, timeout);

		if (need_to_reprotect1)
			reprotectBuf(readfds, len);
		if (need_to_reprotect2)
			reprotectBuf(writefds, len);
		if (need_to_reprotect3)
			reprotectBuf(exceptfds, len);
		if (need_to_reprotect4)
			reprotectBuf(timeout, sizeof(*timeout));

		return rv;
	}

	int pselect(int n, fd_set * readfds, fd_set * writefds, fd_set * exceptfds, const struct timespec * timeout, const sigset_t * sigmask) {
		if (_real_pselect == NULL)
			init_all();

		size_t len = n / 8;
		bool need_to_reprotect1 = unprotectBuf(readfds, len);
		bool need_to_reprotect2 = unprotectBuf(writefds, len);
		bool need_to_reprotect3 = unprotectBuf(exceptfds, len);
		bool need_to_reprotect4 = unprotectBuf(timeout, sizeof(*timeout));
		bool need_to_reprotect5 = unprotectBuf(sigmask, sizeof(*sigmask));

		int rv = (*_real_pselect)(n, readfds, writefds, exceptfds, timeout, sigmask);

		if (need_to_reprotect1)
			reprotectBuf(readfds, len);
		if (need_to_reprotect2)
			reprotectBuf(writefds, len);
		if (need_to_reprotect3)
			reprotectBuf(exceptfds, len);
		if (need_to_reprotect4)
			reprotectBuf(timeout, sizeof(*timeout));
		if (need_to_reprotect5)
			reprotectBuf(sigmask, sizeof(*sigmask));

		return rv;
	}

	int uname(struct utsname * buf) {
		if (_real_uname == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(buf, sizeof(*buf));

		int rv = (*_real_uname)(buf);

		if (need_to_reprotect)
			reprotectBuf(buf, sizeof(*buf));

		return rv;
	}
#if 0
	int getrlimit(int resource, struct rlimit * rlim) {
		if (_real_getrlimit == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(rlim, sizeof(*rlim));

		int rv = (*_real_getrlimit)(resource, rlim);
		dbprintf("getrlimit(%d, %p) returned %d\n", resource, rlim, rv);

		if (need_to_reprotect)
			reprotectBuf(rlim, sizeof(*rlim));

		return rv;
	}
#endif
	int setrlimit(int resource, const struct rlimit * rlim) {
		if (_real_setrlimit == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(rlim, sizeof(*rlim));

		int rv = (*_real_setrlimit)(resource, rlim);

		if (need_to_reprotect)
			reprotectBuf(rlim, sizeof(*rlim));

		return rv;
	}

	int getrusage(int who, struct rusage * usage) {
		if (_real_getrusage == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(usage, sizeof(*usage));

		int rv = (*_real_getrusage)(who, usage);

		if (need_to_reprotect)
			reprotectBuf(usage, sizeof(*usage));

		return rv;
	}

	int shmctl(int shmid, int cmd, struct shmid_ds * buf) {
		if (_real_shmctl == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(buf, sizeof(*buf));

		int rv = (*_real_shmctl)(shmid, cmd, buf);

		if (need_to_reprotect)
			reprotectBuf(buf, sizeof(*buf));

		return rv;
	}

	int sysctl(struct __sysctl_args * args) {
		if (_real_sysctl == NULL)
			init_all();

		bool need_to_reprotect = unprotectBuf(args, sizeof(*args));

		int rv = (*_real_sysctl)(args);

		if (need_to_reprotect)
			reprotectBuf(args, sizeof(*args));

		return rv;
	}

	inline static MemTracer * getMemTracer() {
		static char buf[sizeof(MemTracer)];
		static MemTracer * mt = new (buf) MemTracer;
		return mt;
	}

private:

	typedef void * callocType (size_t, size_t);
	typedef void * mallocType (size_t);
	typedef void freeType (void * ptr);
	typedef void * reallocType (void *, size_t);

	typedef int brkType (void *);
	typedef void * sbrkType (intptr_t);
	typedef void * mmapType (void *, size_t, int, int, int, off_t);
	typedef int munmapType (void *, size_t);
	typedef int madviseType (void *, size_t, int);
	typedef int mprotectType (const void *, size_t, int);
	typedef sighandler_t signalType (int signum, sighandler_t handler);
	typedef int sigactionType (int, const struct sigaction *, struct sigaction *);
//	typedef void exitType (int);

	typedef time_t timeType (time_t *);
	typedef int stimeType (const time_t *);
	typedef int gettimeofdayType (struct timeval *, struct timezone *);
	typedef int settimeofdayType (const struct timeval *, const struct timezone *);
	typedef int getresuidType (uid_t *, uid_t *, uid_t *);
	typedef int getresgidType (gid_t *, gid_t *, gid_t *);
	typedef int sigpendingType (sigset_t *);
	typedef int sigprocmaskType (int, const sigset_t *, sigset_t *);
	typedef int sched_setschedulerType (pid_t, int, const struct sched_param *);
	typedef int sched_setparamType (pid_t, const struct sched_param *);
	typedef int sched_getparamType (pid_t, struct sched_param *);
	typedef pid_t waitType (int *);
	typedef pid_t wait3Type (int *, int, struct rusage *);
	typedef pid_t wait4Type (pid_t, int *, int, struct rusage *);
	typedef int waitidType (idtype_t, id_t, siginfo_t *, int);
	typedef pid_t waitpidType (pid_t, int *, int);
	typedef int statfsType (const char *, struct statfs *);
	typedef int statfs64Type (const char *, struct statfs64 *);
	typedef int fstatfsType (int, struct statfs *);
	typedef int fstatfs64Type (int, struct statfs64 *);
	typedef int linkType (const char *, const char *);
	typedef int symlinkType (const char *, const char *);
	typedef int unlinkType (const char *);
	typedef int renameType (const char *, const char *);
	typedef int chmodType (const char *, mode_t);
	typedef int readlinkType (const char *, char *, size_t);
	typedef int creatType (const char *, mode_t);
	typedef int openType (const char *, int, mode_t);
	typedef int open64Type (const char *, int, mode_t);
	typedef FILE * fopenType (const char *, const char *);
	typedef FILE * fopen64Type (const char *, const char *);
	typedef int accessType (const char *, int);
	typedef int chownType (const char *, uid_t, gid_t);
	typedef int lchownType (const char *, uid_t, gid_t);
	typedef int utimeType (const char *, const struct utimbuf *);
	typedef int utimesType (const char *, const struct timeval *);
	typedef int llseekType (unsigned int, unsigned long, unsigned long, loff_t *, unsigned int);
	typedef ssize_t readType (int, void *, size_t);
	typedef size_t freadType (void *, size_t, size_t, FILE *);
	typedef ssize_t readvType (int, const struct iovec *, int);
	typedef ssize_t writeType (int, const void *, size_t);
	typedef size_t fwriteType (const void *, size_t, size_t, FILE *);
	typedef ssize_t writevType (int, const struct iovec *, int);
	typedef ssize_t preadType (int, void *, size_t, off_t);
	typedef ssize_t pwriteType (int, const void *, size_t, off_t);
	typedef int mkdirType (const char *, mode_t);
	typedef int chdirType (const char *);
	typedef int rmdirType (const char *);
	typedef int setsockoptType (int, int, int, const void *, socklen_t);
	typedef int getsockoptType (int, int, int, void *, socklen_t *);
	typedef int bindType (int, const struct sockaddr *, socklen_t);
	typedef int connectType (int, const struct sockaddr *, socklen_t);
	typedef int acceptType (int, struct sockaddr *, socklen_t *);
	typedef int getsocknameType (int, struct sockaddr *, socklen_t *);
	typedef int getpeernameType (int, struct sockaddr *, socklen_t *);
	typedef ssize_t sendType (int, const void *, size_t, int);
	typedef ssize_t sendtoType (int, const void *, size_t, int, const struct sockaddr *, socklen_t);
	typedef ssize_t sendmsgType (int, const struct msghdr *, int);
	typedef ssize_t recvType (int, void *, size_t, int);
	typedef ssize_t recvfromType (int, void *, size_t, int, struct sockaddr *, socklen_t *);
	typedef ssize_t recvmsgType (int, struct msghdr *, int);
	typedef int pollType (struct pollfd *, nfds_t, int);
	typedef int selectType (int, fd_set *, fd_set *, fd_set *, struct timeval *);
	typedef int pselectType (int, fd_set *, fd_set *, fd_set *, const struct timespec *, const sigset_t *);
	typedef int unameType (struct utsname *);
	typedef int getrlimitType (int, struct rlimit *);
	typedef int setrlimitType (int, const struct rlimit *);
	typedef int getrusageType (int, struct rusage *);
	typedef int shmctlType (int, int, struct shmid_ds *);
	typedef int sysctlType (struct __sysctl_args *);

	typedef void saHandlerType (int);
	typedef void saSigactionType (int, siginfo_t *, void *);

	bool _in_dlsym;

	callocType * _real_calloc;
	mallocType * _real_malloc;
	freeType * _real_free;
	reallocType * _real_realloc;

	brkType * _real_brk;
	sbrkType * _real_sbrk;
	mmapType * _real_mmap;
	munmapType * _real_munmap;
	madviseType * _real_madvise;
	mprotectType * _real_mprotect;
	signalType * _real_signal;
	sigactionType * _real_sigaction;
//	exitType * _real_exit;

	timeType * _real_time;
	stimeType * _real_stime;
	gettimeofdayType * _real_gettimeofday;
	settimeofdayType * _real_settimeofday;
	getresuidType * _real_getresuid;
	getresgidType * _real_getresgid;
	sigpendingType * _real_sigpending;
	sigprocmaskType * _real_sigprocmask;
	sched_setschedulerType * _real_sched_setscheduler;
	sched_setparamType * _real_sched_setparam;
	sched_getparamType * _real_sched_getparam;
	waitType * _real_wait;
	wait3Type * _real_wait3;
	wait4Type * _real_wait4;
	waitidType * _real_waitid;
	waitpidType * _real_waitpid;
	statfsType * _real_statfs;
	statfs64Type * _real_statfs64;
	fstatfsType * _real_fstatfs;
	fstatfs64Type * _real_fstatfs64;
	linkType * _real_link;
	symlinkType * _real_symlink;
	unlinkType * _real_unlink;
	renameType * _real_rename;
	chmodType * _real_chmod;
	readlinkType * _real_readlink;
	creatType * _real_creat;
	openType * _real_open;
	open64Type * _real_open64;
	fopenType * _real_fopen;
	fopen64Type * _real_fopen64;
	accessType * _real_access;
	chownType * _real_chown;
	lchownType * _real_lchown;
	utimeType * _real_utime;
	utimesType * _real_utimes;
	llseekType * _real_llseek;
	readType * _real_read;
	freadType * _real_fread;
	readvType * _real_readv;
	writeType * _real_write;
	fwriteType * _real_fwrite;
	writevType * _real_writev;
	preadType * _real_pread;
	pwriteType * _real_pwrite;
	mkdirType * _real_mkdir;
	chdirType * _real_chdir;
	rmdirType * _real_rmdir;
	setsockoptType * _real_setsockopt;
	getsockoptType * _real_getsockopt;
	bindType * _real_bind;
	connectType * _real_connect;
	acceptType * _real_accept;
	getsocknameType * _real_getsockname;
	getpeernameType * _real_getpeername;
	sendType * _real_send;
	sendtoType * _real_sendto;
	sendmsgType * _real_sendmsg;
	recvType * _real_recv;
	recvfromType * _real_recvfrom;
	recvmsgType * _real_recvmsg;
	pollType * _real_poll;
	selectType * _real_select;
	pselectType * _real_pselect;
	unameType * _real_uname;
	getrlimitType * _real_getrlimit;
	setrlimitType * _real_setrlimit;
	getrusageType * _real_getrusage;
	shmctlType * _real_shmctl;
	sysctlType * _real_sysctl;

	ssize_t currentAllocatedMmap;
	ssize_t currentAllocatedSbrk;
	ssize_t currentMmapDiscarded;
	ssize_t currentSbrkDiscarded;
	ssize_t currentMmapUntouched;
	ssize_t currentSbrkUntouched;
	ssize_t maxAllocated;
	char * initialBreak;
	char * currentBreak;

	MemTracer()
		: _in_dlsym(false),
		  _real_calloc(NULL),
		  _real_malloc(NULL),
		  _real_free(NULL),
		  _real_realloc(NULL),
		  _real_brk(NULL),
		  _real_sbrk(NULL),
		  _real_mmap(NULL),
		  _real_munmap(NULL),
		  _real_madvise(NULL),
		  _real_mprotect(NULL),
		  _real_sigaction(NULL)
//		  _real_exit(NULL),	
	{}

	void init_all() {
		_in_dlsym = true;

		__initialized_memtracer = 0xdeadbeef;

		_real_calloc = (callocType *) dlsym(RTLD_NEXT, "calloc");
		_real_malloc = (mallocType *) dlsym(RTLD_NEXT, "malloc");
		_real_free = (freeType *) dlsym(RTLD_NEXT, "free");
		_real_realloc = (reallocType *) dlsym(RTLD_NEXT, "realloc");

		_real_brk = (brkType *) dlsym(RTLD_NEXT, "brk");
		_real_sbrk = (sbrkType *) dlsym(RTLD_NEXT, "sbrk");
		_real_mmap = (mmapType *) dlsym(RTLD_NEXT, "mmap");
		_real_munmap = (munmapType *) dlsym(RTLD_NEXT, "munmap");
		_real_madvise = (madviseType *) dlsym(RTLD_NEXT, "madvise");
		_real_mprotect = (mprotectType *) dlsym(RTLD_NEXT, "mprotect");
		_real_signal = (signalType *) dlsym(RTLD_NEXT, "signal");
		_real_sigaction = (sigactionType *) dlsym(RTLD_NEXT, "sigaction");
//		_real_exit = (exitType *) dlsym(RTLD_NEXT, "exit");

		_real_time = (timeType *) dlsym(RTLD_NEXT, "time");
		_real_stime = (stimeType *) dlsym(RTLD_NEXT, "stime");
		_real_gettimeofday = (gettimeofdayType *) dlsym(RTLD_NEXT, "gettimeofday");
		_real_settimeofday = (settimeofdayType *) dlsym(RTLD_NEXT, "settimeofday");
		_real_getresuid = (getresuidType *) dlsym(RTLD_NEXT, "getresuid");
		_real_getresgid = (getresgidType *) dlsym(RTLD_NEXT, "getresgid");
		_real_sigpending = (sigpendingType *) dlsym(RTLD_NEXT, "sigpending");
		_real_sigprocmask = (sigprocmaskType *) dlsym(RTLD_NEXT, "sigprocmask");
		_real_sched_setscheduler = (sched_setschedulerType *) dlsym(RTLD_NEXT, "sched_setscheduler");
		_real_sched_setparam = (sched_setparamType *) dlsym(RTLD_NEXT, "sched_setparam");
		_real_sched_getparam = (sched_getparamType *) dlsym(RTLD_NEXT, "sched_getparam");
		_real_wait = (waitType *) dlsym(RTLD_NEXT, "wait");
		_real_wait3 = (wait3Type *) dlsym(RTLD_NEXT, "wait3");
		_real_wait4 = (wait4Type *) dlsym(RTLD_NEXT, "wait4");
		_real_waitid = (waitidType *) dlsym(RTLD_NEXT, "waitid");
		_real_waitpid = (waitpidType *) dlsym(RTLD_NEXT, "waitpid");
		_real_statfs = (statfsType *) dlsym(RTLD_NEXT, "statfs");
		_real_statfs64 = (statfs64Type *) dlsym(RTLD_NEXT, "statfs64");
		_real_fstatfs = (fstatfsType *) dlsym(RTLD_NEXT, "fstatfs");
		_real_fstatfs64 = (fstatfs64Type *) dlsym(RTLD_NEXT, "fstatfs64");
		_real_link = (linkType *) dlsym(RTLD_NEXT, "link");
		_real_symlink = (symlinkType *) dlsym(RTLD_NEXT, "symlink");
		_real_unlink = (unlinkType *) dlsym(RTLD_NEXT, "unlink");
		_real_rename = (renameType *) dlsym(RTLD_NEXT, "rename");
		_real_chmod = (chmodType *) dlsym(RTLD_NEXT, "chmod");
		_real_readlink = (readlinkType *) dlsym(RTLD_NEXT, "readlink");
		_real_creat = (creatType *) dlsym(RTLD_NEXT, "creat");
		_real_open = (openType *) dlsym(RTLD_NEXT, "open");
		_real_open64 = (open64Type *) dlsym(RTLD_NEXT, "open64");
		_real_fopen = (fopenType *) dlsym(RTLD_NEXT, "fopen");
		_real_fopen64 = (fopen64Type *) dlsym(RTLD_NEXT, "fopen64");
		_real_access = (accessType *) dlsym(RTLD_NEXT, "access");
		_real_chown = (chownType *) dlsym(RTLD_NEXT, "chown");
		_real_lchown = (lchownType *) dlsym(RTLD_NEXT, "lchown");
		_real_utime = (utimeType *) dlsym(RTLD_NEXT, "utime");
		_real_utimes = (utimesType *) dlsym(RTLD_NEXT, "utimes");
		_real_llseek = (llseekType *) dlsym(RTLD_NEXT, "llseek");
		_real_read = (readType *) dlsym(RTLD_NEXT, "read");
		_real_fread = (freadType *) dlsym(RTLD_NEXT, "fread");
		_real_readv = (readvType *) dlsym(RTLD_NEXT, "readv");
		_real_write = (writeType *) dlsym(RTLD_NEXT, "write");
		_real_fwrite = (fwriteType *) dlsym(RTLD_NEXT, "fwrite");
		_real_writev = (writevType *) dlsym(RTLD_NEXT, "writev");
		_real_pread = (preadType *) dlsym(RTLD_NEXT, "pread");
		_real_pwrite = (pwriteType *) dlsym(RTLD_NEXT, "pwrite");
		_real_mkdir = (mkdirType *) dlsym(RTLD_NEXT, "mkdir");
		_real_chdir = (chdirType *) dlsym(RTLD_NEXT, "chdir");
		_real_rmdir = (rmdirType *) dlsym(RTLD_NEXT, "rmdir");
		_real_setsockopt = (setsockoptType *) dlsym(RTLD_NEXT, "setsockopt");
		_real_getsockopt = (getsockoptType *) dlsym(RTLD_NEXT, "getsockopt");
		_real_bind = (bindType *) dlsym(RTLD_NEXT, "bind");
		_real_connect = (connectType *) dlsym(RTLD_NEXT, "connect");
		_real_accept = (acceptType *) dlsym(RTLD_NEXT, "accept");
		_real_getsockname = (getsocknameType *) dlsym(RTLD_NEXT, "getsockname");
		_real_getpeername = (getpeernameType *) dlsym(RTLD_NEXT, "getpeername");
		_real_send = (sendType *) dlsym(RTLD_NEXT, "send");
		_real_sendto = (sendtoType *) dlsym(RTLD_NEXT, "sendto");
		_real_sendmsg = (sendmsgType *) dlsym(RTLD_NEXT, "sendmsg");
		_real_recv = (recvType *) dlsym(RTLD_NEXT, "recv");
		_real_recvfrom = (recvfromType *) dlsym(RTLD_NEXT, "recvfrom");
		_real_recvmsg = (recvmsgType *) dlsym(RTLD_NEXT, "recvmsg");
		_real_poll = (pollType *) dlsym(RTLD_NEXT, "poll");
		_real_select = (selectType *) dlsym(RTLD_NEXT, "select");
		_real_pselect = (pselectType *) dlsym(RTLD_NEXT, "pselect");
		_real_uname = (unameType *) dlsym(RTLD_NEXT, "uname");
		_real_getrlimit = (getrlimitType *) dlsym(RTLD_NEXT, "getrlimit");
		_real_setrlimit = (setrlimitType *) dlsym(RTLD_NEXT, "setrlimit");
		_real_getrusage = (getrusageType *) dlsym(RTLD_NEXT, "getrusage");
		_real_shmctl = (shmctlType *) dlsym(RTLD_NEXT, "shmctl");
		_real_sysctl = (sysctlType *) dlsym(RTLD_NEXT, "sysctl");

		_in_dlsym = false;

		assert(_real_calloc != NULL);
		assert(_real_brk != NULL);
		assert(_real_sbrk != NULL);
		assert(_real_mmap != NULL);
		assert(_real_munmap != NULL);
		assert(_real_madvise != NULL);
		assert(_real_mprotect != NULL);
		assert(_real_signal != NULL);
		assert(_real_sigaction != NULL);
//		assert(_real_exit != NULL);

		assert(_real_time != NULL);
		assert(_real_stime != NULL);
		assert(_real_gettimeofday != NULL);
		assert(_real_settimeofday != NULL);
		assert(_real_getresuid != NULL);
		assert(_real_getresgid != NULL);
		assert(_real_sigpending != NULL);
		assert(_real_sigprocmask != NULL);
		assert(_real_sched_setscheduler != NULL);
		assert(_real_sched_setparam != NULL);
		assert(_real_sched_getparam != NULL);
		assert(_real_wait != NULL);
		assert(_real_wait3 != NULL);
		assert(_real_wait4 != NULL);
		assert(_real_waitid != NULL);
		assert(_real_waitpid != NULL);
		assert(_real_statfs != NULL);
		assert(_real_statfs64 != NULL);
		assert(_real_fstatfs != NULL);
		assert(_real_fstatfs64 != NULL);
		assert(_real_link != NULL);
		assert(_real_symlink != NULL);
		assert(_real_unlink != NULL);
		assert(_real_rename != NULL);
		assert(_real_chmod != NULL);
		assert(_real_readlink != NULL);
		assert(_real_creat != NULL);
		assert(_real_open != NULL);
		assert(_real_open64 != NULL);
		assert(_real_fopen != NULL);
		assert(_real_fopen64 != NULL);
		assert(_real_access != NULL);
		assert(_real_chown != NULL);
		assert(_real_lchown != NULL);
		assert(_real_utime != NULL);
		assert(_real_utimes != NULL);
		assert(_real_llseek != NULL);
		assert(_real_read != NULL);
		assert(_real_fread != NULL);
		assert(_real_readv != NULL);
		assert(_real_write != NULL);
		assert(_real_fwrite != NULL);
		assert(_real_writev != NULL);
		assert(_real_pread != NULL);
		assert(_real_pwrite != NULL);
		assert(_real_mkdir != NULL);
		assert(_real_chdir != NULL);
		assert(_real_rmdir != NULL);
		assert(_real_setsockopt != NULL);
		assert(_real_getsockopt != NULL);
		assert(_real_bind != NULL);
		assert(_real_connect != NULL);
		assert(_real_accept != NULL);
		assert(_real_getsockname != NULL);
		assert(_real_getpeername != NULL);
		assert(_real_send != NULL);
		assert(_real_sendto != NULL);
		assert(_real_sendmsg != NULL);
		assert(_real_recv != NULL);
		assert(_real_recvfrom != NULL);
		assert(_real_recvmsg != NULL);
		assert(_real_poll != NULL);
		assert(_real_select != NULL);
		assert(_real_pselect != NULL);
		assert(_real_uname != NULL);
		assert(_real_getrlimit != NULL);
		assert(_real_setrlimit != NULL);
		assert(_real_getrusage != NULL);
		assert(_real_shmctl != NULL);
		assert(_real_sysctl != NULL);

		dbprintf("MemTracer init ending...\n");
	}

	bool unprotectBuf(const void * buf, size_t size) {
	  AOCommon * bl = AOCommon::fromPtr((void *)buf);
	  if(bl) {
	    // NB: possible race here
	    if(bl->isProtected())
	      bl->barrier((DWORD)buf);
	    bl->pin();
	    return true;
	  }
	  else return false;
	}

	void reprotectBuf(const void * buf, size_t size) {
	  AOCommon * bl = AOCommon::fromPtr((void*)buf);
	  bl->unpin();
	}

};

// these are the calls that we are having an eye on

extern "C" {

	// the following calls are intercepted merely because we need to unprotect the user buffers passed into kernel

	time_t time(time_t * t) {
		return MemTracer::getMemTracer()->time(t);
	}

	int stime(const time_t * t) {
		return MemTracer::getMemTracer()->stime(t);
	}

	int gettimeofday(struct timeval * tv, struct timezone * tz) {
		return MemTracer::getMemTracer()->gettimeofday(tv, tz);
	}

	int settimeofday(const struct timeval * tv, const struct timezone * tz) {
		return MemTracer::getMemTracer()->settimeofday(tv, tz);
	}
/*
	int adjtimex(struct timex * buf) {
		return MemTracer::getMemTracer()->adjtimex(buf);
	}

	clock_t times(struct tms * buf) {
		return MemTracer::getMemTracer()->times(buf);
	}

	int nanosleep(const struct timespec * req, struct timespec * rem) {
		return MemTracer::getMemTracer()->nanosleep(req, rem);
	}
*/
	int getresuid(uid_t * ruid, uid_t * euid, uid_t * suid) {
		return MemTracer::getMemTracer()->getresuid(ruid, euid, suid);
	}

	int getresgid(gid_t * rgid, gid_t * egid, gid_t * sgid) {
		return MemTracer::getMemTracer()->getresgid(rgid, egid, sgid);
	}
/*
	int getgroups(int size, gid_t list[]) {
		return MemTracer::getMemTracer()->getgroups(size, list);
	}

	int setgroups(size_t size, const gid_t * list) {
		return MemTracer::getMemTracer()->setgroups(size, list);
	}

	int acct(const char * filename) {
		return MemTracer::getMemTracer()->acct(filename);
	}
*/
	int sigpending(sigset_t * set) {
		return MemTracer::getMemTracer()->sigpending(set);
	}

	int sigprocmask(int how, const sigset_t * set, sigset_t * oldset) {
		return MemTracer::getMemTracer()->sigprocmask(how, set, oldset);
	}
/*
	int getitimer(int which, struct itimerval * value) {
		return MemTracer::getMemTracer()->getitimer(which, value);
	}

	int setitimer(int which, const struct itimerval * value, struct itimerval * ovalue) {
		return MemTracer::getMemTracer()->setitimer(which, value, ovalue);
	}

	int timer_create(clockid_t clockid, struct sigevent *restrict evp,
					 timer_t *restrict timerid);

	int timer_gettime(timer_t timerid, struct itimerspec *value);
	int timer_settime(timer_t timerid, int flags,
					  const struct itimerspec *restrict value,
					  struct itimerspec *restrict ovalue);

	int clock_settime(clockid_t clk_id, const struct timespec *tp);

	int clock_gettime(clockid_t clk_id, struct timespec * tp) {
		return MemTracer::getMemTracer()->clock_gettime(clk_id, tp);
	}

	int clock_getres(clockid_t clk_id, struct timespec *res);
	int clock_nanosleep(clockid_t clock_id, int flags,
						const struct timespec *rqtp, struct timespec *rmtp);
*/
	int sched_setscheduler(pid_t pid, int policy, const struct sched_param * p) {
		return MemTracer::getMemTracer()->sched_setscheduler(pid, policy, p);
	}

	int sched_setparam(pid_t pid, const struct sched_param * p) {
		return MemTracer::getMemTracer()->sched_setparam(pid, p);
	}

	int sched_getparam(pid_t pid, struct sched_param * p) {
		return MemTracer::getMemTracer()->sched_getparam(pid, p);
	}
/*
	int sched_setaffinity(pid_t  pid,  unsigned  int  len,  unsigned  long
						  *mask);

	int  sched_getaffinity(pid_t  pid,  unsigned  int  len,  unsigned long
						   *mask);
	int sched_rr_get_interval(pid_t pid, struct timespec *tp);

	int reboot(int magic, int magic2, int flag, void *arg);
*/

/*
  asmlinkage long sys_kexec_load(unsigned long entry, unsigned long nr_segments,
                                struct kexec_segment __user *segments,
                                unsigned long flags);
*/

	pid_t wait(int * status) {
		return MemTracer::getMemTracer()->wait(status);
	}

	pid_t wait3(int * status, int options, struct rusage * rusage) {
		return MemTracer::getMemTracer()->wait3((int *) status, options, rusage);
	}

	pid_t wait4(pid_t pid, int * status, int options, struct rusage * rusage) {
		return MemTracer::getMemTracer()->wait4(pid, (int *) status, options, rusage);
	}

	int waitid(idtype_t idtype, id_t id, siginfo_t * infop, int options) {
		return MemTracer::getMemTracer()->waitid(idtype, id, infop, options);
	}

	pid_t waitpid(pid_t pid, int * status, int options) {
		return MemTracer::getMemTracer()->waitpid(pid, status, options);
	}

/*
  asmlinkage long sys_set_tid_address(int __user *tidptr);
  asmlinkage long sys_futex(u32 __user *uaddr, int op, int val, struct timespec __user *utime, u32 __user *uaddr2, int val3);

  asmlinkage long sys_init_module(void __user *umod, unsigned long len,
  const char __user *uargs);
  asmlinkage long sys_delete_module(const char __user *name_user,
  unsigned int flags);
  asmlinkage long sys_rt_sigprocmask(int how, sigset_t __user *set,
  sigset_t __user *oset, size_t sigsetsize);
  asmlinkage long sys_rt_sigpending(sigset_t __user *set, size_t sigsetsize);
  asmlinkage long sys_rt_sigtimedwait(const sigset_t __user *uthese,
  siginfo_t __user *uinfo,
  const struct timespec __user *uts,
  size_t sigsetsize);
  asmlinkage long sys_rt_sigqueueinfo(int pid, int sig, siginfo_t __user *uinfo);
*/

/*
	int  sigtimedwait(const  sigset_t  *set, siginfo_t *info, const struct
					  timespec timeout);
	int sigqueue(pid_t pid, int sig, const union sigval value);

	int mount(const char *source, const char *target, const char *filesys-
			  temtype, unsigned long mountflags, const void *data);

	int umount(const char *target);

	int umount2(const char *target, int flags);

	int truncate(const char *path, off_t length);
*/
/*
	int stat(const char * file_name, struct stat * buf) {
		return MemTracer::getMemTracer()->stat(file_name, buf);
	}
*/
	int statfs(const char * path, struct statfs * buf) {
		return MemTracer::getMemTracer()->statfs(path, buf);
	}

	int statfs64(const char * path, struct statfs64 * buf) {
		return MemTracer::getMemTracer()->statfs64(path, buf);
	}

/*
  asmlinkage long sys_statfs64(const char __user *path, size_t sz, struct statfs64 __user *buf);
*/

	int fstatfs(int fd, struct statfs * buf) {
		return MemTracer::getMemTracer()->fstatfs(fd, buf);
	}

	int fstatfs64(int fd, struct statfs64 * buf) {
		return MemTracer::getMemTracer()->fstatfs64(fd, buf);
	}

/*
  asmlinkage long sys_statfs64(const char __user *path, size_t sz, struct statfs64 __user *buf);
*/
/*
	int lstat(const char * file_name, struct stat * buf) {
		return MemTracer::getMemTracer()->lstat(file_name, buf);
	}

	int fstat(int filedes, struct stat * buf) {
		return MemTracer::getMemTracer()->fstat(filedes, buf);
	}
*/
/*
  asmlinkage long sys_stat64(char __user *filename, struct stat64 __user *statbuf);
  asmlinkage long sys_fstat64(unsigned long fd, struct stat64 __user *statbuf);
  asmlinkage long sys_lstat64(char __user *filename, struct stat64 __user *statbuf);
  asmlinkage long sys_truncate64(const char __user *path, loff_t length);
*/
/*
	int setxattr (const char *path, const char *name,
				  const void *value, size_t size, int flags);
	int lsetxattr (const char *path, const char *name,
				   const void *value, size_t size, int flags);
	int fsetxattr (int filedes, const char *name,
				   const void *value, size_t size, int flags);

	ssize_t getxattr (const char *path, const char *name,
					  void *value, size_t size);
	ssize_t lgetxattr (const char *path, const char *name,
					   void *value, size_t size);
	ssize_t fgetxattr (int filedes, const char *name,
					   void *value, size_t size);

	ssize_t listxattr (const char *path,
					   char *list, size_t size);
	ssize_t llistxattr (const char *path,
						char *list, size_t size);
	ssize_t flistxattr (int filedes,
						char *list, size_t size);
	int removexattr (const char *path, const char *name);
	int lremovexattr (const char *path, const char *name);
	int fremovexattr (int filedes, const char *name);

	int mincore(void *start, size_t length, unsigned char *vec);

	int pivot_root(const char *new_root, const char *put_old);

	int chroot(const char *path);
	int mknod(const char *pathname, mode_t mode, dev_t dev);
*/
	int link(const char * oldpath, const char * newpath) {
		return MemTracer::getMemTracer()->link(oldpath, newpath);
	}

	int symlink(const char * oldpath, const char * newpath) {
		return MemTracer::getMemTracer()->symlink(oldpath, newpath);
	}

	int unlink(const char * pathname) {
		return MemTracer::getMemTracer()->unlink(pathname);
	}

	int rename(const char * oldpath, const char * newpath) {
		return MemTracer::getMemTracer()->rename(oldpath, newpath);
	}

	int chmod(const char * path, mode_t mode) {
		return MemTracer::getMemTracer()->chmod(path, mode);
	}

/*
	long io_setup (unsigned nr_events, aio_context_t *ctxp);
	long io_getevents (aio_context_t ctx_id, long min_nr, long nr,
					   struct io_event *events, struct timespec *timeout);
	long io_submit (aio_context_t ctx_id, long nr, struct iocb **iocbpp);
	long io_cancel (aio_context_t ctx_id, struct iocb *iocb,
					struct io_event *result);
	ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count);
*/
/*
  asmlinkage ssize_t sys_sendfile64(int out_fd, int in_fd,
                                loff_t __user *offset, size_t count);
*/

	int readlink(const char * path, char * buf, size_t bufsiz) {
		return MemTracer::getMemTracer()->readlink(path, buf, bufsiz);
	}

	int creat(const char * pathname, mode_t mode) {
		return MemTracer::getMemTracer()->creat(pathname, mode);
	}

	int open(const char * pathname, int flags, ...) {
		va_list ap;
		va_start(ap, flags);
		mode_t mode = va_arg(ap, mode_t);
		return MemTracer::getMemTracer()->open(pathname, flags, mode);
	}

	int open64(const char * pathname, int flags, ...) {
		va_list ap;
		va_start(ap, flags);
		mode_t mode = va_arg(ap, mode_t);
		return MemTracer::getMemTracer()->open64(pathname, flags, mode);
	}

	FILE * fopen(const char * path, const char * mode) {
		return MemTracer::getMemTracer()->fopen(path, mode);
	}

	FILE * fopen64(const char * path, const char * mode) {
		return MemTracer::getMemTracer()->fopen64(path, mode);
	}

	int access(const char * pathname, int mode) {
		return MemTracer::getMemTracer()->access(pathname, mode);
	}

	int chown(const char * path, uid_t owner, gid_t group) {
		return MemTracer::getMemTracer()->chown(path, owner, group);
	}

	int lchown(const char * path, uid_t owner, gid_t group) {
		return MemTracer::getMemTracer()->lchown(path, owner, group);
	}

	int utime(const char * filename, const struct utimbuf * buf) {
		return MemTracer::getMemTracer()->utime(filename, buf);
	}

	int utimes(const char * filename, const struct timeval * tvp) {
		return MemTracer::getMemTracer()->utimes(filename, tvp);
	}

	int llseek(unsigned int fd, unsigned long offset_high, unsigned long offset_low, loff_t * result, unsigned int whence) {
		return MemTracer::getMemTracer()->llseek(fd, offset_high, offset_low, result, whence);
	}

	ssize_t read(int fd, void * buf, size_t count) {
		return MemTracer::getMemTracer()->read(fd, buf, count);
	}

	size_t fread(void * ptr, size_t size, size_t nmemb, FILE * stream) {
		return MemTracer::getMemTracer()->fread(ptr, size, nmemb, stream);
	}

	ssize_t readv(int fd, const struct iovec * vector, int count) {
		return MemTracer::getMemTracer()->readv(fd, vector, count);
	}

	ssize_t write(int fd, const void * buf, size_t count) {
		return MemTracer::getMemTracer()->write(fd, buf, count);
	}

	size_t fwrite(const void * ptr, size_t size, size_t nmemb, FILE * stream) {
		return MemTracer::getMemTracer()->fwrite(ptr, size, nmemb, stream);
	}

	ssize_t writev(int fd, const struct iovec * vector, int count) {
		return MemTracer::getMemTracer()->writev(fd, vector, count);
	}

	ssize_t pread(int fd, void * buf, size_t count, off_t offset) {
		return MemTracer::getMemTracer()->pread(fd, buf, count, offset);
	}

	ssize_t pwrite(int fd, const void * buf, size_t count, off_t offset) {
		return MemTracer::getMemTracer()->pwrite(fd, buf, count, offset);
	}
/*
	char *getcwd(char *buf, size_t size);
	char *get_current_dir_name(void);
	char *getwd(char *buf);
*/
	int mkdir(const char * pathname, mode_t mode) {
		return MemTracer::getMemTracer()->mkdir(pathname, mode);
	}

	int chdir(const char * path) {
		return MemTracer::getMemTracer()->chdir(path);
	}

	int rmdir(const char * pathname) {
		return MemTracer::getMemTracer()->rmdir(pathname);
	}

/*
	int lookup_dcookie(u64 cookie, char * buffer, size_t len);

	int getdents(unsigned int fd, struct dirent * dirp, unsigned int count) {
		return MemTracer::getMemTracer()->getdents(fd, dirp, count);
	}
*/
	int setsockopt(int s, int level, int optname, const void * optval, socklen_t optlen) {
		return MemTracer::getMemTracer()->setsockopt(s, level, optname, optval, optlen);
	}

	int getsockopt(int s, int level, int optname, void * optval, socklen_t * optlen) {
		return MemTracer::getMemTracer()->getsockopt(s, level, optname, optval, optlen);
	}

	int bind(int sockfd, const struct sockaddr * my_addr, socklen_t addrlen) {
		return MemTracer::getMemTracer()->bind(sockfd, my_addr, addrlen);
	}

	int connect(int sockfd, const struct sockaddr * serv_addr, socklen_t addrlen) {
		return MemTracer::getMemTracer()->connect(sockfd, serv_addr, addrlen);
	}

	int accept(int s, struct sockaddr * addr, socklen_t * addrlen) {
		return MemTracer::getMemTracer()->accept(s, addr, addrlen);
	}

	int getsockname(int s, struct sockaddr * name, socklen_t * namelen) {
		return MemTracer::getMemTracer()->getsockname(s, name, namelen);
	}

	int getpeername(int s, struct sockaddr * name, socklen_t * namelen) {
		return MemTracer::getMemTracer()->getpeername(s, name, namelen);
	}

	ssize_t send(int s, const void * buf, size_t len, int flags) {
		return MemTracer::getMemTracer()->send(s, buf, len, flags);
	}

	ssize_t sendto(int s, const void * buf, size_t len, int flags, const struct sockaddr * to, socklen_t tolen) {
		return MemTracer::getMemTracer()->sendto(s, buf, len, flags, to, tolen);
	}

	ssize_t sendmsg(int s, const struct msghdr * msg, int flags) {
		return MemTracer::getMemTracer()->sendmsg(s, msg, flags);
	}

	ssize_t recv(int s, void * buf, size_t len, int flags) {
		return MemTracer::getMemTracer()->recv(s, buf, len, flags);
	}

	ssize_t recvfrom(int s, void * buf, size_t len, int flags, struct sockaddr * from, socklen_t * fromlen) {
		return MemTracer::getMemTracer()->recvfrom(s, buf, len, flags, from, fromlen);
	}

	ssize_t recvmsg(int s, struct msghdr * msg, int flags) {
		return MemTracer::getMemTracer()->recvmsg(s, msg, flags);
	}

/*
	int socketpair(int d, int type, int protocol, int sv[2]);
	int socketcall(int call, unsigned long *args);
*/
	int poll(struct pollfd * ufds, nfds_t nfds, int timeout) {
		return MemTracer::getMemTracer()->poll(ufds, nfds, timeout);
	}

	int select(int n, fd_set * readfds, fd_set * writefds, fd_set * exceptfds, struct timeval * timeout) {
		return MemTracer::getMemTracer()->select(n, readfds, writefds, exceptfds, timeout);
	}

	int pselect(int n, fd_set * readfds, fd_set * writefds, fd_set * exceptfds, const struct timespec * timeout, const sigset_t * sigmask) {
		return MemTracer::getMemTracer()->pselect(n, readfds, writefds, exceptfds, timeout, sigmask);
	}
/*
	int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
		int  epoll_wait(int  epfd, struct epoll_event * events, int maxevents,
						int timeout)

		int gethostname(char *name, size_t len);
	int sethostname(const char *name, size_t len);

	int getdomainname(char *name, size_t len);
	int setdomainname(const char *name, size_t len);
*/
	int uname(struct utsname * buf) {
		return MemTracer::getMemTracer()->uname(buf);
	}
#if 0
	int getrlimit(int resource, struct rlimit * rlim) {
		return MemTracer::getMemTracer()->getrlimit(resource, rlim);
	}
#endif
	int setrlimit(int resource, const struct rlimit * rlim) {
		return MemTracer::getMemTracer()->setrlimit(resource, rlim);
	}

	int getrusage(int who, struct rusage * usage) {
		return MemTracer::getMemTracer()->getrusage(who, usage);
	}
/*
	int msgsnd(int msqid, struct msgbuf *msgp, size_t msgsz, int msgflg);

	ssize_t msgrcv(int msqid, struct msgbuf *msgp, size_t msgsz, long msg-
				   typ, int msgflg);
	int msgctl(int msqid, int cmd, struct msqid_ds *buf);
*/
/*
	void * shmat(int shmid, const void * shmaddr, int shmflg) {
		return MemTracer::getMemTracer()->shmat(shmid, shmadd, shmflg);
	}

	int shmdt(const void * shmaddr) {
		return MemTracer::getMemTracer()->shmdt(shmaddr);
	}
*/
	int shmctl(int shmid, int cmd, struct shmid_ds * buf) {
		return MemTracer::getMemTracer()->shmctl(shmid, cmd, buf);
	}
/*
	asmlinkage long sys_mq_open(const char __user *name, int oflag, mode_t mode, str
								uct mq_attr __user *attr);
	asmlinkage long sys_mq_unlink(const char __user *name);
	asmlinkage long sys_mq_timedsend(mqd_t mqdes, const char __user *msg_ptr, size_t
									 msg_len, unsigned int msg_prio, const struct timespec __user *abs_timeout);
	asmlinkage ssize_t sys_mq_timedreceive(mqd_t mqdes, char __user *msg_ptr, size_t
										   msg_len, unsigned int __user *msg_prio, const struct timespec __user *abs_timeo
										   ut);
	asmlinkage long sys_mq_notify(mqd_t mqdes, const struct sigevent __user *notific
								  ation);
	asmlinkage long sys_mq_getsetattr(mqd_t mqdes, const struct mq_attr __user *mqst
									  at, struct mq_attr __user *omqstat);
	asmlinkage long sys_pciconfig_read(unsigned long bus, unsigned long dfn,
									   unsigned long off, unsigned long len,
									   void __user *buf);
	asmlinkage long sys_pciconfig_write(unsigned long bus, unsigned long dfn,
										unsigned long off, unsigned long len,
										void __user *buf);

	asmlinkage long sys_swapon(const char __user *specialfile, int swap_flags);
	asmlinkage long sys_swapoff(const char __user *specialfile);
*/
	int sysctl(struct __sysctl_args * args) {
		return MemTracer::getMemTracer()->sysctl(args);
	}
/*
	asmlinkage long sys_sysinfo(struct sysinfo __user *info);

	asmlinkage long sys_nfsservctl(int cmd,
								   struct nfsctl_arg __user *arg,
								   void __user *res);
	asmlinkage long sys_syslog(int type, char __user *buf, int len);
	asmlinkage long sys_uselib(const char __user *library);

	asmlinkage long sys_add_key(const char __user *_type,
								const char __user *_description,
								const void __user *_payload,
								size_t plen,
								key_serial_t destringid);

	asmlinkage long sys_request_key(const char __user *_type,
									const char __user *_description,
									const char __user *_callout_info,
									key_serial_t destringid);

	asmlinkage long sys_set_mempolicy(int mode, unsigned long __user *nmask,
									  unsigned long maxnode);
*/
}

#endif
