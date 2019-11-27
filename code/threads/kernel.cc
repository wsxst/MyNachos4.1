// kernel.cc
//	Initialization and cleanup routines for the Nachos kernel.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "main.h"
#include "kernel.h"
#include "sysdep.h"
#include "synch.h"
#include "synchlist.h"
#include "libtest.h"
#include "string.h"
#include "synchconsole.h"
#include "synchdisk.h"
#include "post.h"

#define MAX_PRODUCE_ARRAY_NUM 50
Semaphore *isFull,*isEmpty;
Lock* produceArrayLock;
int high,low;
int produceArray[MAX_PRODUCE_ARRAY_NUM];
#define PRODUCER_NUM 1
#define CONSUMER_NUM 2
#define PRODUCE_TIMES 50
#define CONSUME_TIMES 25
SynchList<int> *produceList;
Condition* barrier;
Lock* barrierMutex;
#define TEST_THREAD_NUM 5
Lock *wLock,*rwMutex;
#define READER_NUM 5
#define WRITER_NUM 3
#define READ_CONTENT_SIZE 100
int readerCount;
int readContent[READ_CONTENT_SIZE]={0};

//----------------------------------------------------------------------
// Kernel::Kernel
// 	Interpret command line arguments in order to determine flags
//	for the initialization (see also comments in main.cc)
//----------------------------------------------------------------------

Kernel::Kernel(int argc, char **argv)
{
    randomSlice = FALSE;
    debugUserProg = FALSE;
    consoleIn = NULL;  // default is stdin
    consoleOut = NULL; // default is stdout
    
#ifndef FILESYS_STUB
    formatFlag = FALSE;
#endif
    reliability = 1; // network reliability, default is 1.0
    hostName = 0;    // machine id, also UNIX socket name
                     // 0 is the default machine id
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-rs") == 0)
        {
            ASSERT(i + 1 < argc);
            RandomInit(atoi(argv[i + 1])); // initialize pseudo-random
                // number generator
            randomSlice = TRUE;
            i++;
        }
        else if (strcmp(argv[i], "-s") == 0)
        {
            debugUserProg = TRUE;
        }
        else if (strcmp(argv[i], "-ci") == 0)
        {
            ASSERT(i + 1 < argc);
            consoleIn = argv[i + 1];
            i++;
        }
        else if (strcmp(argv[i], "-co") == 0)
        {
            ASSERT(i + 1 < argc);
            consoleOut = argv[i + 1];
            i++;
#ifndef FILESYS_STUB
        }
        else if (strcmp(argv[i], "-f") == 0)
        {
            formatFlag = TRUE;
#endif
        }
        else if (strcmp(argv[i], "-n") == 0)
        {
            ASSERT(i + 1 < argc); // next argument is float
            reliability = atof(argv[i + 1]);
            i++;
        }
        else if (strcmp(argv[i], "-m") == 0)
        {
            ASSERT(i + 1 < argc); // next argument is int
            hostName = atoi(argv[i + 1]);
            i++;
        }
        else if (strcmp(argv[i], "-u") == 0)
        {
            cout << "Partial usage: nachos [-rs randomSeed]\n";
            cout << "Partial usage: nachos [-s]\n";
            cout << "Partial usage: nachos [-ci consoleIn] [-co consoleOut]\n";
#ifndef FILESYS_STUB
            cout << "Partial usage: nachos [-nf]\n";
#endif
            cout << "Partial usage: nachos [-n #] [-m #]\n";
        }
    }
}

//----------------------------------------------------------------------
// Kernel::Initialize
// 	Initialize Nachos global data structures.  Separate from the
//	constructor because some of these refer to earlier initialized
//	data via the "kernel" global variable.
//----------------------------------------------------------------------

void Kernel::Initialize()
{
    // We didn't explicitly allocate the current thread we are running in.
    // But if it ever tries to give up the CPU, we better have a Thread
    // object to save its state.
    memset(threadArray,0,sizeof(threadArray));
    currentThread = new Thread("main");
    if(typeno==1) currentThread->setPriority(8);
    currentThread->setStatus(RUNNING);

    stats = new Statistics();       // collect statistics
    interrupt = new Interrupt;      // start up interrupt handling
    scheduler = new Scheduler();    // initialize the ready queue
    alarm = new Alarm(randomSlice); // start up time slicing
    machine = new Machine(debugUserProg);
    synchConsoleIn = new SynchConsoleInput(consoleIn);    // input from stdin
    synchConsoleOut = new SynchConsoleOutput(consoleOut); // output to stdout
    synchDisk = new SynchDisk();                          //
#ifdef FILESYS_STUB
    fileSystem = new FileSystem();
#else
    fileSystem = new FileSystem(formatFlag);
#endif // FILESYS_STUB
    // postOfficeIn = new PostOfficeInput(10);
    // postOfficeOut = new PostOfficeOutput(reliability);

    interrupt->Enable();
}

//----------------------------------------------------------------------
// Kernel::~Kernel
// 	Nachos is halting.  De-allocate global data structures.
//----------------------------------------------------------------------

Kernel::~Kernel()
{
    delete stats;
    delete interrupt;
    delete scheduler;
    delete alarm;
    delete machine;
    delete synchConsoleIn;
    delete synchConsoleOut;
    delete synchDisk;
    delete fileSystem;
    // delete postOfficeIn;
    // delete postOfficeOut;
    Exit(0);
}

//----------------------------------------------------------------------
// Kernel::ThreadSelfTest
//      Test threads, semaphores, synchlists
//----------------------------------------------------------------------

void Kernel::ThreadSelfTest()
{
   currentThread->MyThreadTest();
}

//----------------------------------------------------------------------
// Kernel::ConsoleTest
//      Test the synchconsole
//----------------------------------------------------------------------

void Kernel::ConsoleTest()
{
    char ch;

    cout << "Testing the console device.\n"
         << "Typed characters will be echoed, until ^D is typed.\n"
         << "Note newlines are needed to flush input through UNIX.\n";
    cout.flush();

    do
    {
        ch = synchConsoleIn->GetChar();
        // cout<<"我获取到字符了："<<ch;
        if (ch != EOF)
            synchConsoleOut->PutChar(ch); // echo it!
    } while (ch != EOF);

    cout << "\n";
}

//----------------------------------------------------------------------
// Kernel::NetworkTest
//      Test whether the post office is working. On machines #0 and #1, do:
//
//      1. send a message to the other machine at mail box #0
//      2. wait for the other machine's message to arrive (in our mailbox #0)
//      3. send an acknowledgment for the other machine's message
//      4. wait for an acknowledgement from the other machine to our
//          original message
//
//  This test works best if each Nachos machine has its own window
//----------------------------------------------------------------------

void Kernel::NetworkTest()
{
    if (hostName == 0 || hostName == 1)
    {
        // if we're machine 1, send to 0 and vice versa
        int farHost = (hostName == 0 ? 1 : 0);
        PacketHeader outPktHdr, inPktHdr;
        MailHeader outMailHdr, inMailHdr;
        char *data = "Hello there!";
        char *ack = "Got it!";
        char buffer[MaxMailSize];

        // construct packet, mail header for original message
        // To: destination machine, mailbox 0
        // From: our machine, reply to: mailbox 1
        outPktHdr.to = farHost;
        outMailHdr.to = 0;
        outMailHdr.from = 1;
        outMailHdr.length = strlen(data) + 1;

        // Send the first message
        postOfficeOut->Send(outPktHdr, outMailHdr, data);

        // Wait for the first message from the other machine
        postOfficeIn->Receive(0, &inPktHdr, &inMailHdr, buffer);
        cout << "Got: " << buffer << " : from " << inPktHdr.from << ", box "
             << inMailHdr.from << "\n";
        cout.flush();

        // Send acknowledgement to the other machine (using "reply to" mailbox
        // in the message that just arrived
        outPktHdr.to = inPktHdr.from;
        outMailHdr.to = inMailHdr.from;
        outMailHdr.length = strlen(ack) + 1;
        postOfficeOut->Send(outPktHdr, outMailHdr, ack);

        // Wait for the ack from the other machine to the first message we sent
        postOfficeIn->Receive(1, &inPktHdr, &inMailHdr, buffer);
        cout << "Got: " << buffer << " : from " << inPktHdr.from << ", box "
             << inMailHdr.from << "\n";
        cout.flush();
    }

    // Then we're done!
}

void Kernel::TS()
{
    cerr<<"当前全部线程状态：\n";
    cerr<<"线程ID\t线程名称\t拥有者\t线程状态\t优先级\n";
    for(int i=0;i<MaxThreadNum;++i)
    {
        if(kernel->threadArray[i]) kernel->threadArray[i]->Print();
    }
}

void static Produce(int which)
{
    for(int i=0;i<PRODUCE_TIMES;++i)
    {
        isEmpty->P();
        produceArrayLock->Acquire();
        produceArray[high++] = i;
        if(debug->IsEnabled('s')) cerr<<"producer"<<which<<" produces: "<<i<<", now high is "<<high<<". Now number in list is: "<<high-low-1<<endl;
        produceArrayLock->Release();
        isFull->V();
    }
}

void static Consume(int which)
{
    for(int i=0;i<CONSUME_TIMES;++i)
    {
        isFull->P();
        produceArrayLock->Acquire();
        if(debug->IsEnabled('s')) cerr<<"consumer"<<which<<" consumes: "<<produceArray[++low]<<", now low is "<<low<<". Now number in list is: "<<high-low-2<<endl;
        produceArrayLock->Release();
        isEmpty->V();
    }
}

static void Produce1(int which)
{
    for(int i=0;i<PRODUCE_TIMES;++i)
    {
        produceList->Append(i);
        if(debug->IsEnabled('s')) cerr<<"producer"<<which<<" produces: "<<i<<endl;
    }
}

static void Consume1(int which)
{
    for(int i=0;i<CONSUME_TIMES;++i)
    {
        if(debug->IsEnabled('s')) cerr<<"consumer"<<which<<" consumes: "<<produceList->RemoveFront()<<endl;
    }
}

void MyProducerConsumer1()
{
    isFull = new Semaphore("isFull",0);
    isEmpty = new Semaphore("isEmpty",MAX_PRODUCE_ARRAY_NUM);
    produceArrayLock = new Lock("produceArrayLock");
    high = 0;
    low = -1;
    memset(produceArray,-1,sizeof(produceArray));
    Thread* producers[PRODUCER_NUM];
    Thread* consumers[CONSUMER_NUM];
    char tname[PRODUCER_NUM+CONSUMER_NUM][20];
    int tnn=0;
    for(int i=0;i<PRODUCER_NUM;++i)
    {
        sprintf(tname[tnn],"Producer %d",i);
        producers[i] = new Thread(tname[tnn]);
        producers[i]->Fork((VoidFunctionPtr)Produce,(void*)i);
        ++tnn;
    }
    for(int i=0;i<CONSUMER_NUM;++i)
    {
        sprintf(tname[tnn],"Consumer %d",i);
        consumers[i] = new Thread(tname[tnn]);
        consumers[i]->Fork((VoidFunctionPtr)Consume,(void*)i);
        ++tnn;
    }
    while(!kernel->scheduler->isReadyListEmpty()) kernel->currentThread->Yield();
}

void MyProducerConsumer2()
{
    produceList = new SynchList<int>;
    Thread* producers[PRODUCER_NUM];
    Thread* consumers[CONSUMER_NUM];
    char tname[PRODUCER_NUM+CONSUMER_NUM][20];
    int tnn=0;
    for(int i=0;i<PRODUCER_NUM;++i)
    {
        sprintf(tname[tnn],"Producer %d",i);
        producers[i] = new Thread(tname[tnn]);
        producers[i]->Fork((VoidFunctionPtr)Produce1,(void*)i);
        ++tnn;
    }
    for(int i=0;i<CONSUMER_NUM;++i)
    {
        sprintf(tname[tnn],"Consumer %d",i);
        consumers[i] = new Thread(tname[tnn]);
        consumers[i]->Fork((VoidFunctionPtr)Consume1,(void*)i);
        ++tnn;
    }
    while(!kernel->scheduler->isReadyListEmpty()) kernel->currentThread->Yield();
}

void static testBarrier(int which)
{
    barrierMutex->Acquire();
    cerr<<"thread"<<which<<" output before barrier:"<<1<<endl;
    barrier->Barrier(barrierMutex);
    cerr<<"thread"<<which<<" output after barrier:"<<2<<endl;
    barrierMutex->Release();
}

void MyBarrier()
{
    barrier = new Condition("myBarrier");
    barrierMutex = new Lock("myBarrierMutex");
    Thread* test[TEST_THREAD_NUM];
    char tname[TEST_THREAD_NUM][20];
    int tnn=0;
    for(int i=0;i<TEST_THREAD_NUM;++i)
    {
        sprintf(tname[tnn],"testThread %d",i);
        test[i] = new Thread(tname[tnn]);
        test[i]->Fork((VoidFunctionPtr)testBarrier,(void*)i);
        ++tnn;
    }
    while(!kernel->scheduler->isReadyListEmpty()) kernel->currentThread->Yield();
}

void MyReader(int which)
{
    for(int i=which;i<which+10;++i)
    {
        rwMutex->Acquire();
        ++readerCount;
        if(readerCount==1) wLock->Acquire();
        rwMutex->Release();
        cerr<<"Reader"<<which<<" read pos"<<i<<": "<<readContent[i]<<endl;
        rwMutex->Acquire();
        --readerCount;
        if(!readerCount) wLock->Release();
        rwMutex->Release();
    }
}

void MyWriter(int which)
{
    for(int i=which;i<which+10;++i)
    {
        wLock->Acquire();
        cerr<<"Writer"<<which<<" write pos"<<i<<" from "<<readContent[i];
        readContent[i] = i*2+1;
        cerr<<" to "<<readContent[i]<<endl;
        wLock->Release();
    }
}

void MyReaderWriter()
{
    readerCount = 0;
    for(int i=0;i<READ_CONTENT_SIZE;++i) readContent[i] = i+1;
    wLock = new Lock("writeLock");
    rwMutex = new Lock("readWriteMutex");
    Thread* readers[READER_NUM];
    Thread* writers[WRITER_NUM];
    char tname[READER_NUM+WRITER_NUM][20];
    int tnn=0;
    for(int i=0;i<READER_NUM;++i)
    {
        sprintf(tname[tnn],"reader%d",i);
        readers[i] = new Thread(tname[tnn]);
        readers[i]->Fork((VoidFunctionPtr)MyReader,(void*)i);
        ++tnn;
    }
    for(int i=0;i<WRITER_NUM;++i)
    {
        sprintf(tname[tnn],"writer%d",i);
        writers[i] = new Thread(tname[tnn]);
        writers[i]->Fork((VoidFunctionPtr)MyWriter,(void*)i);
        ++tnn;
    }
    while(!kernel->scheduler->isReadyListEmpty()) kernel->currentThread->Yield();
}

void Kernel::SyncTest(int type)
{
    if(type==0) MyProducerConsumer1();
    else if(type==1) MyProducerConsumer2();
    else if(type==2) MyBarrier();
    else if(type==3) MyReaderWriter();
}