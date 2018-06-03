//
// Rivka Schuss 340903129
//

#include "threadPool.h"

/**
 * creates a new thread pool
 * @param numOfThreads the number of threads to create
 * @return the new thread pool
 */
ThreadPool* tpCreate(int numOfThreads) {
    ThreadPool *pool;

    //allocating memory for the thread pool
    if ((pool = (ThreadPool*)malloc(sizeof(ThreadPool))) == NULL)
        error();


    if (numOfThreads <= 0)
        error();


    //initializing all the fields for the thread pool
    pool->numOfThreads = numOfThreads;
    pool->queue = osCreateQueue();
    pool->stopped = FAILURE;
    pool->state = RUNNING;
    pool->executeTasks = executeTasks;

    //allocating memory for the threads
    if ((pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * numOfThreads)) == NULL)
        error();


    //initialing the mutexes and the condition
    if (pthread_mutex_init(&pool->lock, NULL) != 0 || pthread_mutex_init(&pool->finalLock, NULL) != 0 ||
            pthread_cond_init(&pool->cond, NULL) != 0) {
        error();
    }

    //creating the threads
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

    //while it is not time to destroy
    while (pool->state != END) {

        //if it is not time to destroy, and the queue is empty
        if(osIsQueueEmpty(pool->queue) && (pool->state == RUNNING || pool->state == AFTER_JOIN)) {
            pthread_mutex_lock(&pool->lock);
            //wait for a signal
            pthread_cond_wait(&pool->cond, &pool->lock);
        }
            //if the queue is empty and it is time to destroy, stop waiting
        else if (pool->state == DESTROY && osIsQueueEmpty(pool->queue)) {
            break;
        }

            //the queue is not empty, lock the queue before a task is executed
        else {
            pthread_mutex_lock(&pool->lock);
        }
        //the queue was not empty, continue to execute the tasks
        if (pool->state != END) {
            //there are more tasks
            if (!osIsQueueEmpty(pool->queue)) {
                task *task = osDequeue(pool->queue);
                pthread_mutex_unlock(&pool->lock);
                task->function(task->argument);
                free(task);
                //no more tasks
            } else {
                pthread_mutex_unlock(&pool->lock);
                //sleep(1);
            }
            //if it is time to destroy, and all the tasks are finished
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

    //if there are still tasks in the queue
    if (shouldWaitForTasks) {
        threadPool->state = DESTROY;
        //finish the threads
    } else {
        threadPool->state = AFTER_JOIN;
    }

    threadPool->stopped = 1;
    int i;
    int numOfThreads = threadPool->numOfThreads;
    //joining the threads
    for (i = 0; i < numOfThreads; ++i) {
        pthread_join(threadPool->threads[i], NULL);
    }
    threadPool->state = END;

    //go over the non empty queue, free the tasks
    while (!osIsQueueEmpty(threadPool->queue)) {
        task* task = osDequeue(threadPool->queue);
        free(task);
    }

    //destroy and free
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
        //allocating memory for the new task
        task* newTask = (task *) malloc(sizeof(task));
        if (newTask == NULL) {
            error();
        } else {
            //setting the fields for the task
            newTask->function = computeFunc;
            newTask->argument = param;
            //locking before adding to the queue
            pthread_mutex_lock(&threadPool->lock);
            //insert the task to the queue
            osEnqueue(threadPool->queue, newTask);
            //stop the thread from waiting, signal that the queue is no longer empty
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
    int fd = open("errors.txt", O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
    if (fd == FAILURE)
        error();
    // change the file where the errors will be written.
    dup2(fd, 1);
    write(STDERR, ERROR, sizeof(ERROR));
    dup2(1, fd);
    exit(EXIT_FAILURE);
}
