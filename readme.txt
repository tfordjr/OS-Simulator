Terry Ford CS 4760 Project 4 - oss 10/14/2023
Git:  https://github.com/tfordjr/OS-Simulator 

How to run my program:
make command will generate executables master and slave
./master command with required -t arg will run the program.
sample command to run the program: ./master -t 60 10
where -t arg defines how many seconds before master tires of waiting and kills all children,
and where the final arg is the number of processes created that will access critical section cstest
also, ./slave command will run just that file with sample command: ./slave 10

Implementation:
source code files are: master.c, slave.c, config.h 
master.c creates specified number of children, logs to logfile.master.PID
slave.c dictates children to take turns writing to cstest by waiting and checking repeatedly
config.h defines timeoutSeconds and maxChildren variables
Also included are readme.txt and Makefile, which uses pattern rules. 


Notable inclusions are:
perror() messages for each file open operation and for failed forks
modular design philosophy to increase readability, reusability and ease of use
Makefile with pattern rules, make operation raises no warnings. make clean removes all logfiles, executables
Appropriate comments that briefly explain implementation and methods used
Minimal shared server resources used by only utilizing 1 byte (boolean) to successfully coordinate child processes
Minimal redundant code. Most is designed modularly and reused effectively.
shared memory is a small size and always properly allocated, detached, and deallocated. 
Zombie processes are never spawned from my project.

Notable exclusions:
When incorrect number of arguments are given, program often segfaults.
cstest is commonly empty when program suffers timeout termination, I assume because child is killed with cstest open 
     and that erases the file. Not sure if that can be fixed. 
SIGINT signal successfully kills the program, I had a lot of trouble shielding the program from SIGINT and I'm not sure why it isn't working. I think master is shielded from SIGINT but child processes running ./slave are not shielded from SIGINT signal. I'm not certain if that is why ctrl+c is always successful in killing the program, I never could successfully prevent ctrl+c from killing the program.
