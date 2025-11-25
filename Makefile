build:
	gcc process_generator.c  Processes_DataStructure/process_queue.c Processes_DataStructure/process_priority_queue.c Processes_DataStructure/process.h -o process_generator.out
	gcc clk.c -o clk.out
	gcc scheduler.c Processes_DataStructure/process_queue.c Processes_DataStructure/process_priority_queue.c -lm -o scheduler.out

	gcc test_generator.c -o test_generator.out 
	gcc process.c -o process.out
	gcc process2.c -o process2.out
	

clean:
	rm -f *.out  processes.txt  scheduler.log scheduler.perf
	rm -f *.o   processes.txt   scheduler.log scheduler.perf
all: clean build

run:
	./test_generator.out 
	./process_generator.out processes.txt 
