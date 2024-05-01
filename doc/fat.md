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
* First, just type make in the terminal and press enter
* Then, type ./bin/standalonefat in the terminal and press enter

## For Integrated FAT with Kernel:
* First, just type make in the terminal and press enter
* Then, type ./bin/standalonefat in the terminal and press enter
* Make a filesystem with command such as "mkfs minfs 1 0"
* Then exit and type ./bin/pennos "fs_name" (e.g. ./bin/pennos minfs)

## Overview of Work Accomplished:

### Standlone FAT



### Integrated FAT



## Description of Code and Code Layout:

### Standlone FAT



### Integrated FAT



## General Comments: N/A