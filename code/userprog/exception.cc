// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h"
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions
//	is in machine.h.
//----------------------------------------------------------------------

void ExceptionHandler(ExceptionType which)
{
	int type = kernel->machine->ReadRegister(2);
	int vaddr, vpn, offset;

	DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");

	if(which == SyscallException)
	{
		cerr<<"system call type:"<<type<<endl;
		switch (type)
		{
		case SC_Halt:
			DEBUG(dbgSys, "Shutdown, initiated by user program.\n");

			SysHalt();

			ASSERTNOTREACHED();
			break;
		
		case SC_Exit:
			if(debug->IsEnabled('s')) cerr<<"Current thread "<<kernel->currentThread->getName()<<" exit!\n";
#ifdef USE_RPT
			for(int i=0;i<NumPhysPages;++i)
			{
				if(kernel->machine->pt[i].tID == kernel->currentThread->getTID())
				{
					kernel->machine->mmBitmap->Clear(i);
					kernel->machine->pt[i].reset();
				}
			}
			if(kernel->machine->tlb != NULL)
			{
				for(int i=0;i<TLBSize;++i)
				{
					if(kernel->machine->tlb[i].tID == kernel->currentThread->getTID())
						kernel->machine->tlb[i].reset();
				}
			}
#else
			if(kernel->currentThread->space != NULL)
			{
				kernel->fileSystem->Remove(kernel->currentThread->space->getVMFileName());
			}
			if(kernel->machine->tlb != NULL)
			{
				for(int i=0;i<TLBSize;++i) kernel->machine->tlb[i].reset();
			}
#endif
			kernel->currentThread->Finish();
			return;
			break;

		case SC_Add:
			DEBUG(dbgSys, "Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");

			/* Process SysAdd Systemcall*/
			int result;
			result = SysAdd(/* int op1 */ (int)kernel->machine->ReadRegister(4),
							/* int op2 */ (int)kernel->machine->ReadRegister(5));

			DEBUG(dbgSys, "Add returning with " << result << "\n");
			/* Prepare Result */
			kernel->machine->WriteRegister(2, (int)result);

			/* Modify return point */
			{
				/* set previous programm counter (debugging only)*/
				kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

				/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
				kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

				/* set next programm counter for brach execution */
				kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
			}

			return;

			ASSERTNOTREACHED();

			break;

		default:
			cerr << "Unexpected system call " << type << "\n";
			break;
		}
	}
	else if(which == PageFaultException)
	{
		++(kernel->stats->numPageFaults);
		vaddr = kernel->machine->ReadRegister(BadVAddrReg);
		vpn = vaddr / PageSize;
		offset = vaddr % PageSize;
		if(debug->IsEnabled('a')) cerr<<"vpn:"<<vpn<<";vpo:"<<offset<<endl;
		int avaiPageFrame = kernel->machine->findAvailablePageFrame();
		if(avaiPageFrame == -1)
		{
			DEBUG(dbgAddr,"No physical page frames in main memory available now !");
			avaiPageFrame = kernel->machine->findOneToReplace(kernel->machine->pt, 0);
		}
		if(debug->IsEnabled('a'))
		{
			cerr<<"Load available page frame #"<<avaiPageFrame<<" into main memory!"<<endl;
		}
		kernel->machine->loadPageFrame(vpn, avaiPageFrame);
		return;
	}
	else if(which == TLBMissException)
	{
		++(kernel->stats->numTLBMiss);
		vaddr = kernel->machine->ReadRegister(BadVAddrReg);
		vpn = vaddr / PageSize;
		offset = vaddr % PageSize;
		int ppn = -1;
		if(debug->IsEnabled('a')) cerr<<"vpn:"<<vpn<<";vpo:"<<offset<<endl;
		TranslationEntry entry;
		ExceptionType e = kernel->machine->pageTableTranslation(vpn ,ppn, entry, vaddr);
		if(e!=NoException)
		{
			ExceptionHandler(e);
			return;
		}
		kernel->machine->updateTLB(kernel->machine->tlb, entry);
		return;
	}
	else
	{
		cerr << "Unexpected user mode exception" << (int)which << "\n";
	}
	ASSERTNOTREACHED();
}
