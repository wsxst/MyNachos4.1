/* syscalls.h 
 * 	Nachos system call interface.  These are the enveloped Nachos kernel 
 *      operations that can be invoked from user programs.
 *      Each NachOS system call is translated to an apropriate LIBC call. 
 *      Hopefully this works on MacOS X, *nix and Windows
 */

#ifndef NACHOS_SYSCALLS_H
#define NACHOS_SYSCALLS_H

#include "errno.h"

/*
 * The system call interface.  These are the operations the Nachos
 * kernel needs to support, to be able to run user programs.
 */

/* Stop Nachos, and print out performance stats */
void Halt();
 
 
/*
 * Add the two operants and return the result
 */ 
int Add(int op1, int op2);

/* This user program is done (status = 0 means exited normally). */
void Exit(int status);

/* A unique identifier for an executing user program (address space) */
typedef unsigned int SpaceId;	

/* A unique identifier for a thread within a task */
typedef unsigned int ThreadId;

/* Address space control operations: Exit, Exec, Execv, and Join */

/* Run the specified executable, with no args */
/* This can be implemented as a call to ExecV.
 */ 
SpaceId Exec(char* exec_name);

/* Run the executable, stored in the Nachos file "argv[0]", with
 * parameters stored in argv[1..argc-1] and return the 
 * address space identifier
 */
SpaceId ExecV(int argc, char* argv[]);
 
/* Only return once the user program "id" has finished.  
 * Return the exit status.
 */
int Join(SpaceId id);
 
/* File system operations: Create, Remove, Open, Read, Write, Close
 * These functions are patterned after UNIX -- files represent
 * both files *and* hardware I/O devices.
 *
 * Note that the Nachos file system has a stub implementation, which
 * can be used to support these system calls if the regular Nachos
 * file system has not been implemented.
 */
 
/* A unique identifier for an open Nachos file. */
typedef int OpenFileId;

/* when an address space starts up, it has two open files, representing 
 * keyboard input and display output (in UNIX terms, stdin and stdout).
 * Read and Write can be used directly on these, without first opening
 * the console device.
 */

#define ConsoleInput	0  
#define ConsoleOutput	1  
 
/* Create a Nachos file, with name "name" */
/* Note: Create does not open the file.   */
/* Return 1 on success, negative error code on failure */
int Create(char *name);

/* Remove a Nachos file, with name "name" */
int Delete(char *name);

/* 
 * Open the Nachos file "name", and return an "OpenFileId" that can 
 * be used to read and write to the file. "mode" gives the requested 
 * operation mode for this file.
 */
#define RO 1
#define RW 2
#define APPEND 3
OpenFileId Open(char *name, int mode);

/* 
 * Write "size" bytes from "buffer" to the open file. 
 * Return the number of bytes actually read on success.
 * On failure, a negative error code is returned.
 */
int Write(char *buffer, int size, OpenFileId id);

/* 
 * Read "size" bytes from the open file into "buffer".  
 * Return the number of bytes actually read -- if the open file isn't
 * long enough, or if it is an I/O device, and there aren't enough 
 * characters to read, return whatever is available (for I/O devices, 
 * you should always wait until you can return at least one character).
 */
int Read(char *buffer, int size, OpenFileId id);

/* 
 * Set the seek position of the open file "id"
 * to the byte "position".
 */
int Seek(int position, OpenFileId id);

/* 
 * Deletes a file with the filename given by "name".
 * An error is returned if file does not exist or other wicked things happen.
 */
int Delete(char* name);

/* Close the file, we're done reading and writing to it.
 * Return 1 on success, negative error code on failure
 */
int Close(OpenFileId id);

/* User-level thread operations: Fork and Yield.  To allow multiple
 * threads to run within a user program. 
 *
 * Could define other operations, such as LockAcquire, LockRelease, etc.
 */

/* Fork a thread to run a procedure ("func") in the *same* address space 
 * as the current thread.
 * Return a positive ThreadId on success, negative error code on failure
 */

void ForkHelper(void (*func)());

ThreadId ThreadFork(void (*func)());

/* Yield the CPU to another runnable thread, whether in this address space 
 * or not. 
 */
void ThreadYield();

/*
 * Blocks current thread until lokal thread ThreadID exits with ThreadExit.
 */
int ThreadJoin(ThreadId id);

/*
 * Deletes current thread and returns ExitCode to every waiting lokal thread.
 */
void ThreadExit(int ExitCode);

/*
 * Returns SpaceId of current address space.
 */
SpaceId getSpaceID();

/*
 * Returns ThreadId of current thread. 
 * 
 * In *nix or *doze this is not obvious. But it's sufficient for the start.
 */
ThreadId getThreadID();

#endif /* NACHOS_SYSCALL_H */

