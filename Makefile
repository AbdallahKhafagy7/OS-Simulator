build:
	gcc process_generator.c  Processes_DataStructure/process_queue.c Processes_DataStructure/process_priority_queue.c Processes_DataStructure/process.h -o process_generator.out
	gcc clk.c -o clk.out
	gcc scheduler.c Processes_DataStructure/process_queue.c Processes_DataStructure/process_priority_queue.c Processes_DataStructure/process.h -o scheduler.out
	gcc scheduler.c \
    Processes_DataStructure/process_queue.c \
    Processes_DataStructure/process_priority_queue.c \
    -o scheduler.out
	gcc test_generator.c -o test_generator.out 
	gcc process.c -o process.out
	

clean:
	rm -f *.out    scheduler.log scheduler.perf
	rm -f *.o     scheduler.log scheduler.perf
all: clean build

run:
	
	./process_generator.out processes.txt 
