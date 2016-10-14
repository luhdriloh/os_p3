/*
 * These are the definitions for phase 3 of the project
 */

#ifndef _PHASE3_H
#define _PHASE3_H

#define MAXSEMS                 200

#define CHILD_LIST              1
#define QUIT_LIST               2


/* typedef */
typedef struct semaphore semaphore;
typedef struct proc proc;
typedef struct proc *procPtr;


/* struct definitions */

struct semaphore {
    int         count;
};

struct proc {
    int         pid;
    int         exitStatus;
    int         mailbox;
    int         (*func)(char *);

    procPtr     parent;

    procPtr     childList;
    procPtr     nextSibling;
    procPtr     quitList;
    procPtr     nextQuitSibling;
};

struct psrBits {
    unsigned int curMode:1;
    unsigned int curIntEnable:1;
    unsigned int prevMode:1;
    unsigned int prevIntEnable:1;
    unsigned int unused:28;
};

union psrValues {
   struct psrBits bits;
   unsigned int integerPart;
};


/* function prototypes */
extern void    spawn(systemArgs *args);
extern int     spawnReal(char *name, int (*func)(char *), char *arg, int stacksize, int priority);
extern int     spawnLaunch(char *arg);

extern void    wait(systemArgs *args);
extern int     waitReal(procPtr slot);

extern void    terminate(systemArgs *args);
extern void    terminateReal(int exitStatus, procPtr currentProcess);

extern procPtr addToList(procPtr head, procPtr toAdd, int listType);
extern procPtr removeFromList(procPtr head, procPtr toRemove, int listType);
extern procPtr getNext(procPtr node, int listType);
extern void    setNext(procPtr node, procPtr toSet, int listType);

extern void    switchToUserMode();


#endif /* _PHASE3_H */

