# OS Project 3: Concurrent Threads

This project implements a simulation of a mentoring center using POSIX threads, mutex locks, and semaphores. It demonstrates the synchronization of students, tutors, and a coordinator in a concurrent programming environment.

## Problem Statement

The mentoring center has:
- **Students** seeking help with programming assignments.
- **Tutors** assisting students in a synchronized manner.
- A **Coordinator** managing the queue of waiting students based on priority.

### Key Features:
- Students wait in a **priority queue**, managed by the coordinator.
- Tutors assist students based on priority, ensuring fairness.
- **Synchronization** is achieved using semaphores and mutex locks.
- The program dynamically handles any number of students, tutors, and chairs.

## How to Compile and Run

### Compilation
```bash
gcc main.c -o csmc -Wall -Werror -pthread -std=gnu99
