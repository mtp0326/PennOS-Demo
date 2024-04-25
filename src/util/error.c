#include "error.h"
void u_perror(char* message) {
  char* err_message;
  if (message == NULL) {
    return;
  } else if (strcmp(message, "\0") == 0) {
    return;
  } else {
    write(STDERR_FILENO, message, strlen(message));
    switch (errno) {
      case ETHREADCREATE:
        err_message = "Thread creation failed";
        break;
      case EPCBCREATE:
        err_message = "PCB creation failed";
        break;
      case ENOARGS:
        err_message = "Spawn recieved no arguments";
        break;

      case EBADFILENAME:
        err_message = "Invalid file name";
        break;
      case EMULTWRITE:
        err_message = "Cannot open a file in write mode more than once";
        break;
      case EWRONGPERM:
        err_message = "Incompatible file permission for the action";
        break;
      case ESYSERR:
        err_message = "C-level system error";
        break;
      case ENOFILE:
        err_message = "File wasn't found";
        break;
      case EFILEDEL:
        err_message = "Attempting to use deleted file";
        break;
      case EINVALIDFD:
        err_message = "Attempting to use invalid file descriptor";
        break;
      case EINVALIDPARAMETER:
        err_message = "Invalid parameter for the command";
        break;
      case EUSEDFILE:
        err_message = "Attempting to delete a file used by other process";
        break;

      // ..
      default:
        err_message = "Invalid value of errno";
        break;
    }
  }
  write(STDERR_FILENO, err_message, strlen(err_message));
  write(STDERR_FILENO, "\n", 1);
}
