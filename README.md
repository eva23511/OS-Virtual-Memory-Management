# OS-Virtual-Memory-Management
Implemented and simulated the operation of an Operating System’s Virtual Memory manager which maps the virtual address spaces of multiple processes onto physical frames using page table translation.

SOME TIPS ON HOW TO RUN THIS
----------------------------

### have to load gcc version (gcc-9.2)

1. To load gcc version, by executing the following in a terminal:

	module load gcc-9.2


2. Using makefile to compile:

	make


3. Run the program following below execution format:

	./mmu –f<num_frames> -a<algo> [-o<options>] inputfile randomfile
