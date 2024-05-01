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

## For Standlone FAT:
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



## Description of Code and Code Layout:

### Standlone FAT



### Integrated FAT



## General Comments: N/A