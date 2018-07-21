# cs537-code

Programs
xcheck.c
This program is a working file system checker. A checker reads in a file system image and makes sure that it is consistent.

pzip.c
This is a multithreaded run-length encoding compression program. Takes a file and compresses it using multiple worker threads. 

xv6 lottery scheduler
This program takes the standard xv6 program and changes the scheduler so that it uses a lottery scheduler with tickets. There is a graph inside demonstrating the results.

xv6 Multithreaded
This program takes the standard xv6 program and adds multithreaded features. Xv6 has a fork() function that splits code. This version has a clone() function that does a similar process except instead the clones share an address space with the parent. Includes many other differentiating factors from the fork() function.
