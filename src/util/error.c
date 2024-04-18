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
            case 1:
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

