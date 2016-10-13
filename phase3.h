/*
 * These are the definitions for phase 3 of the project
 */

#ifndef _PHASE3_H
#define _PHASE3_H

#define MAXSEMS         200


/* function prototypes */
extern void spawn(systemArgs *args);
extern int spawnReal(char *name, int (*func)(char *), char *arg, int stacksize, 
    int priority);
extern int spawnLaunch(char *);


/* typedef */
typedef struct semaphore semaphore;
typedef struct proc proc;
typedef struct proc *procPtr;


/* struct definitions */
struct semaphore {
    int count;
};

struct proc {
    int         pid;
    int         status;
    int         mailBox;
    int (*func)(char *);

    /*  */
    procPtr     children;
    procPtr     sibling;
};





#endif /* _PHASE3_H */

