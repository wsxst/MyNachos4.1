Exercises:
 #0 Look into the "shell" sub directory. Everything is in there.
    You will have to come up with a small library with nice little
    helpers (like printf, strncmp, strcmp, atoi, etc.) and some shell
    programs (like ls/dir, cat/type, etc.). On a Windows platform you
    may have to change the SHELL variable in nachos_syscall.c.
    Exec is different in NachOS!
    In here Exec will take all parameters spin of a <real> shell and
    execute it. This will be different in NachOS! For parameters use ExecV!

 #1 Look into the "threads" sub directory. Everything is in there.
    The synchronization mechanisms have to be implemented in
    "synch.cc" and "synch.h".
    To start your tests just hack in another shell parameter in
    kernel.cc. Define your little test suite via a compiler parameter,
    so you can omit it later on.
 
 #2 The system calls are implemented in "userprog/exception.cc" and
    "userprog/ksyscall.h". You have use the functions "ReadMem" and
    "WriteMem" of "machine/machine.*" to access the simulated main memory.
    You'll have to build the coff2noff tool in order to compile the test
    programs.
 
 #3 Your system calls may have to be rewritten! Most stuff is due in
    "userprog/addrspace.*", "userprog/exception" and 
 #4 Mainly done in the filesystem subdir. Be aware of locking problems!
 #5 You're on your own, pal! Don't touch the machine dir!

Building instructions for NachOS-SHELL:
 * go to the directory shell
 * execute the local configure script
 * run make
 
Building Instructions for NachOS:
 * go to the directory build.<host>, where <host> is your working OS
 * do a "make depend" to build depenencies (everytime you change the makefile!)
 * do a "make" to build NachOS

Usage:
see "nachos -u" for all command line options

Building and starting user-level programs in NachOS:
 * use Mips cross-compiler to build and link coff-binaries
 * use coff2noff to translate the binaries to the NachOS-format
 * start binary with nachos -x <path_to_file/file>
