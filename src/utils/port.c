
/*
 bctoolbox
 Copyright (C) 2016  Belledonne Communications SARL

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/



#include "bctoolbox/logging.h"
#include "bctoolbox/port.h"
#include "bctoolbox/list.h"
#include "utils.h"

#if	defined(_WIN32) && !defined(_WIN32_WCE)
#include <process.h>
#endif

#ifdef _MSC_VER
#ifndef access
#define access _access
#endif
#endif

#ifdef HAVE_SYS_SHM_H
#include <sys/shm.h>
#endif

#ifndef MIN
#define MIN(a,b) a<=b ? a : b
#endif

static void *bctbx_libc_malloc(size_t sz){
	return malloc(sz);
}

static void *bctbx_libc_realloc(void *ptr, size_t sz){
	return realloc(ptr,sz);
}

static void bctbx_libc_free(void*ptr){
	free(ptr);
}

static bool_t allocator_used=FALSE;

static BctoolboxMemoryFunctions bctbx_allocator={
	bctbx_libc_malloc,
	bctbx_libc_realloc,
	bctbx_libc_free
};

void bctbx_set_memory_functions(BctoolboxMemoryFunctions *functions){
	if (allocator_used){
		bctbx_fatal("bctbx_set_memory_functions() must be called before "
		"first use of bctbx_malloc or bctbx_realloc");
		return;
	}
	bctbx_allocator=*functions;
}

void* bctbx_malloc(size_t sz){
	allocator_used=TRUE;
	return bctbx_allocator.malloc_fun(sz);
}

void* bctbx_realloc(void *ptr, size_t sz){
	allocator_used=TRUE;
	return bctbx_allocator.realloc_fun(ptr,sz);
}

void bctbx_free(void* ptr){
	bctbx_allocator.free_fun(ptr);
}

void * bctbx_malloc0(size_t size){
	void *ptr=bctbx_malloc(size);
	memset(ptr,0,size);
	return ptr;
}

char * bctbx_strdup(const char *tmp){
	size_t sz;
	char *ret;
	if (tmp==NULL)
	  return NULL;
	sz=strlen(tmp)+1;
	ret=(char*)bctbx_malloc(sz);
	strcpy(ret,tmp);
	ret[sz-1]='\0';
	return ret;
}

/*
 * this method is an utility method that calls fnctl() on UNIX or
 * ioctlsocket on Win32.
 * int retrun the result of the system method
 */
int bctbx_socket_set_non_blocking(bctbx_socket_t sock){
#if	!defined(_WIN32) && !defined(_WIN32_WCE)
	return fcntl (sock, F_SETFL, O_NONBLOCK);
#else
	unsigned long nonBlock = 1;
	return ioctlsocket(sock, FIONBIO , &nonBlock);
#endif
}


/*
 * this method is an utility method that calls close() on UNIX or
 * closesocket on Win32.
 * int retrun the result of the system method
 */
int bctbx_socket_close(bctbx_socket_t sock){
#if	!defined(_WIN32) && !defined(_WIN32_WCE)
	return close (sock);
#else
	return closesocket(sock);
#endif
}

int bctbx_file_exist(const char *pathname) {
	return access(pathname,F_OK);
}

#if	!defined(_WIN32) && !defined(_WIN32_WCE)
	/* Use UNIX inet_aton method */
#else
	int __bctbx_WIN_inet_aton (const char * cp, struct in_addr * addr)
	{
		unsigned long retval;

		retval = inet_addr (cp);

		if (retval == INADDR_NONE)
		{
			return -1;
		}
		else
		{
			addr->S_un.S_addr = retval;
			return 1;
		}
	}
#endif

char *bctbx_strndup(const char *str,int n){
	int min=MIN((int)strlen(str),n)+1;
	char *ret=(char*)bctbx_malloc(min);
	strncpy(ret,str,min);
	ret[min-1]='\0';
	return ret;
}

#if	!defined(_WIN32) && !defined(_WIN32_WCE)
int __bctbx_thread_join(bctbx_thread_t thread, void **ptr){
	int err=pthread_join(thread,ptr);
	if (err!=0) {
		bctbx_error("pthread_join error: %s",strerror(err));
	}
	return err;
}

int __bctbx_thread_create(bctbx_thread_t *thread, pthread_attr_t *attr, void * (*routine)(void*), void *arg){
	pthread_attr_t my_attr;
	pthread_attr_init(&my_attr);
	if (attr)
		my_attr = *attr;
#ifdef BCTBX_DEFAULT_THREAD_STACK_SIZE
	if (BCTBX_DEFAULT_THREAD_STACK_SIZE!=0)
		pthread_attr_setstacksize(&my_attr, BCTBX_DEFAULT_THREAD_STACK_SIZE);
#endif
	return pthread_create(thread, &my_attr, routine, arg);
}

unsigned long __bctbx_thread_self(void) {
	return (unsigned long)pthread_self();
}

#endif
#if	defined(_WIN32) || defined(_WIN32_WCE)

int __bctbx_WIN_mutex_init(bctbx_mutex_t *mutex, void *attr)
{
#ifdef BCTBX_WINDOWS_DESKTOP
	*mutex=CreateMutex(NULL, FALSE, NULL);
#else
	InitializeSRWLock(mutex);
#endif
	return 0;
}

int __bctbx_WIN_mutex_lock(bctbx_mutex_t * hMutex)
{
#ifdef BCTBX_WINDOWS_DESKTOP
	WaitForSingleObject(*hMutex, INFINITE); /* == WAIT_TIMEOUT; */
#else
	AcquireSRWLockExclusive(hMutex);
#endif
	return 0;
}

int __bctbx_WIN_mutex_unlock(bctbx_mutex_t * hMutex)
{
#ifdef BCTBX_WINDOWS_DESKTOP
	ReleaseMutex(*hMutex);
#else
	ReleaseSRWLockExclusive(hMutex);
#endif
	return 0;
}

int __bctbx_WIN_mutex_destroy(bctbx_mutex_t * hMutex)
{
#ifdef BCTBX_WINDOWS_DESKTOP
	CloseHandle(*hMutex);
#endif
	return 0;
}

typedef struct thread_param{
	void * (*func)(void *);
	void * arg;
}thread_param_t;

static unsigned WINAPI thread_starter(void *data){
	thread_param_t *params=(thread_param_t*)data;
	params->func(params->arg);
	bctbx_free(data);
	return 0;
}

#if defined _WIN32_WCE
#    define _beginthreadex	CreateThread
#    define	_endthreadex	ExitThread
#endif

int __bctbx_WIN_thread_create(bctbx_thread_t *th, void *attr, void * (*func)(void *), void *data)
{
	thread_param_t *params=bctbx_new(thread_param_t,1);
	params->func=func;
	params->arg=data;
	*th=(HANDLE)_beginthreadex( NULL, 0, thread_starter, params, 0, NULL);
	return 0;
}

int __bctbx_WIN_thread_join(bctbx_thread_t thread_h, void **unused)
{
	if (thread_h!=NULL)
	{
		WaitForSingleObjectEx(thread_h, INFINITE, FALSE);
		CloseHandle(thread_h);
	}
	return 0;
}

unsigned long __bctbx_WIN_thread_self(void) {
	return (unsigned long)GetCurrentThreadId();
}

int __bctbx_WIN_cond_init(bctbx_cond_t *cond, void *attr)
{
#ifdef BCTBX_WINDOWS_DESKTOP
	*cond=CreateEvent(NULL, FALSE, FALSE, NULL);
#else
	InitializeConditionVariable(cond);
#endif
	return 0;
}

int __bctbx_WIN_cond_wait(bctbx_cond_t* hCond, bctbx_mutex_t * hMutex)
{
#ifdef BCTBX_WINDOWS_DESKTOP
	//gulp: this is not very atomic ! bug here ?
	__bctbx_WIN_mutex_unlock(hMutex);
	WaitForSingleObject(*hCond, INFINITE);
	__bctbx_WIN_mutex_lock(hMutex);
#else
	SleepConditionVariableSRW(hCond, hMutex, INFINITE, 0);
#endif
	return 0;
}

int __bctbx_WIN_cond_signal(bctbx_cond_t * hCond)
{
#ifdef BCTBX_WINDOWS_DESKTOP
	SetEvent(*hCond);
#else
	WakeConditionVariable(hCond);
#endif
	return 0;
}

int __bctbx_WIN_cond_broadcast(bctbx_cond_t * hCond)
{
	__bctbx_WIN_cond_signal(hCond);
	return 0;
}

int __bctbx_WIN_cond_destroy(bctbx_cond_t * hCond)
{
#ifdef BCTBX_WINDOWS_DESKTOP
	CloseHandle(*hCond);
#endif
	return 0;
}

#if defined(_WIN32_WCE)
#include <time.h>

const char * bctbx_strerror(DWORD value) {
	static TCHAR msgBuf[256];
	FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			value,
			0, // Default language
			(LPTSTR) &msgBuf,
			0,
			NULL
	);
	return (const char *)msgBuf;
}

int
gettimeofday (struct timeval *tv, void *tz)
{
  DWORD timemillis = GetTickCount();
  tv->tv_sec  = timemillis/1000;
  tv->tv_usec = (timemillis - (tv->tv_sec*1000)) * 1000;
  return 0;
}

#else

int bctbx_gettimeofday (struct timeval *tv, void* tz)
{
	union
	{
		__int64 ns100; /*time since 1 Jan 1601 in 100ns units */
		FILETIME fileTime;
	} now;

	GetSystemTimeAsFileTime (&now.fileTime);
	tv->tv_usec = (long) ((now.ns100 / 10LL) % 1000000LL);
	tv->tv_sec = (long) ((now.ns100 - 116444736000000000LL) / 10000000LL);
	return (0);
}

#endif

const char *__bctbx_getWinSocketError(int error)
{
	static char buf[80];

	switch (error)
	{
		case WSANOTINITIALISED: return "Windows sockets not initialized : call WSAStartup";
		case WSAEADDRINUSE:		return "Local Address already in use";
		case WSAEADDRNOTAVAIL:	return "The specified address is not a valid address for this machine";
		case WSAEINVAL:			return "The socket is already bound to an address.";
		case WSAENOBUFS:		return "Not enough buffers available, too many connections.";
		case WSAENOTSOCK:		return "The descriptor is not a socket.";
		case WSAECONNRESET:		return "Connection reset by peer";

		default :
			sprintf(buf, "Error code : %d", error);
			return buf;
		break;
	}

	return buf;
}

#ifdef _WORKAROUND_MINGW32_BUGS
char * WSAAPI gai_strerror(int errnum){
	 return (char*)__bctbx_getWinSocketError(errnum);
}
#endif

#endif

#ifndef _WIN32

#include <sys/socket.h>
#include <netdb.h>
#include <sys/un.h>
#include <sys/stat.h>

static char *make_pipe_name(const char *name){
	return bctbx_strdup_printf("/tmp/%s",name);
}

/* portable named pipes */
bctbx_socket_t bctbx_server_pipe_create(const char *name){
	struct sockaddr_un sa;
	char *pipename=make_pipe_name(name);
	bctbx_socket_t sock;
	sock=socket(AF_UNIX,SOCK_STREAM,0);
	sa.sun_family=AF_UNIX;
	strncpy(sa.sun_path,pipename,sizeof(sa.sun_path)-1);
	unlink(pipename);/*in case we didn't finished properly previous time */
	bctbx_free(pipename);
	fchmod(sock,S_IRUSR|S_IWUSR);
	if (bind(sock,(struct sockaddr*)&sa,sizeof(sa))!=0){
		bctbx_error("Failed to bind command unix socket: %s",strerror(errno));
		return -1;
	}
	listen(sock,1);
	return sock;
}

bctbx_socket_t bctbx_server_pipe_accept_client(bctbx_socket_t server){
	struct sockaddr_un su;
	socklen_t ssize=sizeof(su);
	bctbx_socket_t client_sock=accept(server,(struct sockaddr*)&su,&ssize);
	return client_sock;
}

int bctbx_server_pipe_close_client(bctbx_socket_t client){
	return close(client);
}

int bctbx_server_pipe_close(bctbx_socket_t spipe){
	struct sockaddr_un sa;
	socklen_t len=sizeof(sa);
	int err;
	/*this is to retrieve the name of the pipe, in order to unlink the file*/
	err=getsockname(spipe,(struct sockaddr*)&sa,&len);
	if (err==0){
		unlink(sa.sun_path);
	}else bctbx_error("getsockname(): %s",strerror(errno));
	return close(spipe);
}

bctbx_socket_t bctbx_client_pipe_connect(const char *name){
	bctbx_socket_t sock = -1;
	struct sockaddr_un sa;
	struct stat fstats;
	char *pipename=make_pipe_name(name);
	uid_t uid = getuid();

	// check that the creator of the pipe is us
	if( (stat(name, &fstats) == 0) && (fstats.st_uid != uid) ){
		bctbx_error("UID of file %s (%lu) differs from ours (%lu)", pipename, (unsigned long)fstats.st_uid, (unsigned long)uid);
		return -1;
	}

	sock = socket(AF_UNIX,SOCK_STREAM,0);
	sa.sun_family=AF_UNIX;
	strncpy(sa.sun_path,pipename,sizeof(sa.sun_path)-1);
	bctbx_free(pipename);
	if (connect(sock,(struct sockaddr*)&sa,sizeof(sa))!=0){
		close(sock);
		return -1;
	}
	return sock;
}

int bctbx_pipe_read(bctbx_socket_t p, uint8_t *buf, int len){
	return read(p,buf,len);
}

int bctbx_pipe_write(bctbx_socket_t p, const uint8_t *buf, int len){
	return write(p,buf,len);
}

int bctbx_client_pipe_close(bctbx_socket_t sock){
	return close(sock);
}

#ifdef HAVE_SYS_SHM_H

void *bctbx_shm_open(unsigned int keyid, int size, int create){
	key_t key=keyid;
	void *mem;
	int perms=S_IRUSR|S_IWUSR;
	int fd=shmget(key,size,create ? (IPC_CREAT | perms ) : perms);
	if (fd==-1){
		printf("shmget failed: %s\n",strerror(errno));
		return NULL;
	}
	mem=shmat(fd,NULL,0);
	if (mem==(void*)-1){
		printf("shmat() failed: %s", strerror(errno));
		return NULL;
	}
	return mem;
}

void bctbx_shm_close(void *mem){
	shmdt(mem);
}

#endif

#elif defined(_WIN32) && !defined(_WIN32_WCE)

static char *make_pipe_name(const char *name){
	return bctbx_strdup_printf("\\\\.\\pipe\\%s",name);
}

static HANDLE event=NULL;

/* portable named pipes */
bctbx_pipe_t bctbx_server_pipe_create(const char *name){
#ifdef BCTBX_WINDOWS_DESKTOP
	bctbx_pipe_t h;
	char *pipename=make_pipe_name(name);
	h=CreateNamedPipe(pipename,PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED,PIPE_TYPE_MESSAGE|PIPE_WAIT,1,
						32768,32768,0,NULL);
	bctbx_free(pipename);
	if (h==INVALID_HANDLE_VALUE){
		bctbx_error("Fail to create named pipe %s",pipename);
	}
	if (event==NULL) event=CreateEvent(NULL,TRUE,FALSE,NULL);
	return h;
#else
	bctbx_error("%s not supported!", __FUNCTION__);
	return INVALID_HANDLE_VALUE;
#endif
}


/*this function is a bit complex because we need to wakeup someday
even if nobody connects to the pipe.
bctbx_server_pipe_close() makes this function to exit.
*/
bctbx_pipe_t bctbx_server_pipe_accept_client(bctbx_pipe_t server){
#ifdef BCTBX_WINDOWS_DESKTOP
	OVERLAPPED ol;
	DWORD undef;
	HANDLE handles[2];
	memset(&ol,0,sizeof(ol));
	ol.hEvent=CreateEvent(NULL,TRUE,FALSE,NULL);
	ConnectNamedPipe(server,&ol);
	handles[0]=ol.hEvent;
	handles[1]=event;
	WaitForMultipleObjects(2,handles,FALSE,INFINITE);
	if (GetOverlappedResult(server,&ol,&undef,FALSE)){
		CloseHandle(ol.hEvent);
		return server;
	}
	CloseHandle(ol.hEvent);
	return INVALID_HANDLE_VALUE;
#else
	bctbx_error("%s not supported!", __FUNCTION__);
	return INVALID_HANDLE_VALUE;
#endif
}

int bctbx_server_pipe_close_client(bctbx_pipe_t server){
#ifdef BCTBX_WINDOWS_DESKTOP
	return DisconnectNamedPipe(server)==TRUE ? 0 : -1;
#else
	bctbx_error("%s not supported!", __FUNCTION__);
	return -1;
#endif
}

int bctbx_server_pipe_close(bctbx_pipe_t spipe){
#ifdef BCTBX_WINDOWS_DESKTOP
	SetEvent(event);
	//CancelIoEx(spipe,NULL); /*vista only*/
	return CloseHandle(spipe);
#else
	bctbx_error("%s not supported!", __FUNCTION__);
	return -1;
#endif
}

bctbx_pipe_t bctbx_client_pipe_connect(const char *name){
#ifdef BCTBX_WINDOWS_DESKTOP
	char *pipename=make_pipe_name(name);
	bctbx_pipe_t hpipe = CreateFile(
		 pipename,   // pipe name
		 GENERIC_READ |  // read and write access
		 GENERIC_WRITE,
		 0,              // no sharing
		 NULL,           // default security attributes
		 OPEN_EXISTING,  // opens existing pipe
		 0,              // default attributes
		 NULL);          // no template file
	bctbx_free(pipename);
	return hpipe;
#else
	bctbx_error("%s not supported!", __FUNCTION__);
	return INVALID_HANDLE_VALUE;
#endif
}

int bctbx_pipe_read(bctbx_pipe_t p, uint8_t *buf, int len){
	DWORD ret=0;
	if (ReadFile(p,buf,len,&ret,NULL))
		return ret;
	/*bctbx_error("Could not read from pipe: %s",strerror(GetLastError()));*/
	return -1;
}

int bctbx_pipe_write(bctbx_pipe_t p, const uint8_t *buf, int len){
	DWORD ret=0;
	if (WriteFile(p,buf,len,&ret,NULL))
		return ret;
	/*bctbx_error("Could not write to pipe: %s",strerror(GetLastError()));*/
	return -1;
}


int bctbx_client_pipe_close(bctbx_pipe_t sock){
	return CloseHandle(sock);
}


typedef struct MapInfo{
	HANDLE h;
	void *mem;
}MapInfo;

static bctbx_list_t *maplist=NULL;

void *bctbx_shm_open(unsigned int keyid, int size, int create){
#ifdef BCTBX_WINDOWS_DESKTOP
	HANDLE h;
	char name[64];
	void *buf;

	snprintf(name,sizeof(name),"%x",keyid);
	if (create){
		h = CreateFileMapping(
			INVALID_HANDLE_VALUE,    // use paging file
			NULL,                    // default security
			PAGE_READWRITE,          // read/write access
			0,                       // maximum object size (high-order DWORD)
			size,                // maximum object size (low-order DWORD)
			name);                 // name of mapping object
	}else{
		h = OpenFileMapping(
			FILE_MAP_ALL_ACCESS,   // read/write access
			FALSE,                 // do not inherit the name
			name);               // name of mapping object
	}
	if (h==(HANDLE)-1) {
		bctbx_error("Fail to open file mapping (create=%i)",create);
		return NULL;
	}
	buf = (LPTSTR) MapViewOfFile(h, // handle to map object
		FILE_MAP_ALL_ACCESS,  // read/write permission
		0,
		0,
		size);
	if (buf!=NULL){
		MapInfo *i=(MapInfo*)bctbx_new(MapInfo,1);
		i->h=h;
		i->mem=buf;
		maplist=bctbx_list_append(maplist,i);
	}else{
		CloseHandle(h);
		bctbx_error("MapViewOfFile failed");
	}
	return buf;
#else
	bctbx_error("%s not supported!", __FUNCTION__);
	return NULL;
#endif
}

void bctbx_shm_close(void *mem){
#ifdef BCTBX_WINDOWS_DESKTOP
	bctbx_list_t *elem;
	for(elem=maplist;elem;elem=bctbx_list_next(elem)){
		MapInfo *i=(MapInfo*)bctbx_list_get_data(elem);
		if (i->mem==mem){
			CloseHandle(i->h);
			UnmapViewOfFile(mem);
			bctbx_free(i);
			maplist=bctbx_list_remove_link(maplist,elem);
			return;
		}
	}
	bctbx_error("No shared memory at %p was found.",mem);
#else
	bctbx_error("%s not supported!", __FUNCTION__);
#endif
}


#endif


#ifdef __MACH__
#include <sys/types.h>
#include <sys/timeb.h>
#endif

void _bctbx_get_cur_time(bctoolboxTimeSpec *ret, bool_t realtime){
#if defined(_WIN32_WCE) || defined(WIN32)
#ifdef BCTBX_WINDOWS_DESKTOP
	DWORD timemillis;
#	if defined(_WIN32_WCE)
	timemillis=GetTickCount();
#	else
	timemillis=timeGetTime();
#	endif
	ret->tv_sec=timemillis/1000;
	ret->tv_nsec=(timemillis%1000)*1000000LL;
#else
	ULONGLONG timemillis = GetTickCount64();
	ret->tv_sec = timemillis / 1000;
	ret->tv_nsec = (timemillis % 1000) * 1000000LL;
#endif
#elif defined(__MACH__) && defined(__GNUC__) && (__GNUC__ >= 3)
	struct timeval tv;
	gettimeofday(&tv, NULL);
	ret->tv_sec=tv.tv_sec;
	ret->tv_nsec=tv.tv_usec*1000LL;
#elif defined(__MACH__)
	struct timeb time_val;

	ftime (&time_val);
	ret->tv_sec = time_val.time;
	ret->tv_nsec = time_val.millitm * 1000000LL;
#else
	struct timespec ts;
	if (clock_gettime(realtime ? CLOCK_REALTIME : CLOCK_MONOTONIC,&ts)<0){
		bctbx_fatal("clock_gettime() doesn't work: %s",strerror(errno));
	}
	ret->tv_sec=ts.tv_sec;
	ret->tv_nsec=ts.tv_nsec;
#endif
}

void bctbx_get_cur_time(bctoolboxTimeSpec *ret){
	_bctbx_get_cur_time(ret, FALSE);
}


uint64_t bctbx_get_cur_time_ms(void) {
	bctoolboxTimeSpec ts;
	bctbx_get_cur_time(&ts);
	return (ts.tv_sec * 1000LL) + ((ts.tv_nsec + 500000LL) / 1000000LL);
}

void bctbx_sleep_ms(int ms){
#ifdef _WIN32
#ifdef BCTBX_WINDOWS_DESKTOP
	Sleep(ms);
#else
	HANDLE sleepEvent = CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
	if (!sleepEvent) return;
	WaitForSingleObjectEx(sleepEvent, ms, FALSE);
	CloseHandle(sleepEvent);
#endif
#else
	struct timespec ts;
	ts.tv_sec=ms/1000;
	ts.tv_nsec=(ms%1000)*1000000LL;
	nanosleep(&ts,NULL);
#endif
}

void bctbx_sleep_until(const bctoolboxTimeSpec *ts){
#ifdef __linux
	struct timespec rq;
	rq.tv_sec=ts->tv_sec;
	rq.tv_nsec=ts->tv_nsec;
	while (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &rq, NULL)==-1 && errno==EINTR){
	}
#else
	bctoolboxTimeSpec current;
	bctoolboxTimeSpec diff;
	_bctbx_get_cur_time(&current, TRUE);
	diff.tv_sec=ts->tv_sec-current.tv_sec;
	diff.tv_nsec=ts->tv_nsec-current.tv_nsec;
	if (diff.tv_nsec<0){
		diff.tv_nsec+=1000000000LL;
		diff.tv_sec-=1;
	}
#ifdef _WIN32
		bctbx_sleep_ms((int)((diff.tv_sec * 1000LL) + (diff.tv_nsec/1000000LL)));
#else
	{
		struct timespec dur,rem;
		dur.tv_sec=diff.tv_sec;
		dur.tv_nsec=diff.tv_nsec;
		while (nanosleep(&dur,&rem)==-1 && errno==EINTR){
			dur=rem;
		};
	}
#endif
#endif
}

#if defined(_WIN32) && !defined(_MSC_VER)
char* strtok_r(char *str, const char *delim, char **nextp){
	char *ret;

	if (str == NULL){
		str = *nextp;
	}
	str += strspn(str, delim);
	if (*str == '\0'){
		return NULL;
	}
	ret = str;
	str += strcspn(str, delim);
	if (*str){
		*str++ = '\0';
	}
	*nextp = str;
	return ret;
}
#endif


#if defined(_WIN32) && !defined(_MSC_VER)
#include <wincrypt.h>
static int bctbx_wincrypto_random(unsigned int *rand_number){
	static HCRYPTPROV hProv=(HCRYPTPROV)-1;
	static int initd=0;
	
	if (!initd){
		if (!CryptAcquireContext(&hProv,NULL,NULL,PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)){
			bctbx_error("bctbx_wincrypto_random(): Could not acquire a windows crypto context");
			return -1;
		}
		initd=TRUE;
	}
	if (hProv==(HCRYPTPROV)-1)
		return -1;
	
	if (!CryptGenRandom(hProv,4,(BYTE*)rand_number)){
		bctbx_error("bctbx_wincrypto_random(): CryptGenRandom() failed.");
		return -1;
	}
	return 0;
}
#endif

unsigned int bctbx_random(void){
#ifdef HAVE_ARC4RANDOM
	return arc4random();
#elif  defined(__linux) || defined(__APPLE__)
	static int fd=-1;
	if (fd==-1) fd=open("/dev/urandom",O_RDONLY);
	if (fd!=-1){
		unsigned int tmp;
		if (read(fd,&tmp,4)!=4){
			bctbx_error("Reading /dev/urandom failed.");
		}else return tmp;
	}else bctbx_error("Could not open /dev/urandom");
#elif defined(_WIN32)
	static int initd=0;
	unsigned int ret;
#ifdef _MSC_VER
	/*rand_s() is pretty nice and simple function but is not wrapped by mingw.*/
	
	if (rand_s(&ret)==0){
		return ret;
	}
#else
	if (bctbx_wincrypto_random(&ret)==0){
		return ret;
	}
#endif
	/* Windows's rand() is unsecure but is used as a fallback*/
	if (!initd) {
		struct timeval tv;
		bctbx_gettimeofday(&tv,NULL);
		srand((unsigned int)tv.tv_sec+tv.tv_usec);
		initd=1;
		bctbx_warning("bctoolbox: Random generator is using rand(), this is unsecure !");
	}
	return rand()<<16 | rand();
#endif
	/*fallback to UNIX random()*/
#ifndef _WIN32
	return (unsigned int) random();
#endif
}
bool_t bctbx_is_multicast_addr(const struct sockaddr *addr) {
	
	switch (addr->sa_family) {
		case AF_INET:
			return IN_MULTICAST(ntohl(((struct sockaddr_in *) addr)->sin_addr.s_addr));
		case AF_INET6:
			return IN6_IS_ADDR_MULTICAST(&(((struct sockaddr_in6 *) addr)->sin6_addr));
		default:
			return FALSE;
	}
	
}

#ifdef _WIN32

int bctbx_bind(bctbx_socket_t socket, const struct sockaddr *address, socklen_t address_len) {
	return bind(socket, address, (int)address_len);
}

int bctbx_connect(bctbx_socket_t socket, const struct sockaddr *address, socklen_t address_len) {
	return connect(socket, address, (int)address_len);
}

ssize_t bctbx_send(bctbx_socket_t socket, const void *buffer, size_t length, int flags) {
	return send(socket, (const char *)buffer, (int)length, flags);
}

ssize_t bctbx_sendto(bctbx_socket_t socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len) {
	return sendto(socket, (const char *)message, (int)length, flags, dest_addr, (int)dest_len);
}

ssize_t bctbx_recv(bctbx_socket_t socket, void *buffer, size_t length, int flags) {
	return recv(socket, (char *)buffer, (int)length, flags);
}

ssize_t bctbx_recvfrom(bctbx_socket_t socket, void *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len) {
	return recvfrom(socket, (char *)buffer, (int)length, flags, address, (int *)address_len);
}

ssize_t bctbx_read(int fd, void *buf, size_t nbytes) {
	return (ssize_t)_read(fd, buf, (unsigned int)nbytes);
}

ssize_t bctbx_write(int fd, const void *buf, size_t nbytes) {
	return (ssize_t)_write(fd, buf, (unsigned int)nbytes);
}

#else

int bctbx_bind(bctbx_socket_t socket, const struct sockaddr *address, socklen_t address_len) {
	return bind(socket, address, address_len);
}

int bctbx_connect(bctbx_socket_t socket, const struct sockaddr *address, socklen_t address_len) {
	return connect(socket, address, address_len);
}

ssize_t bctbx_send(bctbx_socket_t socket, const void *buffer, size_t length, int flags) {
	return send(socket, buffer, length, flags);
}

ssize_t bctbx_sendto(bctbx_socket_t socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len) {
	return sendto(socket, message, length, flags, dest_addr, dest_len);
}

ssize_t bctbx_recv(bctbx_socket_t socket, void *buffer, size_t length, int flags) {
	return recv(socket, buffer, length, flags);
}

ssize_t bctbx_recvfrom(bctbx_socket_t socket, void *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len) {
	return recvfrom(socket, buffer, length, flags, address, address_len);
}

ssize_t bctbx_read(int fd, void *buf, size_t nbytes) {
	return read(fd, buf, nbytes);
}

ssize_t bctbx_write(int fd, const void *buf, size_t nbytes) {
	return write(fd, buf, nbytes);
}

#endif


#if defined(ANDROID) || defined(_WIN32)

/*
 * SHAME !!! bionic's getaddrinfo does not implement the AI_V4MAPPED flag !
 * It is declared in header file but rejected by the implementation.
 * The code below is to emulate a _compliant_ getaddrinfo for android.
**/

/**
 * SHAME AGAIN !!! Win32's implementation of getaddrinfo is bogus !
 * it is not able to return an IPv6 addrinfo from an IPv4 address when AI_V4MAPPED is set !
**/

struct addrinfo *_bctbx_alloc_addrinfo(int ai_family, int socktype, int proto){
	struct addrinfo *ai=(struct addrinfo*)bctbx_malloc0(sizeof(struct addrinfo));
	ai->ai_family=ai_family;
	ai->ai_socktype=socktype;
	ai->ai_protocol=proto;
	ai->ai_addrlen=AF_INET6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);
	ai->ai_addr=(struct sockaddr *) bctbx_malloc0(ai->ai_addrlen);
	return ai;
}

struct addrinfo *convert_to_v4mapped(const struct addrinfo *ai){
	struct addrinfo *res=NULL;
	const struct addrinfo *it;
	struct addrinfo *v4m=NULL;
	struct addrinfo *last=NULL;
	
	for (it=ai;it!=NULL;it=it->ai_next){
		struct sockaddr_in6 *sin6;
		struct sockaddr_in *sin;
		v4m=_bctbx_alloc_addrinfo(AF_INET6, it->ai_socktype, it->ai_protocol);
		v4m->ai_flags|=AI_V4MAPPED;
		sin6=(struct sockaddr_in6*)v4m->ai_addr;
		sin=(struct sockaddr_in*)it->ai_addr;
		sin6->sin6_family=AF_INET6;
		((uint8_t*)&sin6->sin6_addr)[10]=0xff;
		((uint8_t*)&sin6->sin6_addr)[11]=0xff;
		memcpy(((uint8_t*)&sin6->sin6_addr)+12,&sin->sin_addr,4);
		sin6->sin6_port=sin->sin_port;
		if (last){
			last->ai_next=v4m;
		}else{
			res=v4m;
		}
		last=v4m;
	}
	return res;
}

struct addrinfo *addrinfo_concat(struct addrinfo *a1, struct addrinfo *a2){
	struct addrinfo *it;
	struct addrinfo *last=NULL;
	for (it=a1;it!=NULL;it=it->ai_next){
		last=it;
	}
	if (last){
		last->ai_next=a2;
		return a1;
	}else
		return a2;
}

int bctbx_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res){
	if (hints && hints->ai_family!=AF_INET && hints->ai_flags & AI_V4MAPPED){
		struct addrinfo *res6=NULL;
		struct addrinfo *res4=NULL;
		struct addrinfo lhints={0};
		int err;
		
		if (hints) memcpy(&lhints,hints,sizeof(lhints));
		
		lhints.ai_flags &= ~(AI_ALL | AI_V4MAPPED); /*remove the unsupported flags*/
		if (hints->ai_flags & AI_ALL){
			lhints.ai_family=AF_INET6;
			err=getaddrinfo(node, service, &lhints, &res6);
		}
		lhints.ai_family=AF_INET;
		err=getaddrinfo(node, service, &lhints, &res4);
		if (err==0){
			struct addrinfo *v4m=convert_to_v4mapped(res4);
			freeaddrinfo(res4);
			res4=v4m;
		}
		*res=addrinfo_concat(res6,res4);
		if (*res) err=0;
		return err;
	}
	return getaddrinfo(node, service, hints, res);
}

void _bctbx_freeaddrinfo(struct addrinfo *res){
	struct addrinfo *it,*next_it;
	for(it=res;it!=NULL;it=next_it){
		next_it=it->ai_next;
		bctbx_free(it->ai_addr);
		bctbx_free(it);
	}
}

void bctbx_freeaddrinfo(struct addrinfo *res){
	struct addrinfo *it,*previt=NULL;
	struct addrinfo *allocated_by_bctbx=NULL;
	for(it=res;it!=NULL;it=it->ai_next){
		if (it->ai_flags & AI_V4MAPPED){
			allocated_by_bctbx=it;
			if (previt) previt->ai_next=NULL;
			break;
		}
		previt=it;
	}
	if (res!=allocated_by_bctbx) freeaddrinfo(res);
	if (allocated_by_bctbx) _bctbx_freeaddrinfo(allocated_by_bctbx);
}

#else

int bctbx_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res){
	int result = getaddrinfo(node, service, hints, res);
#if __APPLE__
	if (*res && (*res)->ai_family == AF_INET6) {
		struct sockaddr_in6* sockaddr = (struct sockaddr_in6*)(*res)->ai_addr;
		if (sockaddr->sin6_port == 0 && service) {
			int possible_port = atoi(service);
			if (possible_port > 0 && possible_port <= 65535) {
				bctbx_message("Apple nat64 getaddrinfo bug, fixing port to [%i]",possible_port);
				sockaddr->sin6_port = htons(possible_port);
			}
			
		}
		
	}
#endif
	return result;

}

void bctbx_freeaddrinfo(struct addrinfo *res){
	freeaddrinfo(res);
}

#endif

static void _bctbx_addrinfo_to_ip_address_error(int err, char *ip, size_t ip_size) {
	bctbx_error("getnameinfo() error: %s", gai_strerror(err));
	strncpy(ip, "<bug!!>", ip_size);
}

int bctbx_addrinfo_to_ip_address(const struct addrinfo *ai, char *ip, size_t ip_size, int *port){
	char serv[16];
	int err=getnameinfo(ai->ai_addr,(socklen_t)ai->ai_addrlen,ip,(socklen_t)ip_size,serv,(socklen_t)sizeof(serv),NI_NUMERICHOST|NI_NUMERICSERV);
	if (err!=0) _bctbx_addrinfo_to_ip_address_error(err, ip, ip_size);
	if (port) *port=atoi(serv);
	return 0;
}

int bctbx_addrinfo_to_printable_ip_address(const struct addrinfo *ai, char *printable_ip, size_t printable_ip_size) {
	char ip[64];
	char serv[16];
	int err = getnameinfo(ai->ai_addr, (socklen_t)ai->ai_addrlen, ip, sizeof(ip), serv, sizeof(serv), NI_NUMERICHOST | NI_NUMERICSERV);
	if (err != 0) _bctbx_addrinfo_to_ip_address_error(err, ip, sizeof(ip));
	if (ai->ai_family == AF_INET)
		snprintf(printable_ip, printable_ip_size, "%s:%s", ip, serv);
	else if (ai->ai_family == AF_INET6)
		snprintf(printable_ip, printable_ip_size, "[%s]:%s", ip, serv);
	return 0;
}

int bctbx_sockaddr_to_ip_address(struct sockaddr *sa, socklen_t salen, char *ip, size_t ip_size, int *port) {
	struct addrinfo ai = { 0 };
	ai.ai_addr = sa;
	ai.ai_addrlen = salen;
	ai.ai_family = sa->sa_family;
	return bctbx_addrinfo_to_ip_address(&ai, ip, ip_size, port);
}

int bctbx_sockaddr_to_printable_ip_address(struct sockaddr *sa, socklen_t salen, char *printable_ip, size_t printable_ip_size) {
	if ((sa->sa_family == 0) || (salen == 0)) {
		snprintf(printable_ip, printable_ip_size, "no-addr");
		return 0;
	} else {
		struct addrinfo ai = { 0 };
		ai.ai_addr = sa;
		ai.ai_addrlen = salen;
		ai.ai_family = sa->sa_family;
		return bctbx_addrinfo_to_printable_ip_address(&ai, printable_ip, printable_ip_size);
	}
}

static struct addrinfo * _bctbx_name_to_addrinfo(int family, int socktype, const char *ipaddress, int port, int numeric_only){
	struct addrinfo *res=NULL;
	struct addrinfo hints={0};
	char serv[10];
	int err;

	snprintf(serv,sizeof(serv),"%i",port);
	hints.ai_family=family;
	if (numeric_only) hints.ai_flags=AI_NUMERICSERV|AI_NUMERICHOST;
	hints.ai_socktype=socktype;
	
	if (family==AF_INET6 && strchr(ipaddress,':')==NULL) {
		hints.ai_flags|=AI_V4MAPPED;
	}
	err=bctbx_getaddrinfo(ipaddress,serv,&hints,&res);

	if (err!=0){
		if (!numeric_only || err!=EAI_NONAME)
			bctbx_error("%s(%s): getaddrinfo failed: %s",__FUNCTION__, ipaddress, gai_strerror(err));
		return NULL;
	}
	return res;
}

struct addrinfo * bctbx_name_to_addrinfo(int family, int socktype, const char *name, int port){
	return _bctbx_name_to_addrinfo(family, socktype, name, port, FALSE);
}

struct addrinfo * bctbx_ip_address_to_addrinfo(int family, int socktype, const char *name, int port){
	struct addrinfo * res = _bctbx_name_to_addrinfo(family, socktype, name, port, TRUE);
#if __APPLE__
	/*required for nat64 on apple platform*/
	if (res) {
		/*fine, we are sure that name was an ip address, give a chance to get its nat64 form*/
		bctbx_freeaddrinfo(res);
		res = bctbx_name_to_addrinfo(family, SOCK_STREAM, name, port);
	}
#endif
	return res;

}

#ifndef IN6_GET_ADDR_V4MAPPED
#define IN6_GET_ADDR_V4MAPPED(sin6_addr)	*(unsigned int*)((unsigned char*)(sin6_addr)+12)
#endif

void bctbx_sockaddr_remove_v4_mapping(const struct sockaddr *v6, struct sockaddr *result, socklen_t *result_len) {
	if (v6->sa_family == AF_INET6) {
		struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)v6;

		if (IN6_IS_ADDR_V4MAPPED(&in6->sin6_addr)) {
			struct sockaddr_in *in = (struct sockaddr_in *)result;
			result->sa_family = AF_INET;
			in->sin_addr.s_addr = IN6_GET_ADDR_V4MAPPED(&in6->sin6_addr);
			in->sin_port = in6->sin6_port;
			*result_len = sizeof(struct sockaddr_in);
		} else {
			if (v6 != result) memcpy(result, v6, sizeof(struct sockaddr_in6));
			*result_len = sizeof(struct sockaddr_in6);
		}
	} else {
		*result_len = sizeof(struct sockaddr_in);
		if (v6 != result) memcpy(result, v6, sizeof(struct sockaddr_in));
	}
}

char * bctbx_concat(const char *str, ...) {
	va_list ap;
	size_t allocated = 100;
	char *result = (char *) malloc (allocated);
	
	if (result != NULL)
	{
		char *newp;
		char *wp;
		const char* s;
		
		va_start (ap, str);
		
		wp = result;
		for (s = str; s != NULL; s = va_arg (ap, const char *)) {
			size_t len = strlen (s);
			
			/* Resize the allocated memory if necessary.  */
			if (wp + len + 1 > result + allocated)
			{
				allocated = (allocated + len) * 2;
				newp = (char *) realloc (result, allocated);
				if (newp == NULL)
				{
					free (result);
					return NULL;
				}
				wp = newp + (wp - result);
				result = newp;
			}
			memcpy (wp, s, len);
			wp +=len;
		}
		
		/* Terminate the result string.  */
		*wp++ = '\0';
		
		/* Resize memory to the optimal size.  */
		newp = realloc (result, wp - result);
		if (newp != NULL)
			result = newp;
		
		va_end (ap);
	}
	
	return result;
}

bool_t bctbx_sockaddr_equals(const struct sockaddr * sa, const struct sockaddr * sb) {
	
	if (sa->sa_family != sb->sa_family)
		return FALSE;
	
	if (sa->sa_family == AF_INET) {
		if ((((struct sockaddr_in*)sa)->sin_addr.s_addr != ((struct sockaddr_in*)sb)->sin_addr.s_addr
			 || ((struct sockaddr_in*)sa)->sin_port != ((struct sockaddr_in*)sb)->sin_port))
			return FALSE;
	} else if (sa->sa_family == AF_INET6) {
		if (memcmp(&((struct sockaddr_in6*)sa)->sin6_addr
				   , &((struct sockaddr_in6*)sb)->sin6_addr
				   , sizeof(struct in6_addr)) !=0
			|| ((struct sockaddr_in6*)sa)->sin6_port != ((struct sockaddr_in6*)sb)->sin6_port)
			return FALSE;
	} else {
		bctbx_warning ("Cannot compare family type [%d]", sa->sa_family);
		return FALSE;
	}
	return TRUE;
	
}
