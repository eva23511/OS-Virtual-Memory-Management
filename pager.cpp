//
//  pager.cpp
//  mmu
//
//  Created by 賴郁文 on 2020/12/4.
//  Copyright © 2020 賴郁文. All rights reserved.
//

#include "pager.hpp"
#include <stdio.h>
#include <vector>

using namespace std;

// FIFO Pager
FifoPager::FifoPager() : Pager() {
    hand = -1;
}

Frame * FifoPager::SelectVictimFrame(vector<Frame *> frame_table, vector<Process *> processes, unsigned long timestamp) {
    hand++;
    if (hand >= frame_table.size()) {
        hand = 0;
    }
    return frame_table[hand];
}

void FifoPager::ResetPager(int idx) {
}

// Random Pager
RandomPager::RandomPager(vector<int> randoms) : Pager(), ofs(-1), randoms(randoms) {
    hand = -1;
}

Frame * RandomPager::SelectVictimFrame(vector<Frame *> frame_table, vector<Process *> processes, unsigned long timestamp) {
    ofs++;
    
    if (ofs >= randoms.size()) {
        ofs = 0;
    }
    
    hand = randoms[ofs] % frame_table.size();
    return frame_table[hand];
}

void RandomPager::ResetPager(int idx) {
}

// Clock Pager
ClockPager::ClockPager() : Pager() {
    hand = -1;
}

Frame * ClockPager::SelectVictimFrame(vector<Frame *> frame_table, vector<Process *> processes, unsigned long timestamp) {
    hand++;
    if (hand >= frame_table.size()) {
        hand = 0;
    }
    
    while (processes[frame_table[hand]->pid]->page_table[frame_table[hand]->vpage]->referenced) {
        processes[frame_table[hand]->pid]->page_table[frame_table[hand]->vpage]->referenced = 0;

        hand++;
        if (hand >= frame_table.size()) {
            hand = 0;
        }
    }
    return frame_table[hand];
}

void ClockPager::ResetPager(int idx) {
}

// NRU(Enhanced Second Chance) Pager
NruPager::NruPager(int a_option) : Pager(), a_option(a_option), timestamp(0) {
    hand = -1;
}

Frame * NruPager::SelectVictimFrame(vector<Frame *> frame_table, vector<Process *> processes, unsigned long timestamp) {
    hand++;
    if (hand >= frame_table.size()) {
        hand = 0;
    }
    
    int beginning_hand = hand; // hand at beginning of select function
    int reset_rbit = (timestamp - this->timestamp >= 50); // whether reference bit has to be reset
    int lowest_class = 4;
    int victim_frame = hand; // Victim frame selected
    int scanned_num = 0; // Number of frames scanned

    if (reset_rbit) {
        this->timestamp = timestamp;
    }

    do {
        int new_class = GetClass(processes[frame_table[hand]->pid]->page_table[frame_table[hand]->vpage]);

        if (new_class < lowest_class) {
            lowest_class = new_class;
            victim_frame = hand;
        }
        
        scanned_num++;
        
        if (lowest_class == 0 && !reset_rbit) {
            break;
        }
        
        if (reset_rbit) {
            processes[frame_table[hand]->pid]->page_table[frame_table[hand]->vpage]->referenced = 0;
        }
        
        hand++;
        if (hand >= frame_table.size()) {
            hand = 0;
        }
    } while (hand != beginning_hand);
    
    hand = victim_frame;
    if (a_option) {
        printf("ASELECT: %d %d | %d %d %d\n", beginning_hand, reset_rbit, lowest_class, hand, scanned_num);
    }
    return frame_table[hand];
}

int NruPager::GetClass(Pte * pte) {
    return pte->referenced * 2 + pte->modified;
}

void NruPager::ResetPager(int idx) {
}

// Aging Pager
AgingPager::AgingPager(int a_option, int num_frames) : Pager(), a_option(a_option) {
    hand = -1;
    age_table.resize(num_frames, 0);
}

Frame * AgingPager::SelectVictimFrame(vector<Frame *> frame_table, vector<Process *> processes, unsigned long timestamp) {
    hand++;
    if (hand >= frame_table.size()) {
        hand = 0;
    }
    
    if (a_option) {
        printf("ASELECT %d-%lu |", hand, (hand + frame_table.size() - 1) % frame_table.size());
    }
    
    int scan_from = hand;
    unsigned int min_age = UINT32_MAX;
    int victim_frame = hand;
    
    do {
        age_table[hand] = age_table[hand] >> 1;
        if (processes[frame_table[hand]->pid]->page_table[frame_table[hand]->vpage]->referenced) {
            age_table[hand] = (age_table[hand] | 0x80000000);
            processes[frame_table[hand]->pid]->page_table[frame_table[hand]->vpage]->referenced = 0;
        }
        
        if (age_table[hand] < min_age) {
            min_age = age_table[hand];
            victim_frame = hand;
        }
        
        if (a_option) {
            printf(" %d:%x", hand, age_table[hand]);
        }
        
        hand++;
        if (hand >= frame_table.size()) {
            hand = 0;
        }
    } while (hand != scan_from);
    
    hand = victim_frame;
    age_table[hand] = 0;
    if (a_option) {
        printf(" | %d\n", hand);
    }
    return frame_table[hand];
}

void AgingPager::ResetPager(int idx) {
    ResetAge(idx);
}

void AgingPager::ResetAge(int idx) {
    age_table[idx] = 0;
}

// Working Set Pager
WorkingSetPager::WorkingSetPager(int a_option) : Pager(), a_option(a_option) {
    hand = -1;
}

Frame * WorkingSetPager::SelectVictimFrame(vector<Frame *> frame_table, vector<Process *> processes, unsigned long timestamp) {
    hand++;
    if (hand >= frame_table.size()) {
        hand = 0;
    }
    
    if (a_option) {
        printf("ASELECT %d-%lu |", hand, (hand + frame_table.size() - 1) % frame_table.size());
    }
    
    int scan_from = hand;
    int victim_frame = hand; // Victim frame selected
    int scanned_num = 0; // Number of frames scanned
    int max_age = 0; // oldest “time_last_used” frame
    
    do {
        scanned_num++;
        Pte * pte = processes[frame_table[hand]->pid]->page_table[frame_table[hand]->vpage];
        
        if (a_option) {
            printf(" %d(%d %d:%d %d", hand, pte->referenced, frame_table[hand]->pid, frame_table[hand]->vpage, frame_table[hand]->timestamp);
        }
        
        // R = 0 && age > τ -> select this frame
        if (!pte->referenced && timestamp - frame_table[hand]->timestamp > kTau) {
            victim_frame = hand;
            if (a_option) {
                printf(" STOP(%d)", scanned_num);
            }
            break;
        }
        
        if (!pte->referenced && timestamp - frame_table[hand]->timestamp > max_age) {
            victim_frame = hand;
            max_age = (int)timestamp - frame_table[hand]->timestamp;
        }
        
        if (pte->referenced) {
            pte->referenced = 0;
            frame_table[hand]->timestamp = (unsigned int)timestamp;
        }
        
        hand++;
        if (hand >= frame_table.size()) {
            hand = 0;
        }
    } while (hand != scan_from);
    
    hand = victim_frame;
    if (a_option) {
        printf(" | %d\n", hand);
    }
    return frame_table[hand];
}

void WorkingSetPager::ResetPager(int idx) {
}
