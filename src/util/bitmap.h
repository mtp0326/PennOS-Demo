#ifndef OPENFD_BITMAP_H
#define OPENFD_BITMAP_H

#include <stdbool.h>  // For bool type
#include <stdint.h>   // For uint8_t, uint32_t types

#define FD_BITMAP_SIZE 1024  // Maximum number of file descriptors
#define FD_BITMAP_BYTES (FD_BITMAP_SIZE / 8)  // Number of bytes needed

/**
 * @brief Structure for managing open file descriptors using a bitmap.
 */
typedef struct {
  uint8_t bits[FD_BITMAP_BYTES]; /**< Array of bytes to represent the bitmap. */
} FD_Bitmap;

/**
 * @brief Initializes the bitmap to all zeros, indicating that no file
 * descriptors are in use.
 *
 * @param bitmap A pointer to the file descriptor bitmap to initialize.
 */
void fd_bitmap_initialize(FD_Bitmap* bitmap);

/**
 * @brief Sets the bit for a given file descriptor, indicating it is now in use.
 *
 * @param bitmap A pointer to the file descriptor bitmap.
 * @param fd The file descriptor to mark as in use.
 * @return bool True if the operation was successful, false if the fd is out of
 * range.
 */
bool fd_bitmap_set(FD_Bitmap* bitmap, uint32_t fd);

/**
 * @brief Clears the bit for a given file descriptor, indicating it is no longer
 * in use.
 *
 * @param bitmap A pointer to the file descriptor bitmap.
 * @param fd The file descriptor to mark as not in use.
 * @return bool True if the operation was successful, false if the fd is out of
 * range.
 */
bool fd_bitmap_clear(FD_Bitmap* bitmap, uint32_t fd);

/**
 * @brief Tests whether the bit for a given file descriptor is set, indicating
 * whether it is in use.
 *
 * @param bitmap A pointer to the file descriptor bitmap.
 * @param fd The file descriptor to check.
 * @return bool True if the file descriptor is in use, false otherwise.
 */
bool fd_bitmap_test(const FD_Bitmap* bitmap, uint32_t fd);

#endif