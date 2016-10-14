#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <usyscall.h>
#include <stdio.h>
#include <sems.h>


proc ProcTable[MAXPROC];


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

    name = ((char *)args->arg5);
    argument = ((char *)args->arg2);
    func = (int(*)(char *))(args->arg1);
    stacksize = *((int *)args->arg3);
    priority = *((int *)args->arg4);
    errors = 0;


    /* check stacksize, name, func and priority */
    if (stacksize < USLOSS_MIN_STACK) {
        errors = 1;
    }

    if (name == NULL || func == NULL || priority < 1 || priority > 6) {
        errors = 1;
    }

    if (errors) {
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

    // terminate
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
    procPtr slot, parent;


    /* intialize values */
    parent = &ProcTable[getpid() % MAXPROC];
    pid = fork1(name, spawnLaunch, arg, stacksize, priority);
    slot = &ProcTable[pid % MAXPROC];


    /* set child and parent nodes */
    parent->childList = addToList(parent->childList, slot, CHILD_LIST);
    slot->parent = parent;


    /* fill out process table */
    slot->pid = pid;
    slot->exitStatus = 0;
    slot->childList = NULL;
    slot->func = func;


    /* unblock spawnLaunch */
    MboxSend(slot->mailbox, NULL, 0);


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
    MboxReceive(slot->mailbox, NULL, 0);

    /* call function and return value if it is given call terminate() */
    switchToUserMode();
    returnVal = slot->func(arg);

    // call terminate
    return returnVal;
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
    int pid, processIndex, terminationCode;
    procPtr currentProcess;

    /* retrieve the pid of the process that needs to wait */
    pid = getpid();
    processIndex = pid % MAXPROC;
    currentProcess = &ProcTable[processIndex];

    /* call waitReal and retrieve exit code */
    terminationCode = waitReal(currentProcess);

    args->arg1 = (void *) pid;
    args->arg4 = (void *) terminationCode;
}



/* ------------------------------------------------------------------------
   Name - wait
   Purpose - The system call handler for SYS_SPAWN
   Parameters - A systemArgs pointer
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

int waitReal(procPtr currentProcess)
{
    procPtr quitChild;
    int exitStatus;

    /* if no one in quitlist block*/
    if (currentProcess->quitList == NULL) {
        MboxReceive(currentProcess->mailbox, NULL);
    }

    /* retrieve a child who has quit */
    quitChild = currentProcess->quitList;
    exitStatus = quitChild->exitStatus;
    procPtr->quitlist = removeFromList(procPtr->quitlist, quitChild, QUIT_LIST);

    return exitStatus;
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

    exitStatus = *((int *)(args->arg1));

    currentProcess = &ProcTable[getpid() % MAXPROC];

    terminateReal(exitStatus, currentProcess);
}



/* ------------------------------------------------------------------------
   Name - terminateReal
   Purpose - The system call handler for SYS_TERMINATE
   Parameters - A systemArgs pointer
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

void terminateReal(int exitStatus, procPtr currentProcess)
{
    procPtr childToTerminate;

    childToTerminate = NULL;

    /* check if process has children */
    if (currentProcess->childList != NULL) {
        childToTerminate = currentProcess->childList;

        while (childToTerminate != NULL) {
            zap(childToTerminate->pid);
            childToTerminate = childToTerminate->nextSibling;
        }
    }

    /* set exit status and conditionally send to parent if it is waiting */
    currentProcess->exitStatus = exitStatus;
    MboxCondSend(currentProcess->parent->mailbox, NULL, 0);
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

    while (getNext(head, listType) != NULL) {
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

    while (getNext(head, listType) != NULL) {
        if (getNext(temp, listType) == toRemove) {
            setNext(temp, getNext(getNext(temp, listType), listType), listType);
            return head;
        }
        temp = getNext(temp, listType);
    }
    
    fprintf(stderr, "The node to remove wasnt found.\n");
} 


procPtr getNext(procPtr node, int listType)
{
    switch (listType) {
        case CHILD_LIST:
            return node->nextSibling;
        case QUIT_LIST:
            return node->nextQuitSibling;
        default:
            fprintf(stderr, "The horse got you again...\n"); 
    }
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
        default:
            fprintf(stderr, "The horse got you again...\n"); 
    }   
}


void switchToUserMode()
{
    union psrValues currentPsrStatus;

    currentPsrStatus.integerPart = USLOSS_PsrGet();
    currentPsrStatus.bits.curMode = 0;
    USLOSS_PsrSet(currentPsrStatus.integerPart);
}













