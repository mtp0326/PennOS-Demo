#ifndef ARRAY_H
#define ARRAY_H

#include <stdbool.h>    // For bool type
#include <stddef.h>     // For size_t
#include <sys/types.h>  //needed for ssize_t, if we use ints, can remove

/**
 * @brief Structure for the dynamic array to store child PIDs.
 */
typedef struct {
  pid_t* array; /**< Pointer to the array of child PIDs. */
  size_t used;  /**< Number of elements currently used. */
  size_t size;  /**< Current allocated size of the array. */
} DynamicPIDArray;

/**
 * @brief Initializes a new dynamic array for PIDs with an initial size.
 *
 * @param initial_size The initial size of the dynamic array.
 * @return DynamicPIDArray* A pointer to the newly created dynamic array
 * structure.
 */
DynamicPIDArray* dynamic_pid_array_create(size_t initial_size);

/**
 * @brief Destroys a dynamic PID array, freeing its resources.
 *
 * @param array A pointer to the dynamic PID array to be destroyed.
 */
void dynamic_pid_array_destroy(DynamicPIDArray* array);

/**
 * @brief Adds a PID to the dynamic array, resizing if necessary.
 *
 * @param array A pointer to the dynamic PID array.
 * @param pid The PID to add to the array.
 * @return true on success, false on failure (e.g., if memory allocation fails).
 */
bool dynamic_pid_array_add(DynamicPIDArray* array, pid_t pid);

/**
 * @brief Removes a PID from the dynamic array.
 *
 * @param array A pointer to the dynamic PID array.
 * @param pid The PID to remove from the array.
 * @return true if the PID was successfully removed, false if the PID was not
 * found.
 */
bool dynamic_pid_array_remove(DynamicPIDArray* array, pid_t pid);

/**
 * @brief Checks if a PID exists in the dynamic array.
 *
 * @param array A pointer to the dynamic PID array.
 * @param pid The PID to check for in the array.
 * @return true if the PID exists in the array, false otherwise.
 */
bool dynamic_pid_array_contains(const DynamicPIDArray* array, pid_t pid);

#endif
