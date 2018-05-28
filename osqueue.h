//
// Created by Rivka Schuss on 28/05/2018.
//

#ifndef OSEX4_OSQUEUE_H
#define OSEX4_OSQUEUE_H


typedef struct os_node
{
    struct os_node* next;
    void* data;
}OSNode;

typedef struct os_queue
{
    OSNode *head, *tail;

}OSQueue;

OSQueue* osCreateQueue();

void osDestroyQueue(OSQueue* queue);

int osIsQueueEmpty(OSQueue* queue);

void osEnqueue(OSQueue* queue, void* data);

void* osDequeue(OSQueue* queue);

#endif //OSEX4_OSQUEUE_H
