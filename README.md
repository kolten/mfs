# mfs
Programming Assignment for reading FAT32 File System for Operating Systems
Created by Kolten Sturgill And Mary Huerta

This is a user space shell application that is capable of interpreting a FAT32 file system image called "mfs". The utility does not corrupt
the file system image. 

## To compile:

`g++ mfs.c -o mfs`
or by using the makefile located in this repo

At the start of the program it prints out a prompt of `mfs>` when it is ready to accept input.
Note: this program shall supports relative paths and absolute paths. 

## The following commands are supported:

`open <filename>`
Opens a fat32 image. Filenames of fat32 images shall not contain spaces.

`close `
Closes the fat32 image. 

`info`
Prints out information about the file system 

`stat <filename> or <directory name>`
Prints the attributes and starting cluster number of the file or directory name. 

`get <filename>`
Retrieves the file from the FAT 32 image and place it in your current working
directory. 

`cd <directory>`
Changes the current working directory to the given directory.

`ls <directory>`
Lists the directory contents. 

`volume`
Prints the volume name of the file system image
 
