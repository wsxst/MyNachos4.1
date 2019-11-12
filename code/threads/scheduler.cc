// scheduler.cc
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would
//	end up calling FindNextToRun(), and that would put us in an
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------

int cmp(Thread* t1,Thread* t2)
{
    if(t1->getPriority()<t2->getPriority()) return -1;
    else if(t1->getPriority()>t2->getPriority()) return 1;
    return 0;
}

Scheduler::Scheduler()
{
    //FIFO
    readyList = new List<Thread *>;
    //抢占式优先级
    sortedReadyList = new SortedList<Thread *>(cmp);
    //多级反馈队列
    for(int i=0;i<QueueNum;++i) threadArrQueue[i]=new List<Thread*>;

    toBeDestroyed = NULL;
}

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{
    delete readyList;
    delete sortedReadyList;
}

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void Scheduler::ReadyToRun(Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);//判断是否已经关中断
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());

    thread->setStatus(READY);//将该线程状态设置为就绪态
    if(typeno==1) sortedReadyList->Insert(thread);//抢占式优先级
    else if(typeno==0||typeno==2)
    {
        if(typeno==2) thread->setRemainTime(timeSlice);
        readyList->Append(thread);//FIFO
    }
    else if(typeno==3)
    {
        if(thread->getPriority()!=QueueNum-1)
        {
            thread->setPriority(thread->getPriority()+1);
        }
        thread->setRemainTime(threadArrTimeSlice[thread->getPriority()]);
        threadArrQueue[thread->getPriority()]->Append(thread);
        cerr<<thread->getName()<<"进入第"<<thread->getPriority()<<"级队列,时间片为:"<<thread->getRemainTime()<<endl;
    }
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread* Scheduler::FindNextToRun()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    // if(debug->IsEnabled('t')) kernel->TS();
    if(typeno==0)
    {
        if (!readyList->IsEmpty())
        {
            // Print();
            if(debug->IsEnabled('t')) cerr<<"从就绪队列中选出线程："<<readyList->Front()->getName()<<endl;
            return readyList->RemoveFront();
        }
    }
    else if(typeno==1)
    {
        if (!sortedReadyList->IsEmpty())
        {
            // Print();
            if(debug->IsEnabled('t')) cerr<<"从就绪队列中选出线程："<<sortedReadyList->Front()->getName()<<"；优先级："<<sortedReadyList->Front()->getPriority()<<endl;
            return sortedReadyList->RemoveFront();
        }
    }
    else if(typeno==2)
    {
        if (!readyList->IsEmpty())
        {
            // Print();
            if(debug->IsEnabled('t')) cerr<<"从就绪队列中选出线程："<<readyList->Front()->getName()<<";剩余时间片:"<<readyList->Front()->getRemainTime()<<endl;
            return readyList->RemoveFront();
        }
    }
    else if(typeno==3)
    {
        // Print();
        for(int i=0;i<QueueNum;++i)
        {
            if(!threadArrQueue[i]->IsEmpty())
            {
                if(debug->IsEnabled('t')) cerr<<"从就绪队列中选出线程："<<threadArrQueue[i]->Front()->getName()<<";所在队列:"<<i<<";剩余时间片:"<<threadArrQueue[i]->Front()->getRemainTime()<<endl;
                return threadArrQueue[i]->RemoveFront();
            }
        }
    }
    
    return NULL;
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void Scheduler::Run(Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;

    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing)
    { // mark that we need to delete current thread
        ASSERT(toBeDestroyed == NULL);
        toBeDestroyed = oldThread;
    }

    if (oldThread->space != NULL)
    {                               // if this thread is a user program,
        oldThread->SaveUserState(); // save the user's CPU registers
        oldThread->space->SaveState();
    }

    oldThread->CheckOverflow(); // check if the old thread
                                // had an undetected stack overflow

    kernel->currentThread = nextThread; // switch to the next thread
    nextThread->setStatus(RUNNING);     // nextThread is now running

    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());

    // This is a machine-dependent assembly language routine defined
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);

    // we're back, running oldThread

    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed(); // check if thread we were running
                          // before this one has finished
                          // and needs to be cleaned up

    if (oldThread->space != NULL)
    {                                  // if there is an address space
        oldThread->RestoreUserState(); // to restore, do it.
        oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL)
    {
        delete toBeDestroyed;
        toBeDestroyed = NULL;
    }
}

//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void Scheduler::Print()
{
    cerr << "当前的就绪队列：\n";
    if(typeno==0||typeno==2) readyList->Apply(ThreadPrint);
    else if(typeno==1) sortedReadyList->Apply(ThreadPrint);
    else if(typeno==3)
    {
        for(int i=0;i<QueueNum;++i)
        {
            cerr<<"第"<<i<<"级队列:\n";
            threadArrQueue[i]->Apply(ThreadPrint);
        }
    }
}

bool Scheduler::isReadyListEmpty()
{
    if(typeno==0||typeno==2) return readyList->IsEmpty();
    else if(typeno==1) return sortedReadyList->IsEmpty();
    else if(typeno==3)
    {
        for(int i=0;i<QueueNum;++i)
        {
            if(!threadArrQueue[i]->IsEmpty()) return false;
        }
    }
    return true;
}

Thread* Scheduler::getReadyListFront()
{
    if(typeno==0||typeno==2)
    {
        if (!readyList->IsEmpty()) return readyList->Front();
    }
    else if(typeno==1)
    {
        if (!sortedReadyList->IsEmpty()) return sortedReadyList->Front();
    }
    else if(typeno==3)
    {
        for(int i=0;i<QueueNum;++i)
        {
            if(!threadArrQueue[i]->IsEmpty()) return threadArrQueue[i]->Front();
        }
    }
    return NULL;
}