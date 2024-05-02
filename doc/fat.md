# Filesystem Team: Aaron Tsui and Joseph Cho

## List of Submitted Source Files for Standalone FAT:

* parser.c and parser.h
* pennfat.c and pennfat.h
* pennfat_kernel.c and pennfat_kernel.h
* standalonefat.c

## List of Submitted Source Files for Integrated FAT with Kernel:

* parser.c and parser.h
* pennfat.c and pennfat.h
* pennfat_kernel.c and pennfat_kernel.h
* sys_call.c and sys_call.h
* shellbuiltins.c and shellbuiltins.h
* error.c and error.h
* bitmap.c and bitmap.h
* All other kernel files necessary for scheduling

## Compilation Instructions:

## For Standalone FAT:
* First, use the make commmand to compile the sttandalonefat.
* Then, type ./bin/standalonefat in the terminal and press enter

## For Integrated FAT with Kernel:
* First, use the make commmand to compile the sttandalonefat.
* Then, type ./bin/standalonefat in the terminal and press enter
* Create a filesystem with command such as "mkfs minfs 1 0"
* Exit the standalone fat 
* Use ./bin/pennos "fs_name" to start the pennos with "fs_name" mounted (e.g. ./bin/pennos minfs)

## Overview of Work Accomplished:

### Standalone FAT

Standalone FAT is based on FAT 16. It supports a root directory that can store multiple text files. It is interacts with the user using the routine commands which is described in the project description. The implementation of the Standalone FAT is abstracted away from the user. That is, it is not possible for the user to directly utilize the kernel level functions to interact with the FAT system. This reduces the overhead of the user that is using this FAT system so that they can focus on maintaining the data they are interested in.

In the high level, the Standalone FAT is maintained by the FAT region and the DATA region. FAT region is stored in memory
so that the system can easily access and index the DATA region. DATA region is divided into the user-set block size. As the data is written or deleted from the DATA region, the Standalone FAT maintains the soundness of the system by modifying and updating both the FAT region and the DATA region. 

### Integrated FAT

The Integrated FAT is mounted on to the pennos over the terminal. Once it is mounted, the user can interact with the file system just like it would interact with the unix terminal. All of the commands that are FAT related are scheduled by the scheduler and logged accordingly.


## Description of Code and Code Layout:

### Standalone FAT
pennfat_kernel.c : All kernel level functions that directly interacts with the FAT system. It it also the only place we use system level functions such as write and read. It is abstracted away from the user, so that they don't have to worry about the actualy file descriptor table or numbers. 

pennfat.c : Functions that call the kernel level functions to carry out the routine for the Standalone FAT. This level effectively abstracts the detail of the FAT implementation from the user.

standalone.c : "Shell" for the Standalone FAT. It continuously takes in user input through the terminal and carry them out.

### Integrated FAT
pennfat_kernel.c : All kernel level functions that directly interacts with the FAT system. It it also the only place we use system level functions such as write and read. It is abstracted away from the user, so that they don't have to worry about the actualy file descriptor table or numbers. 

sys_call.c : Functions that call the kernel level functions to carry out the routine for the Standalone FAT. This level effectively abstracts the detail of the FAT implementation from the user. Also, on error of the kernel level functions, all system level functions sets the errno, so that u_perror can successfully.

shellbuiltins.c : Functions that call the system level functions to carry out the built-in features. All built-in level functions aren't called "directly" by our host system. It is always scheduled by the scheduler in order for it to be executed.