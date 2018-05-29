//
// Created by Rivka Schuss on 28/05/2018.
//

#ifndef OSEX4_THREADPOOL_H
#define OSEX4_THREADPOOL_H

#include <zconf.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include "osqueue.h"
#include <stdio.h>


#define ERROR "Error in system call\n"
#define STDERR 2
#define SUCCESS 1
#define FAILURE 0
#define EXIT_FAILURE -1


enum State {RUNNING, DESTROY, BEFORE_DESTROY, AFTER_JOIN};

typedef struct task{
    void (*function)(void *);
    void *argument;
} task;

typedef struct thread_pool
{
    int numOfThreads;
    int stopped;
    enum State state;
    pthread_mutex_t lock, finalLock;
    pthread_cond_t cond;
    pthread_t *threads;
    OSQueue *queue;
    void (*executeTasks)(void *);
}ThreadPool;


ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void*), void* param);

void* func(void* args);

void executeTasks(void* args);

void error();


#endif //OSEX4_THREADPOOL_H
