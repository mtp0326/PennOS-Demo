#include "prioritylist.h"
#include <stdlib.h>  // for malloc, free

// Initializes a circular linked list
PList* init_priority(void) {
  PList* list = (PList*)malloc(sizeof(PList));
  if (list == NULL) {
    return NULL;  // Failed to allocate memory for the list
  }
  list->head = NULL;
  list->size = 0;
  return list;
}

// Adds a new process to the circular linked list
void add_priority(PList* list, unsigned int priority) {
  if (list == NULL) {
    return;  // Invalid parameters
  }

  PNode* newNode = (PNode*)malloc(sizeof(PNode));
  if (newNode == NULL) {
    return;  // Failed to allocate memory for the new node
  }
  newNode->priority = priority;

  if (list->head == NULL) {
    // If the list is empty, initialize it with the new node
    newNode->next = newNode;  // Points to itself, making it circular
    list->head = newNode;
  } else {
    // Insert the new node at the end of the list
    PNode* currentnode = list->head;
    while (currentnode->next != list->head) {
      currentnode = currentnode->next;
    }
    currentnode->next = newNode;
    newNode->next = list->head;  // Complete the circle
  }
  list->size++;
}

// Removes a process from the circular linked list by its PID
bool remove_priority(PList* list, unsigned int priority) {
  if (list == NULL || list->head == NULL) {
    return false;  // List is empty or not initialized
  }

  PNode *currentnode = list->head, *prev = NULL;
  do {
    if (currentnode->priority == priority) {
      if (prev == NULL) {                       // Removing the head
        if (currentnode->next == list->head) {  // Only one element
          list->head = NULL;
        } else {
          prev = list->head;
          while (prev->next != list->head)
            prev = prev->next;
          list->head = currentnode->next;
          prev->next = list->head;
        }
      } else {  // Removing non-head
        prev->next = currentnode->next;
        if (currentnode == list->head) {
          list->head = currentnode->next;
        }
      }
      free(currentnode);
      list->size--;
      return true;
    }
    prev = currentnode;
    currentnode = currentnode->next;
  } while (currentnode != list->head);

  return false;  // Process not found
}


void free_plist(PList* list) {
    if (list == NULL) {
        return;
    }

    if (list->head == NULL) {
        free(list);
        return;
    }

    PNode* current = list->head;
    PNode* next_node;

    do {
        next_node = current->next;
        free(current);
        current = next_node;
    } while (current != list->head);

    free(list);
}

