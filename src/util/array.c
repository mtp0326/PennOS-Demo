#include "array.h"
#include <stdlib.h>  // For malloc, free, realloc

// Initializes a new dynamic array for PIDs with an initial size.
DynamicPIDArray* dynamic_pid_array_create(size_t initial_size) {
  DynamicPIDArray* array = (DynamicPIDArray*)malloc(sizeof(DynamicPIDArray));
  if (array == NULL) {
    // Memory allocation failed
    return NULL;
  }
  array->array = (pid_t*)malloc(initial_size * sizeof(pid_t));
  if (array->array == NULL) {
    // Memory allocation failed
    free(array);
    return NULL;
  }
  array->used = 0;
  array->size = initial_size;
  return array;
}

// Destroys a dynamic PID array, freeing its resources.
void dynamic_pid_array_destroy(DynamicPIDArray* array) {
  if (array != NULL) {
    free(array->array);  // Free the internal array
    free(array);         // Free the structure itself
  }
}

// Adds a PID to the dynamic array, resizing if necessary.
bool dynamic_pid_array_add(DynamicPIDArray* array, pid_t pid) {
  if (array->used == array->size) {
    size_t newSize = array->size * 2;
    pid_t* newArray = (pid_t*)realloc(array->array, newSize * sizeof(pid_t));
    if (newArray == NULL) {
      // Memory allocation failed
      return false;
    }
    array->array = newArray;
    array->size = newSize;
  }
  array->array[array->used++] = pid;
  return true;
}

// Removes a PID from the dynamic array.
bool dynamic_pid_array_remove(DynamicPIDArray* array, pid_t pid) {
  for (size_t i = 0; i < array->used; i++) {
    if (array->array[i] == pid) {
      // Shift elements down to fill the gap
      for (size_t j = i; j < array->used - 1; j++) {
        array->array[j] = array->array[j + 1];
      }
      array->used--;
      return true;
    }
  }
  // PID not found
  return false;
}

// Checks if a PID exists in the dynamic array.
bool dynamic_pid_array_contains(const DynamicPIDArray* array, pid_t pid) {
  for (size_t i = 0; i < array->used; i++) {
    if (array->array[i] == pid) {
      return true;  // PID found
    }
  }
  return false;  // PID not found
}
