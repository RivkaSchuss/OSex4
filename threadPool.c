//
// Created by Rivka Schuss on 28/05/2018.
//

#include "threadPool.h"

/**
 * creates a new thread pool
 * @param numOfThreads the number of threads to create
 * @return the new thread pool
 */
ThreadPool* tpCreate(int numOfThreads) {
    ThreadPool *pool;


    if ((pool = (ThreadPool*)malloc(sizeof(ThreadPool))) == NULL)
        error();


    pool->numOfThreads = numOfThreads;
    pool->queue = osCreateQueue();
    pool->stopped = FAILURE;
    pool->state = RUNNING;
    pool->executeTasks = executeTasks;



    if ((pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * numOfThreads)) == NULL)
        error();


    if (pthread_mutex_init(&pool->lock, NULL) != 0 || pthread_mutex_init(&pool->finalLock, NULL) != 0 ||
            pthread_cond_init(&pool->cond, NULL) != 0) {
        error();
    }

    int i;
    for (i = 0; i < numOfThreads; ++i) {
        if (pthread_create(&(pool->threads[i]), NULL, func, pool) != 0)
            error();
    }


    return pool;

}

/**
 * executes the tasks in the queue
 * @param args the arguments for the task
 */
void executeTasks(void* args) {

    ThreadPool *pool = (ThreadPool*)args;

    while (pool->state != BEFORE_DESTROY) {

        if(osIsQueueEmpty(pool->queue) && (pool->state == RUNNING || pool->state == AFTER_JOIN)) {
            pthread_mutex_lock(&pool->lock);
            pthread_cond_wait(&pool->cond, &pool->lock);
        }
        else if (pool->state == DESTROY && osIsQueueEmpty(pool->queue)) {
            break;
        }

        else {
            pthread_mutex_lock(&pool->lock);
        }
        if (pool->state != BEFORE_DESTROY) {
            if (!osIsQueueEmpty(pool->queue)) {
                task *task = osDequeue(pool->queue);
                pthread_mutex_unlock(&pool->lock);
                task->function(task->argument);
                free(task);
            } else {
                pthread_mutex_unlock(&pool->lock);
                sleep(1);
            }
            if (pool->state == AFTER_JOIN) {
                break;
            }
        }
    }
}

/**
 * destroys the thread pool
 * @param threadPool the threadpool
 * @param shouldWaitForTasks whether or not to wait for tasks
 */
void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks) {

    pthread_mutex_lock(&threadPool->finalLock);
    if (threadPool->stopped) {
        return;
    }
    pthread_mutex_unlock(&threadPool->finalLock);

    pthread_mutex_lock(&threadPool->lock);
    if (pthread_cond_broadcast(&threadPool->cond) > 0 || pthread_mutex_unlock(&threadPool->lock) > 0) {
        error();
    }

    if (shouldWaitForTasks) {
        threadPool->state = DESTROY;
    } else {
        threadPool->state = AFTER_JOIN;
    }

    threadPool->stopped = 1;
    int i;
    int numOfThreads = threadPool->numOfThreads;
    for (i = 0; i < numOfThreads; ++i) {
        pthread_join(threadPool->threads[i], NULL);
    }
    threadPool->state = BEFORE_DESTROY;

    while (!osIsQueueEmpty(threadPool->queue)) {
        task* task = osDequeue(threadPool->queue);
        free(task);
    }

    osDestroyQueue(threadPool->queue);
    free(threadPool->threads);
    free(threadPool);
    pthread_mutex_destroy(&threadPool->lock);
    pthread_mutex_destroy(&threadPool->finalLock);
    /*
        //TODO fix busy waiting
        while(SUCCESS) {
            if (osIsQueueEmpty(threadPool->queue)) {
                break;
            } else {
                //TODO not allowed to use sleep
                sleep(1);
            }
        }
    }
     */


}

/**
 * inserts a task into the queue
 * @param threadPool the thread pool
 * @param computeFunc pointer to a function to add to the task
 * @param param the parameter for the function
 * @return returns 1 in success, 0 in failure
 */
int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void*), void* param) {

    if (threadPool->stopped) {
        return FAILURE;
    } else {
        task* newTask = (task *) malloc(sizeof(task));
        if (newTask == NULL) {
            error();
        } else {
            newTask->function = computeFunc;
            newTask->argument = param;
            pthread_mutex_lock(&threadPool->lock);
            osEnqueue(threadPool->queue, newTask);
            if (pthread_cond_signal(&threadPool->cond) != 0) {
                error();
            }
            pthread_mutex_unlock(&threadPool->lock);
            return SUCCESS;
        }
    }
}

/**
 * function that calls the execute function
 * @param args the arguments
 * @return void pointer
 */
void* func(void* args) {
    ThreadPool *pool = (ThreadPool *) args;
    pool->executeTasks(pool);
}

/**
 * writes an error.
 */
void error() {
    write(STDERR, ERROR, sizeof(ERROR));
    printf("error\n");
    exit(EXIT_FAILURE);
}
