//
//  process.cpp
//  mmu
//
//  Created by 賴郁文 on 2020/12/4.
//  Copyright © 2020 賴郁文. All rights reserved.
//

#include "process.hpp"
#include <iostream>

using namespace std;

Process::Process(int pid): pid(pid), segv_count(0), segprot_count(0), unmap_count(0), map_count(0), in_count(0), fin_count(0), out_count(0), fout_count(0), zero_count(0) {
    for (int i = 0; i < kVpageSize; i++) {
        page_table.push_back(new Pte());
    }
}

void Process::AddVma(int s, int e, int w, int f) {
    Vma * new_vma = new Vma{s, e, w, f};
    vmas.push_back(new_vma);
}

bool Process::InVmas(int vpage) {
    for (Vma * vma: vmas) {
        if (vpage >= vma->starting_vpage && vpage <= vma->ending_vpage) {
            page_table[vpage]->write_protect = vma->write_protected;
            page_table[vpage]->file_mapped = vma->filemapped;
            return true;
        }
    }
    return false;
}

void Process::PrintStatistics() {
    printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n", pid,
    unmap_count, map_count, in_count, out_count, fin_count, fout_count, zero_count, segv_count, segprot_count);
}

unsigned long long Process::ComputeCost() {
    return (long long)(map_count + unmap_count) * 400 + (long long)(in_count + out_count) * 3000 + (long long)(fin_count + fout_count) * 2500 + (long long)(zero_count) * 150 + (long long)(segv_count) * 240 + (long long)(segprot_count) * 300;
}

void Process::PrintPagetable() {
    printf("PT[%d]:", pid);
    for(int i = 0; i < kVpageSize; i++) {
        if (!page_table[i]->present) {
            if (page_table[i]->pagedout) {
                printf(" #");
            } else {
                printf(" *");
            }
            continue;
        }
        printf(" %d:%c%c%c", i, page_table[i]->referenced ? 'R' : '-', page_table[i]->modified ? 'M' : '-', page_table[i]->pagedout ? 'S' : '-');
    }
    printf("\n");
}
