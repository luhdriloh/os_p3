#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <usyscall.h>
#include <stdio.h>


proc      ProcTable[MAXPROC];
semaphore SemTable[MAXSEMS];
int       semCreateMutex;

extern int start3 (char *);

int start2(char *arg)
{
    int pid;
    int status;
    /*
     * Check kernel mode here.
     */

    /*
     * Data structure initialization as needed...
     */


    /*
     * Create first user-level process and wait for it to finish.
     * These are lower-case because they are not system calls;
     * system calls cannot be invoked from kernel mode.
     * Assumes kernel-mode versions of the system calls
     * with lower-case names.  I.e., Spawn is the user-mode function
     * called by the test cases; spawn is the kernel-mode function that
     * is called by the syscallHandler; spawnReal is the function that
     * contains the implementation and is called by spawn.
     *
     * Spawn() is in libuser.c.  It invokes USLOSS_Syscall()
     * The system call handler calls a function named spawn() -- note lower
     * case -- that extracts the arguments from the sysargs pointer, and
     * checks them for possible errors.  This function then calls spawnReal().
     *
     * Here, we only call spawnReal(), since we are already in kernel mode.
     *
     * spawnReal() will create the process by using a call to fork1 to
     * create a process executing the code in spawnLaunch().  spawnReal()
     * and spawnLaunch() then coordinate the completion of the phase 3
     * process table entries needed for the new process.  spawnReal() will
     * return to the original caller of Spawn, while spawnLaunch() will
     * begin executing the function passed to Spawn. spawnLaunch() will
     * need to switch to user-mode before allowing user code to execute.
     * spawnReal() will return to spawn(), which will put the return
     * values back into the sysargs pointer, switch to user-mode, and 
     * return to the user code that called Spawn.
     */

    for (int i = 0; i < USLOSS_MAX_SYSCALLS; i++) {
        systemCallVec[i] = nullsys3;
    }

    systemCallVec[SYS_SPAWN] = spawn;
    systemCallVec[SYS_TERMINATE] = terminate;
    systemCallVec[SYS_WAIT] = wait;
    systemCallVec[SYS_SEMCREATE] = semCreate;
    systemCallVec[SYS_SEMP] = semP;
    systemCallVec[SYS_SEMV] = semV;
    systemCallVec[SYS_SEMFREE] = semFree;
    systemCallVec[SYS_GETPID] = getPID;
    systemCallVec[SYS_GETTIMEOFDAY] = getTimeofDay;
    systemCallVec[SYS_CPUTIME] = cpuTime;

    semCreateMutex = MboxCreate(1,0);

    /* fill out semaphore table */
    for (int i = 0; i < MAXSEMS; i++) {
        SemTable[i].mutex = MboxCreate(1, 0);
    }


    /* give each process tableslot its own personal mailbox */
    for (int i = 0; i < MAXPROC; i++) {
        ProcTable[i].mailbox = MboxCreate(0,0);
    }

    pid = spawnReal("start3", start3, NULL, USLOSS_MIN_STACK, 3);

    /* Call the waitReal version of your wait code here.
     * You call waitReal (rather than Wait) because start2 is running
     * in kernel (not user) mode.ww
     */
    pid = waitReal(&status);
} /* start2 */



/* ------------------------------------------------------------------------
   Name - spawn
   Purpose - The system call handler for SYS_SPAWN
   Parameters - A systemArgs pointer
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

void spawn(systemArgs *args)
{
    /* parse the systemArgs argument and call spawnReal */
    char *name, *argument;
    int (*func)(char *);
    int stacksize, priority, errors, spawnSuccess;

    errors = 0;

    /* check stacksize, name, func and priority */
    if (args->arg3 < USLOSS_MIN_STACK) {
        errors = 1;
    }

    if (args->arg5 == NULL || args->arg1 == NULL || args->arg4 == NULL) {
        errors = 1;
    }

    if (errors) {
        args->arg1 = (void *) -1;
        args->arg4 = (void *) -1;
        return;
    }

    name = args->arg5;
    argument = args->arg2;
    func = (int(*)(char *))(args->arg1);
    stacksize = (int) args->arg3;
    priority = (int) args->arg4;

    if (priority < 1 || priority > 6) {
        args->arg1 = (void *) -1;
        args->arg4 = (void *) -1;
        return;
    }

    /* call spawn to create the user proces */
    spawnSuccess = spawnReal(name, func, argument, stacksize, priority);

    if (spawnSuccess == -1) {
        args->arg1 = (void *) -1;
        args->arg4 = (void *) -1;
    }

    args->arg1 = (void *) spawnSuccess;
    args->arg4 = (void *) 0;
} /* spawn */



/* ------------------------------------------------------------------------
   Name - spawnReal
   Purpose - n/a
   Parameters - n/a
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

int spawnReal(char *name, int (*func)(char *), char *arg, int stacksize, int priority)
{
    int pid;
    procPtr spawnedSlot, parent;

    /* initialize values */
    pid = fork1(name, spawnLaunch, arg, stacksize, priority);

    if (pid == -1) {
        return -1;
    }

    spawnedSlot = &ProcTable[pid % MAXPROC];


    /* set child and parent nodes */
    parent = &ProcTable[getpid() % MAXPROC];

    parent->childList = addToList(parent->childList, spawnedSlot, CHILD_LIST);
    spawnedSlot->parent = parent;


    /* fill out process table */
    spawnedSlot->pid = pid;
    spawnedSlot->exitStatus = 0;
    spawnedSlot->childList = NULL;
    spawnedSlot->func = func;


    /* unblock spawnLaunch */
    MboxCondSend(spawnedSlot->mailbox, NULL, 0);

    // put into process table
    return pid;
} /* spawnReal */



/* ------------------------------------------------------------------------
   Name - spawnLaunch
   Purpose - n/a
   Parameters - n/a
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

int spawnLaunch(char *arg)
{
    int returnVal;
    procPtr slot;

    /* block spawnLaunch until process table filled in */
    slot = &ProcTable[getpid() % MAXPROC];

    if (slot->pid != getpid()) {
        MboxReceive(slot->mailbox, NULL, 0);
    }

    if (isZapped()) {
        terminateReal(-1);
    }

    /* call function and return value if it is given call terminate() */
    switchToUserMode();
    returnVal = slot->func(arg);

    /* call terminate */
    Terminate(returnVal);
}



/* ------------------------------------------------------------------------
   Name - wait
   Purpose - The system call handler for SYS_WAIT
   Parameters - A systemArgs pointer
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

void wait(systemArgs *args)
{
    int terminationCode, pid;

    /* call waitReal and retrieve exit code */
    pid = waitReal(&terminationCode);

    args->arg1 = pid;
    args->arg2 = terminationCode;
}



/* ------------------------------------------------------------------------
   Name - wait
   Purpose - The system call handler for SYS_SPAWN
   Parameters - A systemArgs pointer
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

int waitReal(int *terminationCode)
{
    procPtr quitChild, currentProcess;
    int exitStatus;

    /* get current process and check if no one is in quitlist block */
    currentProcess = &ProcTable[getpid() % MAXPROC];
    exitStatus = 0;

    join(&exitStatus);

    /* retrieve a child who has quit */
    quitChild = currentProcess->quitList;
    exitStatus = quitChild->exitStatus;
    currentProcess->quitList = removeFromList(currentProcess->quitList, quitChild, QUIT_LIST);

    /* put terminationCode in int pointer given and return pid of quit child */
    *terminationCode = exitStatus;
    return quitChild->pid;
}



/* ------------------------------------------------------------------------
   Name - Terminate
   Purpose - n/a
   Parameters - n/a
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

void terminate(systemArgs *args)
{
    int exitStatus;
    procPtr currentProcess;

    exitStatus = (int) args->arg1;

    /* get current process and call terminate real */
    currentProcess = &ProcTable[getpid() % MAXPROC];
    terminateReal(exitStatus);
}



/* ------------------------------------------------------------------------
   Name - terminateReal
   Purpose - The system call handler for SYS_TERMINATE
   Parameters - A systemArgs pointer
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

void terminateReal(int exitStatus)
{
    procPtr childToTerminate, parent, currentProcess;

    currentProcess = &ProcTable[getpid() % MAXPROC];
    parent = currentProcess->parent;
    childToTerminate = NULL;


    if (currentProcess->childList != NULL) {
        childToTerminate = currentProcess->childList;

        while (childToTerminate != NULL) {
            zap(childToTerminate->pid);
            childToTerminate = currentProcess->childList;
        }
    }

    currentProcess->exitStatus = exitStatus;
    parent->childList = removeFromList(parent->childList, currentProcess, CHILD_LIST);
    parent->quitList = addToList(parent->quitList, currentProcess, QUIT_LIST);

    quit(0);
}



/* ------------------------------------------------------------------------
   Name - semCreate
   Purpose - 
   Parameters -
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

void semCreate(systemArgs *args)
{
    int initialValue, slot;

    initialValue = args->arg1;

    /* check that valid start count is given */
    if (initialValue < 0) {
        args->arg1 = -1;
        args->arg4 = -1;
        return;
    }

    slot = semCreateReal(initialValue);
   
    /* check if semaphore table is full */
    if (slot == -1) {
        args->arg1 = -1;
        args->arg4 = -1;
        return;
    }

    args->arg1 = slot;
    args->arg4 = 0;

    if (isZapped()) {
        Terminate(0);
    }

    switchToUserMode();
}


/* ------------------------------------------------------------------------
   Name - semCreateReal
   Purpose - 
   Parameters -
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

int semCreateReal(int semValue)
{
    int slot;

    slot = -1;

    /* mutual exclusion */
    MboxSend(semCreateMutex, NULL, 0);
    
    /* find an empty semaphore slot */
    for (int i = 0; i < MAXSEMS; i++) {

        if (SemTable[i].status == FREE) {
            slot = i;
            break;
        }
    }

    if (slot != -1) {
        SemTable[slot].status = IN_USE;
        SemTable[slot].count = semValue;
        SemTable[slot].blockedProcs = NULL;
    }

    /* release any other blocked proesses */
    MboxReceive(semCreateMutex, NULL, 0);

    return slot;
}


/* ------------------------------------------------------------------------
   Name - semP
   Purpose - 
   Parameters -
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

void semP(systemArgs *args)
{
    int semHandle, semVal;

    semHandle = args->arg1;

    if (semHandle < 0 || semHandle > MAXSEMS) {
        args->arg4 = -1;
    }
    else {
        semVal = semPReal(semHandle);
        args->arg4 = semVal;
    }
}


/* ------------------------------------------------------------------------
   Name - semPReal
   Purpose - 
   Parameters -
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

int semPReal(int semHandle)
{
    semPtr semToChange;

    semToChange = &SemTable[semHandle];

    /* check if we have a valid semaphore handle */
    if (semToChange->status == FREE) {
        return -1;
    }

    /* mutex */
    MboxSend(semToChange->mutex, NULL, 0);

    if (semToChange->count > 0) {
        (semToChange->count)--;
        
        /* mutex release */
        MboxReceive(semToChange->mutex, NULL, 0);

    }
    else {
        procPtr currentProcess = &ProcTable[getpid() % MAXPROC];
        semToChange->blockedProcs = addToList(semToChange->blockedProcs,
                                              currentProcess, SEM_BLOCK_LIST);

        /* mutex release */
        MboxReceive(semToChange->mutex, NULL, 0);
        MboxReceive(currentProcess->mailbox, NULL, 0);

        if (semToChange->status == FREE) {
            terminateReal(1);
        }

        (semToChange->count)--;
    }

    return 0;
}


/* ------------------------------------------------------------------------
   Name - semV
   Purpose - 
   Parameters -
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

void semV(systemArgs *args)
{
    int semHandle, semVal;

    semHandle = args->arg1;

    if (semHandle < 0 || semHandle > MAXSEMS) {
        args->arg4 = -1;
    }
    else {
        semVal = semVReal(semHandle);
        args->arg4 = semVal;
    }
}


/* ------------------------------------------------------------------------
   Name - semVReal
   Purpose - 
   Parameters -
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

int semVReal(int semHandle)
{
    semPtr semToChange;

    semToChange = &SemTable[semHandle];

    /* check if we have a valid semaphore handle */
    if (semToChange->status == FREE) {
        return -1;
    }

    /* mutex */
    MboxSend(semToChange->mutex, NULL, 0);
    (semToChange->count)++;

    if (semToChange->blockedProcs != NULL) {
        procPtr procToUnblock = semToChange->blockedProcs;
        semToChange->blockedProcs = removeFromList(semToChange->blockedProcs, procToUnblock, SEM_BLOCK_LIST);
        MboxSend(procToUnblock->mailbox, NULL, 0);
    }

    MboxReceive(semToChange->mutex, NULL, 0);

    return 0;
}



/* ------------------------------------------------------------------------
   Name - semFree
   Purpose - 
   Parameters -
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

void semFree(systemArgs *args)
{
    int semHandle, semVal;

    semHandle = args->arg1;

    if (semHandle < 0 || semHandle > MAXSEMS) {
        args->arg4 = -1;
    }
    else {
        semVal = semFreeReal(semHandle);
        args->arg4 = semVal;
    }
}


/* ------------------------------------------------------------------------
   Name - semFreeReal
   Purpose - 
   Parameters -
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

int semFreeReal(int semHandle)
{
    semPtr semToFree;
    procPtr procToFree;
    int returnVal;

    semToFree = &SemTable[semHandle];
    returnVal = 0;

    /* check that we have a valid semaphore */
    if (semToFree->blockedProcs != NULL) {
        returnVal = 1;
    }

    semToFree->status = FREE;
    procToFree = semToFree->blockedProcs;

    while (procToFree != NULL) {
        MboxSend(procToFree->mailbox, NULL, 0);
        procToFree = procToFree->nextSemBlockedSibling;
    }

    semToFree->blockedProcs = NULL;
    return returnVal;
}


/* ------------------------------------------------------------------------
   Name - getPID
   Purpose - 
   Parameters -
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

void getPID(systemArgs *args)
{
    int pid;

    pid = getPIDReal();

    args->arg1 = pid;
}



/* ------------------------------------------------------------------------
   Name - getPIDReal
   Purpose - 
   Parameters -
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

int getPIDReal()
{
    return getpid();
}


/* ------------------------------------------------------------------------
   Name - getTimeofDay
   Purpose - 
   Parameters -
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

void getTimeofDay(systemArgs *args)
{
    int time;

    time = getTimeofDayReal();

    args->arg1 = time;
}



/* ------------------------------------------------------------------------
   Name - getTimeofDayReal
   Purpose - 
   Parameters -
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

int getTimeofDayReal()
{
    return USLOSS_Clock();
}


/* ------------------------------------------------------------------------
   Name - cpuTime
   Purpose - 
   Parameters -
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

void cpuTime(systemArgs *args)
{
    int time;

    time = getTimeofDayReal();

    args->arg1 = time;
}



/* ------------------------------------------------------------------------
   Name - cputTimeReal
   Purpose - 
   Parameters -
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

int cpuTimeReal()
{
    return readtime();
}


/* ------------------------------------------------------------------------
   Name - nullsys3
   Purpose - Temporary function to call for the syscall handler
   Parameters - n/a
   Returns - n/a
   Side Effects - Halts
   ----------------------------------------------------------------------- */

void nullsys3(int status)
{

} /* nullsys3 */



procPtr addToList(procPtr head, procPtr toAdd, int listType)
{  
    setNext(toAdd, NULL, listType);

    if (head == NULL) {
        return toAdd;
    }
    
    procPtr temp = head;

    while (getNext(temp, listType) != NULL) {
        temp = getNext(temp, listType);
    }

    setNext(temp, toAdd, listType);
    return head; 
}


procPtr removeFromList(procPtr head, procPtr toRemove, int listType)
{

    if (head == toRemove) {
        return getNext(head, listType);   
    }

    procPtr temp = head;    

    while (getNext(temp, listType) != NULL) {
        if (getNext(temp, listType) == toRemove) {
            setNext(temp, getNext(getNext(temp, listType), listType), listType);
            return head;
        }
        temp = getNext(temp, listType);
    }
    
    fprintf(stderr, "removeFromList(): The node to remove wasnt found.\nlist type: %d\n\n", listType);
    return NULL;
} 


procPtr getNext(procPtr node, int listType)
{
    switch (listType) {
        case CHILD_LIST:
            return node->nextSibling;
        case QUIT_LIST:
            return node->nextQuitSibling;
        case SEM_BLOCK_LIST:
            return node->nextSemBlockedSibling;
        default:
            fprintf(stderr, "getNext(): The horse got you again...\n"); 
    }
    return NULL;
}


void setNext(procPtr node, procPtr toSet, int listType)
{

    switch (listType) {
        case CHILD_LIST:
            node->nextSibling = toSet;
            return;
        case QUIT_LIST:
            node->nextQuitSibling = toSet;
            return;
        case SEM_BLOCK_LIST:
            node->nextSemBlockedSibling = toSet;
            return;
        default:
            fprintf(stderr, "setNext(): The horse got you again...\n"); 
    }   
}


void switchToUserMode()
{
    union psrValues currentPsrStatus;

    currentPsrStatus.integerPart = USLOSS_PsrGet();
    currentPsrStatus.bits.curMode = 0;
    USLOSS_PsrSet(currentPsrStatus.integerPart);
}









