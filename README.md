INB365 - Systems Programming - Queensland University of Technology

Distributed Communication - Assignment 2 - 2014 (Server/Client & Producer/Consumer Example)

Created By - Lindsay Watt

Dependancies required to compile and run the Server/Client Dietician Program:
	Server.c
	Client.c
	assign2lib.c
	assign2.h
	Makefile
	calories.csv

To compile, run 'make && make clean' command in the dependancy directory.

To run the Dietician Program, the usage is:

"./Server <port>" - port is optional, defaults to 12345

Once the Server is running, up to 10 clients at a time can connect.

To run a Client:

"./Client <hostname> <port>"

if more than 10 clients attempt to connect, they will have to wait in a queue
until a thread from the thread pool is free.

To close a client, type 'q' followed by <enter>

To close the server, press Ctrl+C, the server will end all threads, free memory
and save any additions to the csv file.
