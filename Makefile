build:
	gcc process_generator.c  Processes_DataStructure/process_queue.c Processes_DataStructure/process_priority_queue.c Processes_DataStructure/process.h -o process_generator.out
	gcc clk.c -o clk.out
	gcc scheduler.c -o scheduler.out
	gcc process.c -o process.out
	gcc test_generator.c -o test_generator.out

clean:
	rm -f *.out  processes.txt

all: clean build

run:
	./process_generator.out test.txt
