#------------------------------------------------------------------------------
# Makefile for CSE 101 Programming Assignment 1
#
# make                   makes Lex
# make ListClient        makes ListClient
# make clean             removes all binaries
# make checkClient       checks ListClient for memory errors
# make checkLex          checks Lex for memory errors
#------------------------------------------------------------------------------

# Hello : Hello.o
# 	clang -std=c17 -Wall -o Hello Hello.o
all: hello

hello : hello.c
	clang -std=c17 -Wall -o hello hello.c

hello.o: hello.c
	clang -std=c17 -Wall -c hello.c
# ListClient: ListClient.o List.o
# 	clang -std=c17 -Wall -o ListClient ListClient.o List.o

# ListClient.o : ListClient.c List.h
# 	clang -std=c17 -Wall -c ListClient.c

# List.o : List.c List.h
# 	clang -std=c17 -Wall -c List.c

clean :
	rm -f hello.o hello

# checkClient : 
# 	valgrind --leak-check=full ListClient

# checkLex :
# 	valgrind  --leak-check=yes Lex File1 File2