#include "bitmap.h"
#include <string.h>  // For memset

void fd_bitmap_initialize(FD_Bitmap* bitmap) {
  memset(bitmap->bits, 0, sizeof(bitmap->bits));
}

bool fd_bitmap_set(FD_Bitmap* bitmap, uint32_t fd) {
  if (fd >= FD_BITMAP_SIZE) {
    return false;  // File descriptor out of range
  }
  uint32_t byteIndex = fd / 8;
  uint32_t bitIndex = fd % 8;
  bitmap->bits[byteIndex] |= (1 << bitIndex);  // Set the bit
  return true;
}

bool fd_bitmap_clear(FD_Bitmap* bitmap, uint32_t fd) {
  if (fd >= FD_BITMAP_SIZE) {
    return false;  // File descriptor out of range
  }
  uint32_t byteIndex = fd / 8;
  uint32_t bitIndex = fd % 8;
  bitmap->bits[byteIndex] &= ~(1 << bitIndex);  // Clear the bit
  return true;
}

bool fd_bitmap_test(const FD_Bitmap* bitmap, uint32_t fd) {
  if (fd >= FD_BITMAP_SIZE) {
    return false;  // File descriptor out of range
  }
  uint32_t byteIndex = fd / 8;
  uint32_t bitIndex = fd % 8;
  return (bitmap->bits[byteIndex] & (1 << bitIndex)) != 0;  // Test the bit
}
