#include "error.h"

void u_perror(char* message) {
    char* err_message;
    if (message == NULL) {
        return;
    } else if (strcmp(message, "\0") == 0) {
        return;
    } else {
        switch (errno) {
            case 0:
                break;
            case 1:
                break;
            // ..
            default:
                write(STDERR_FILENO, message, strlen(message));
                err_message = "Invalid value of errno";
                write(STDERR_FILENO, err_message, strlen(err_message));
                break;
        }    
    }
}

