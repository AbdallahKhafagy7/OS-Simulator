# 🖥️ Operating System Scheduling & Memory Simulator

Welcome to my **OS Simulator** project! This university-level project, implemented in **C**, brings classic operating system concepts to life. It simulates CPU scheduling, process lifecycle management, and paging-based memory management (for Round Robin), including **Second Chance** and **LRU** page replacement algorithms.

---

## 🚀 Features & Contributions

### CPU Scheduling Algorithms

* **Highest Priority First (HPF)** with dependency handling
* **Shortest Remaining Time Next (SRTN)**
* **Round Robin (RR)** with paging-based memory management (Second Chance & LRU)

### Process Lifecycle Management

* Tracks process states: New, Ready, Running, Blocked, Finished
* Implements context switching and preemption logic
* Handles process arrival and termination

### Project Enhancements

* Detailed documentation and setup instructions
* Usage examples and walkthroughs
* Refactored code for clarity and maintainability

---

## 🛠️ Build & Run Instructions

The project uses a **Makefile** for building and running.

### 1. Build the project

```bash
make build
```

Cleans old builds and compiles all source files into executables.

### 2. Run the simulator

```bash
make run
```

Generates test data and runs the process scheduler simulation.

### 3. Clean up generated files

```bash
make clean
```

Removes `.out`, `.txt`, `.log`, `.perf`, and object files.

---

## 📂 Output Files

* `scheduler.log` → Scheduler execution details
* `memory.log` → Memory management operations
* `scheduler.perf` → Performance statistics

---

## ⚙️ Executables

* `process_generator.out` → Generates process data file
* `clk.out` → Simulates system clock
* `scheduler.out` → Runs scheduling simulation
* `test_generator.out` → Runs tests
* `process.out` → Auxiliary process operations

---

## 🧰 Technologies Used

* C (C99)
* Makefile for build automation
* Linux environment
* Data structures: queues & priority queues
* OS concepts: CPU scheduling (HPF w/ dependencies, SRTN, RR w/ Second Chance & LRU), process management, memory management

---

## 📖 Summary

This project showcases my ability to implement core OS algorithms in C, handle **dependencies in HPF scheduling**, manage memory using **Second Chance and LRU for RR**, and structure a multi-file project with automated building and testing. It includes logging and performance measurements through `scheduler.log`, `memory.log`, and `scheduler.perf`.

The simulator demonstrates a clear understanding of OS fundamentals and professional coding practices
