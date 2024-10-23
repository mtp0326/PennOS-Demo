Welcome to PennOS, a project designed to simulate the functionalities of a UNIX-like operating system. This project was created by Aaron Tsui, Matt Park, Joesph Cho, and Maya Huizar. The following repository only contains the executables.

Kernel: Matt Park, Maya Huizar

File System: Aaron Tsui, Joseph Cho

## Overview

PennOS is a UNIX-like operating system that runs as a guest OS within a single process on a host OS. It includes implementations of a basic priority scheduler, a FAT file system (PennFAT), and user shell interactions.

## How to run

Run a container in Docker Desktop and enter the correct file path, "PennOS-Demo" in the shell.
Then type:
`bin/pennos minfs`

If the symbol $ appears, you are in the PennOS shell!

## Documentation

- [**Kernel Documentation**](doc/kernel.md): Here is an overview of the Kernel, Scheduler, and Shell, and its related functions and structure. 
- [**FAT Documentation**](doc/fat.md): Here is an overview of the structure of the filesystem, and its related functions.
