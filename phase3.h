/*
 * These are the definitions for phase 3 of the project
 */

#ifndef _PHASE3_H
#define _PHASE3_H

#define MAXSEMS                 200

#define CHILD_LIST              1
#define SIBLING_LIST            2
#define QUIT_LIST               3
#define QUIT_SIBLING_LIST       4



/* function prototypes */
extern void spawn(systemArgs *args);
extern int spawnReal(char *name, int (*func)(char *), char *arg, int stacksize, 
    int priority);
extern int spawnLaunch(char *);


/* typedef */
typedef struct semaphore semaphore;
typedef struct proc proc;
typedef struct proc *procPtr;

typedef struct childList childList;
typedef struct siblingList siblingList;
typedef struct quitList quitList;
typedef struct quitSiblingList quitSiblingList;


/* struct definitions */

struct childList {
    procPtr next;
};

struct siblingList {
    procPtr next;
};

struct quitList {
    procPtr next;
};

struct quitSiblingList {
    procPtr next;
};

struct semaphore {
    int count;
};

struct proc {
    int         pid;
    int         status;
    int         mailBox;
    int (*func)(char *);

    
    childList       children;
    siblingList     siblings;
    quitList        quits;
    quitSiblingList quitSiblings;
};





#endif /* _PHASE3_H */

