/* syscalls.c
 * 	Nachos system call interface.  These are the enveloped Nachos kernel 
 *      operations that can be invoked from user programs.
 *      Each NachOS system call is translated to an apropriate LIBC call. 
 *      Hopefully this works on MacOS X, *nix and Windows
 */


#include "nachos_syscall.h"

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/file.h>
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sched.h>

#include <signal.h>
#include <sys/types.h>
#include <pthread.h>



#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define SHELL "/bin/sh"

/*
 * The system call interface.  These are the operations the Nachos
 * kernel needs to support, to be able to run user programs.
 */

/* Stop Nachos, and print out performance stats */
void Halt()
{
	Exit(0);
}
 
 
/*
 * Add the two operants and return the result
 */ 

int Add(int op1, int op2)
{ 
	return op1 + op2;
}


/* This user program is done (status = 0 means exited normally). */
void Exit(int status)
{
	exit(status);
}

/* Address space control operations: Exit, Exec, Execv, and Join */

/* Run the specified executable, with no args */
/* This can be implemented as a call to ExecV.
 */ 
SpaceId Exec(char* exec_name)
{
    pid_t child;
    child = vfork();
    if(child == 0)
    {
	execl (SHELL, SHELL, "-c", exec_name, NULL);
	_exit (EXIT_FAILURE);
    }
    else if(child < 0) 
	return EPERM;
    return (SpaceId) child;
}


/* 
 * Run the executable, stored in the Nachos file "argv[0]", with
 * parameters stored in argv[1..argc-1] and return the 
 * address space identifier
 * For this, the incoming string has to be seperated by replacing " " 
 * with "\n" and building the appropriate pointer structure argv.
 */
SpaceId ExecV(int argc, char* argv[])
{
    pid_t child;
    child = vfork();
    if(child == 0){
	execl (SHELL, SHELL, "-c", argv, NULL);
	_exit (EXIT_FAILURE);
    }
    else if(child < 0) 
	return EPERM;
    return (SpaceId) child;
}

/* Only return once the user program "id" has finished.  
 * Return the exit status.
 */
int Join(SpaceId id)
{
    return waitpid((pid_t) id, (int*) 0, 0);
}
 

/* File system operations: Create, Remove, Open, Read, Write, Close
 * These functions are patterned after UNIX -- files represent
 * both files *and* hardware I/O devices.
 *
 * Note that the Nachos file system has a stub implementation, which
 * can be used to support these system calls if the regular Nachos
 * file system has not been implemented.
 */
 

/* when an address space starts up, it has two open files, representing 
 * keyboard input and display output (in UNIX terms, stdin and stdout).
 * Read and Write can be used directly on these, without first opening
 * the console device.
 */

/* Create a Nachos file, with name "name" */
/* Note: Create does not open the file.   */
/* Return 1 on success, negative error code on failure */
int Create(char *name)
{
    int fd;
    fd=open(name, O_TRUNC | O_CREAT |  O_WRONLY);
    if (fd>0) {
	close(fd);
	return 1;
    }else return fd;
}

/* Remove a Nachos file, with name "name" */
int Remove(char *name)
{
    return remove(name);
}

/* 
 * Open the Nachos file "name", and return an "OpenFileId" that can 
 * be used to read and write to the file. "mode" gives the requested 
 * operation mode for this file.
 */

OpenFileId Open(char *name, int mode)
{
    int pmode;
    switch(mode){
	case RO: pmode=O_RDONLY;
	    break;
	case RW: pmode=O_RDWR;
	    break;
	case APPEND:
	    pmode=O_APPEND;
	    break;
	default: return -1;
    }
    return open(name, pmode);
}

/* 
 * Write "size" bytes from "buffer" to the open file. 
 * Return the number of bytes actually read on success.
 * On failure, a negative error code is returned.
 */
int Write(char *buffer, int size, OpenFileId id)
{
    return write(id, buffer, (size_t) size);
}

/* 
 * Read "size" bytes from the open file into "buffer".  
 * Return the number of bytes actually read -- if the open file isn't
 * long enough, or if it is an I/O device, and there aren't enough 
 * characters to read, return whatever is available (for I/O devices, 
 * you should always wait until you can return at least one character).
 */
int Read(char *buffer, int size, OpenFileId id)
{
    return read(id, buffer, (size_t) size);
}

/* 
 * Set the seek position of the open file "id"
 * to the byte "position".
 */
int Seek(int position, OpenFileId id)
{
    return (int) lseek(id, SEEK_SET, position);
}

/* 
 * Deletes a file with the filename given by "name".
 * An error is returned if file does not exist or other wicked things happen.
 */
int Delete(char* name)
{
    return unlink(name);
}

/* Close the file, we're done reading and writing to it.
 * Return 1 on success, negative error code on failure
 */
int Close(OpenFileId id)
{
    return close(id);
}


/*
 * User-level thread operations: Fork and Yield.  To allow multiple
 * threads to run within a user program. 
 *
 * Could define other operations, such as LockAcquire, LockRelease, etc.
 */

/* Fork a thread to run a procedure ("func") in the *same* address space 
 * as the current thread.
 * Return a positive ThreadId on success, negative error code on failure
 */

void ForkHelper(void (*func)())
{
    /* untested */
    func();
}

ThreadId ThreadFork(void (*func)())
{
#ifdef HAVE_LIBPTHREAD
   /*
    * This malloc is a memory leak! Small but I can't help it.
    * As it's to complicated to clean up after this (multiple threads
    * could be waiting on it...)
    */
    pthread_t * thread=malloc(sizeof(pthread_t));
    pthread_create(thread ,NULL, (void*) &ForkHelper, func);
    return (ThreadId) thread;
#else 
#warning pthreads not supported on this platform
    printf("pthreads not supported on this platform\n");
#endif
}

/* Yield the CPU to another runnable thread, whether in this address space 
 * or not. 
 */
void ThreadYield()
{
#ifdef HAVE_LIBPTHREAD
    sched_yield();
#else
#warning pthreads not supported on this platform
    printf("pthreads not supported on this platform\n");
#endif
}

/*
 * Blocks current thread until lokal thread ThreadID exits with ThreadExit.
 */
int ThreadJoin(ThreadId id)
{
#ifdef HAVE_LIBPTHREAD
    pthread_join( ((pthread_t) id) , NULL);
#else
#warning pthreads not supported on this platform
    printf("pthreads not supported on this platform\n");
#endif
}

/*
 * Deletes current thread and returns ExitCode to every waiting lokal thread.
 */
void ThreadExit(int ExitCode)
{
#ifdef HAVE_LIBPTHREAD
    pthread_exit(NULL);
#else
#warning pthreads not supported on this platform
    printf("pthreads not supported on this platform\n");
#endif
}

/*
 * Returns SpaceId of current address space.
 */
SpaceId getSpaceID()
{
    return (SpaceId) getpid();
}

/*
 * Returns ThreadId of current thread. 
 * 
 * In *nix or *doze this is not obvious. But it's sufficient for the
 * start in the simulated environment.
 */
ThreadId getThreadID()
{
    return (ThreadId) getpid();
}

