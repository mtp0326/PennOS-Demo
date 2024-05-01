#include "error.h"
void u_perror(char* message) {
  char* err_message;
  if (message == NULL || strcmp(message, "\0") == 0) {
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
      case ENOPROC:
        err_message = "PCB not found";
        break;
      case EPCBSTATE:
        err_message = "Invalid state";
        break;
      case EREMOVEPROC:
        err_message = "Failed to remove PCB";
        break;
      case EINVALIDCMD:
        err_message = "Invalid command name";
        break;
      case EINVALIDCHMOD:
        err_message = "Invalid chmod";
        break;
      case EREADERROR:
        err_message = "Invalid read";
        break;
      case ENOREADPERM:
        err_message = "Invalid read, attempting to read file with no read perm";
        break;
      case ENOWRITEPERM:
        err_message = "Invalid write, attempting to write to file with no write perm";
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

// char* output;
// ssize_t result = s_write(STDOUT_FILENO, output, strlen(output));
// if (result == -1) {
//   u_perror("Failed to write to STDOUT");
// }

// char message[40];
// sprintf(message, "\n[%d] + Stopped %s\n", process->job_num,
// process->cmd_name); ssize_t result = s_write(STDOUT_FILENO, message,
// strlen(message)); if (result == -1) {
//   u_perror("Failed to write to STDOUT");
// }
