//
//  process.hpp
//  mmu
//
//  Created by 賴郁文 on 2020/12/4.
//  Copyright © 2020 賴郁文. All rights reserved.
//

#ifndef process_hpp
#define process_hpp

#include "pte.hpp"
#include <stdio.h>
#include <vector>

using namespace std;

const int kVpageSize = 64;

struct Vma {
    int starting_vpage;
    int ending_vpage;
    int write_protected; // [0/1]
    int filemapped; // [0/1]
};

class Process {
public:
    int pid;
    vector<Vma *> vmas;
    vector<Pte *> page_table;
    
    // keep track of the number of operation
    unsigned long segv_count;
    unsigned long segprot_count;
    unsigned long unmap_count;
    unsigned long map_count;
    unsigned long in_count;
    unsigned long fin_count;
    unsigned long out_count;
    unsigned long fout_count;
    unsigned long zero_count;
    
    Process(int pid);
    void AddVma(int s, int e, int w,  int f);
    bool InVmas(int vpage);
    void PrintStatistics();
    unsigned long long ComputeCost();
    void PrintPagetable();
};

#endif /* process_hpp */
