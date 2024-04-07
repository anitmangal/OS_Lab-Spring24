# Define the C compiler to use
CC = gcc

# Define any compile-time flags
CFLAGS = -Wall -g

# Define the target files
TARGETS = master mmu scheduler process

# Define the default rule to run the program
run: $(TARGETS)
	./master

master: master.c
	$(CC) $(CFLAGS) -o master master.c

mmu: mmu.c
	$(CC) $(CFLAGS) -o mmu mmu.c

scheduler: sched.c
	$(CC) $(CFLAGS) -o scheduler sched.c

process: process.c
	$(CC) $(CFLAGS) -o process process.c

# Define a clean rule to remove the target files
clean:
	rm -f $(TARGETS) result.txt