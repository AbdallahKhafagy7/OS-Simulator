build:
	gcc process_generator.c  Processes_DataStructure/process_queue.c Processes_DataStructure/process_priority_queue.c Processes_DataStructure/process.h -o process_generator.out
	gcc clk.c -o clk.out
	gcc scheduler.c Processes_DataStructure/process_queue.c Processes_DataStructure/process_priority_queue.c Processes_DataStructure/process.h -o scheduler.out
	gcc scheduler.c \
    Processes_DataStructure/process_queue.c \
    Processes_DataStructure/process_priority_queue.c \
    -o scheduler.out
	gcc process.c -o process.out
	

clean:
	rm -f *.out   scheduler_log.txt
	rm -f *.o     scheduler_log.txt
all: clean build

run:
	./process_generator.out processes.txt 
