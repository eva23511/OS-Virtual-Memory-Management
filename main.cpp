//
//  main.cpp
//  mmu
//
//  Created by 賴郁文 on 2020/12/4.
//  Copyright © 2020 賴郁文. All rights reserved.
//

#include "process.hpp"
#include "frame.hpp"
#include "pager.hpp"
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <fstream>
#include <cstdio>
#include <sstream>
#include <vector>
#include <list>

using namespace std;

int O_option = 0;
int P_option = 0;
int F_option = 0;
int S_option = 0;
int x_option = 0;
int y_option = 0;
int f_option = 0;
int a_option = 0;
vector<Frame *> frame_table;
list<Frame *> free_frames;
unsigned long inst_count = 0;
unsigned long ctx_switches = 0;
unsigned long process_exits = 0;
unsigned long long cost = 0;

void ParseCommand(int argc, char * argv[], int &num_frames, string &algo, string &input_filename, string &rand_filename);
void ParseOptions(string option);
vector<int> ParseRandFile(string filename);
void SkipComments(ifstream& input_file, string &line);
void initFrames(int num_frames);
Pager * GetPager(string algo, vector<int> randoms, int num_frames);
vector<Process *> GetProcess(ifstream &input_file);
bool GetNextInstruction(ifstream &input_file, char &operation, int &vpage);
Frame * GetFrame(Pager * pager, vector<Process *> processes);
void ExitProcess(Process * curr_process, Pager * pager);
void Simulation(ifstream &input_file, Pager * pager, vector<Process *> processes);
void PrintPagetable(vector<Process *> processes);
void PrintFrameTable();
void PrintSummary(vector<Process *> processes);

int main(int argc, char * argv[]) {
    string input_filename;
    string rand_filename;
    int num_frames = 0;
    string algo;
    
    ParseCommand(argc, argv, num_frames, algo, input_filename, rand_filename);
    
    // get randoms
    vector<int> randoms = ParseRandFile(rand_filename);
    
    // initialzie frames and pager
    initFrames(num_frames);
    Pager * pager = GetPager(algo, randoms, num_frames);
    
    
    // parse input_file and Initialzie process
    ifstream input_file (input_filename);
    vector<Process *> processes = GetProcess(input_file);
    
    Simulation(input_file, pager, processes);
    
    if (P_option) {
        PrintPagetable(processes);
    }
    
    if (F_option) {
        PrintFrameTable();
    }
    
    if (S_option) {
        PrintSummary(processes);
    }
    
    input_file.close();
    return 0;
}

void Simulation(ifstream &input_file, Pager * pager, vector<Process *> processes) {
    char operation;
    int vpage;
    Process * curr_process = nullptr;
    
    while (GetNextInstruction(input_file, operation, vpage)) {
        
        if (O_option) {
            printf("%ld: ==> %c %d\n", inst_count, operation, vpage);
        }
        inst_count++;
        
        // handle special case of “c”: context switch to process #<procid>
        if (operation == 'c') {
            ctx_switches++;
            curr_process = processes[vpage];
            continue;
        }
        
        // handle special case of “e”: exit current process
        if (operation == 'e') {
            process_exits++;
            ExitProcess(curr_process, pager);
            curr_process = nullptr;
            continue;
        }
        
        Pte * curr_pte = curr_process->page_table[vpage];
        
        // the page is not present, raise a page fault exception
        if (!curr_pte->present) {
            
            // the vpage can't be accessed, i.e. it is not part of one of the VMAs
            if (!curr_process->InVmas(vpage)) {
                curr_process->segv_count++;
                if (O_option) {
                    printf(" SEGV\n");
                }
                continue;
            }
            
            // Select a frame to replace
            Frame * curr_frame = GetFrame(pager, processes);
            
            // curr_frame is not free currently
            if (curr_frame->pid != -1) {
                
                // Unmap its current user (UNMAP)
                processes[curr_frame->pid]->unmap_count++;
                if (O_option) {
                    printf(" UNMAP %d:%d\n", curr_frame->pid, curr_frame->vpage);
                }
                Pte * pre_ptd = processes[curr_frame->pid]->page_table[curr_frame->vpage];
                pre_ptd->present = 0;
                
                // Save frame to disk (OUT / FOUT)
                if (pre_ptd->modified) {
                    pre_ptd->modified = 0;
                    pre_ptd->physical_frame = 0;
                    
                    if (pre_ptd->file_mapped) {
                        processes[curr_frame->pid]->fout_count++;
                        if (O_option) {
                            printf(" FOUT\n");
                        }
                    } else {
                        pre_ptd->pagedout = 1;
                        processes[curr_frame->pid]->out_count++;
                        if (O_option) {
                            printf(" OUT\n");
                        }
                    }
                }
            }
            
            // Fill frame with proper content of current instruction’s address space (IN, FIN,ZERO)
            curr_frame->pid = curr_process->pid;
            curr_frame->vpage = vpage;
            
            if (curr_pte->file_mapped) {
                curr_process->fin_count++;
                if (O_option) {
                    printf(" FIN\n");
                }
            } else if (curr_pte->pagedout) {
                curr_process->in_count++;
                if (O_option) {
                    printf(" IN\n");
                }
            } else {
                curr_process->zero_count++;
                if (O_option) {
                    printf(" ZERO\n");
                }
            }
            
            // Map its new user (MAP)
            curr_process->map_count++;
            curr_pte->physical_frame = curr_frame->idx;
            if (O_option) {
                printf(" MAP %d\n", curr_frame->idx);
            }
        }
        
        // check write protection: page is considered referenced but not modified
        if (operation == 'w' && curr_pte->write_protect) {
            curr_process->segprot_count++;
            if (O_option) {
                printf(" SEGPROT\n");
            }
        }
        
        // updating the R/M PTE bits
        curr_pte->UpdatePte(operation); // update_pte(read/modify) bits based on operations
        
        if (y_option) {
            PrintPagetable(processes);
        } else if (x_option) {
            curr_process->PrintPagetable();
        }
        
        if (f_option) {
            PrintFrameTable();
        }
    }
}

void ExitProcess(Process * curr_process, Pager * pager) {
    printf("EXIT current process %d\n", curr_process->pid);
    for (Pte * pte : curr_process->page_table) {
        if (pte->present) {
            // Unmap the page (UNMAP)
            Frame * curr_frame = frame_table[pte->physical_frame];
            curr_process->unmap_count++;
            if (O_option) {
                printf(" UNMAP %d:%d\n", curr_frame->pid, curr_frame->vpage);
            }
                        
            // Save frame to disk (FOUT): FOUT modified filemapped pages
            // Note that dirty non-fmapped (anonymous) pages are not written back (OUT) as the process exits.
            if (pte->modified && pte->file_mapped) {
                curr_process->fout_count++;
                if (O_option) {
                    printf(" FOUT\n");
                }
            }
            // Reset the used frame and Returned it to the free pool
            curr_frame->ResetFrame();
            pager->ResetPager(pte->physical_frame);
            free_frames.push_back(curr_frame);
        }
        pte->ResetPte();
    }
    
}

Frame * GetFrame(Pager * pager, vector<Process *> processes) {
    if (free_frames.empty()) {
        return pager->SelectVictimFrame(frame_table, processes, inst_count);
    }
    Frame * frame = free_frames.front();
    free_frames.pop_front();
    return frame;
}

bool GetNextInstruction(ifstream &input_file, char &operation, int &vpage) {
    string line;
    SkipComments(input_file, line);
    
    if (input_file.eof() || line[0] == '#') {
        return false;
    }
    
    istringstream input(line);
    input >> operation >> vpage;
    return true;
}

Pager * GetPager(string algo, vector<int> randoms, int num_frames) {
    if (algo == "f") {
        return new FifoPager();
    } else if (algo == "r") {
        return new RandomPager(randoms);
    } else if (algo == "c") {
        return new ClockPager();
    } else if (algo == "e") {
        return new NruPager(a_option);
    } else if (algo == "a") {
        return new AgingPager(a_option, num_frames);
    } else if (algo == "w") {
        return new WorkingSetPager(a_option);
    }
    
    printf("Wrong Pager Specification: %s\n", algo.c_str());
    exit(EXIT_FAILURE);
}

// init frame_table and free_frames
void initFrames(int num_frames) {
    for (int i = 0; i < num_frames; i++) {
        frame_table.push_back(new Frame(i));
        free_frames.push_back(frame_table[i]);
    }
}

vector<Process *> GetProcess(ifstream &input_file) {
    vector<Process *> processes;
    string line;
    int process_num;
    int vma_num;
    int s, e, w, f;
    
    SkipComments(input_file, line);
    
    istringstream input(line);
    input >> process_num;
    
    for (int i = 0; i < process_num; i++) {
        SkipComments(input_file, line);
        Process * process = new Process(i);
        istringstream input(line);
        input >> vma_num;
        
        for (int j = 0; j < vma_num; j++) {
            getline(input_file, line);
            istringstream input(line);
            input >> s >> e >> w >> f;
            process->AddVma(s, e, w, f);
        }
        processes.push_back(process);
    }
    return processes;
}

// skip comments that start with '#'
void SkipComments(ifstream& input_file, string &line) {
    while (getline(input_file, line)) {
        if (line[0] != '#') {
            return;
        }
    }
}

// parse random
vector<int> ParseRandFile(string filename) {
    ifstream rand_file(filename);
    
    if (!rand_file.is_open()) {
        printf("Could not open rand file: %s\n", filename.c_str());
        exit (EXIT_FAILURE);
    }
    
    int num;
    vector<int> randoms;
    
    rand_file >> num;
    while (rand_file >> num) {
        randoms.push_back(num);
    }
    
    rand_file.close();
    return randoms;
}

void ParseCommand(int argc, char * argv[], int &num_frames, string &algo, string &input_filename, string &rand_filename) {
    
    if (argc < 3) {
        printf("Use this to run: ./mmu –f<num_frames> -a<algo> [-o<options>] inputfile randomfile\n");
        exit (EXIT_FAILURE);
    }
    
    int c;
    while ((c = getopt (argc, argv, "a:f:o:")) != -1) {
        switch(c) {
            case 'f':
                num_frames = atoi(optarg);
                break;
            case 'a':
                algo = string(optarg);
                break;
            case 'o':
                ParseOptions(string(optarg));
                break;
            case '?':
                printf("Unknown Option -%c\n", optopt);
                printf("Use this to run: ./mmu –f<num_frames> -a<algo> [-o<options>] inputfile randomfile\n");
                exit (EXIT_FAILURE);
                break;
        }
    }
    
    for (int index = optind; index < argc; index++) {
        if (index == argc - 2) {
            input_filename = argv[index];
        } else if (index == argc - 1) {
            rand_filename = argv[index];
        } else {
            printf("Non-option argument %s\n", argv[index]);
            printf("Use this to run: ./mmu –f<num_frames> -a<algo> [-o<options>] inputfile randomfile\n");
        }
    }
}

void ParseOptions(string options) {
    for (int i = 0; options[i]; i++) {
        switch (options[i]) {
            case 'O':
              O_option = 1;
              break;
            case 'P':
              P_option = 1;
              break;
            case 'F':
              F_option = 1;
              break;
            case 'S':
              S_option = 1;
              break;
            case 'x':
              x_option = 1;
              break;
            case 'y':
              y_option = 1;
              break;
            case 'f':
              f_option = 1;
              break;
            case 'a':
              a_option = 1;
              break;
        }
    }
}

void PrintSummary(vector<Process *> processes) {
    for (Process * p : processes) {
        p->PrintStatistics();
        cost += p->ComputeCost();
    }
    
    cost += (long long)(inst_count - ctx_switches - process_exits) + (long long)(ctx_switches) * 121 + (long long)(process_exits) * 175;
    
    printf("TOTALCOST %lu %lu %lu %llu\n", inst_count, ctx_switches, process_exits, cost);
}

void PrintPagetable(vector<Process *> processes) {
    for (Process * p: processes) {
        p->PrintPagetable();
    }
}

void PrintFrameTable() {
    printf("FT:");
    for (Frame * f : frame_table) {
        if (f->pid != -1) {
            printf(" %d:%d", f->pid, f->vpage);
        } else {
            printf(" *");
        }
    }
    printf("\n");
}
