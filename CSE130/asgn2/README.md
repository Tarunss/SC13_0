#Assignment 2 directory

This directory contains source code and other files for Assignment 2.

Use this README document to store notes about design, testing, and
questions you have while developing your assignment.


This assignment 2 is about building a server that can handle many commands, but only do get and put operations. Specifications on these operations can be found on the assignment 2 document. 

In terms of producing this assignment, I will be splitting it into a couple of modules in order to help me work on different things, without necessarily needing them to all connect at once right away.
- ok well I thought I would do that but I might implement modularity later after I get the assignment working
Note that the assignment must keep running even after recieving bad inputs, so set everything to null and return to main when this happens

The first module will handle getting an input from a connector. This will use sockets,
.....
Ok, I finished most of it, all the planning was done on my IPAD which is why this README is kind of empty
Here are the sources that I ended up using;
Man(2) Read, Write, Stat

For all of the regex, I used man(7) regex, but most of the info came from testing + the practica

There are many ideas that I implemented from stack overflow, but the ones that come to mind were
- Using snprintf to cast numbytes so that I could create an efficiently sized array
https://stackoverflow.com/questions/8257714/how-to-convert-an-int-to-string-in-c
- Using sscanf to cast a string into a size
https://stackoverflow.com/questions/3420629/what-is-the-difference-between-sscanf-or-atoi-to-convert-a-string-to-an-integer

- Here are some stack overflow sources that I used for STAT:
https://stackoverflow.com/questions/4553012/checking-if-a-file-is-a-directory-or-just-a-file
https://www.techiedelight.com/find-size-of-file-c/

I had like a million tabs open while I researched a variety of bugs, but most of them were actually not helpful and usually
I just had some sort of read/write error or segmentation fault, or most likely it was a null terminator I needed or didnt
need somewhere.
