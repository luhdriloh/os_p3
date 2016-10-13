#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <usyscall.h>
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
        ProcTable[i].mailBox = MboxCreate(0,0);
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
        *(args->arg1) = (void *) -1;
        *(args->arg4) = (void *) -1;
        return;
    }

    
    /* call spawn to create the user proces */
    spawnSuccess = spawnReal(name, func, argument, stacksize, priority);

    if (spawnSuccess == -1) {
        *(args->arg1) = (void *) -1;
        *(args->arg4) = (void *) -1;
    }

    *(args->arg4) = (void *) -1;
    *(args->arg1) = (void *) spawnSuccess;

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


    /* add child process to parent */
    parent->children = addChild(parent->children, slot);


    /* fill out process table */
    slot->pid = pid;
    slot->func = func;
    slot->children = NULL;


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
    returnVal = slot->func(arg);

    // call terminate
    return returnVal;
}



/* ------------------------------------------------------------------------
   Name - wait
   Purpose - The system call handler for SYS_SPAWN
   Parameters - A systemArgs pointer
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

void wait(systemArgs *args) {

}



/* ------------------------------------------------------------------------
   Name - wait
   Purpose - The system call handler for SYS_SPAWN
   Parameters - A systemArgs pointer
   Returns - n/a
   Side Effects - n/a
   ----------------------------------------------------------------------- */

void wait(systemArgs *args) {

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
    int status;

    status = *((int *)(args->args1));
}



/* ------------------------------------------------------------------------
   Name - nullsys3
   Purpose - Temporary function to call for the syscall handler
   Parameters - n/a
   Returns - n/a
   Side Effects - Halts
   ----------------------------------------------------------------------- */

void nullsys3(systemArgs *args)
{

} /* nullsys3 */



/* ------------------------------------------------------------------------
   Name - addChild
   Purpose - Temporary function to call for the syscall handler
   Parameters - n/a
   Returns - n/a
   Side Effects - Halts
   ----------------------------------------------------------------------- */

procPtr addChild(procPtr head, procPtr toAdd)
{

} /* addChild */

















