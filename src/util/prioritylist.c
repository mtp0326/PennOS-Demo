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
    PNode* current = list->head;
    while (current->next != list->head) {
      current = current->next;
    }
    current->next = newNode;
    newNode->next = list->head;  // Complete the circle
  }
  list->size++;
}

// Removes a process from the circular linked list by its PID
bool remove_priority(PList* list, unsigned int priority) {
  if (list == NULL || list->head == NULL) {
    return false;  // List is empty or not initialized
  }

  PNode *current = list->head, *prev = NULL;
  do {
    if (current->priority == priority) {
      if (prev == NULL) {                   // Removing the head
        if (current->next == list->head) {  // Only one element
          list->head = NULL;
        } else {
          prev = list->head;
          while (prev->next != list->head)
            prev = prev->next;
          list->head = current->next;
          prev->next = list->head;
        }
      } else {  // Removing non-head
        prev->next = current->next;
        if (current == list->head) {
          list->head = current->next;
        }
      }
      free(current);
      list->size--;
      return true;
    }
    prev = current;
    current = current->next;
  } while (current != list->head);

  return false;  // Process not found
}
