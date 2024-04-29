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
