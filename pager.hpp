//
//  pager.hpp
//  mmu
//
//  Created by 賴郁文 on 2020/12/4.
//  Copyright © 2020 賴郁文. All rights reserved.
//

#ifndef pager_hpp
#define pager_hpp

#include "frame.hpp"
#include "process.hpp"
#include <stdio.h>
#include <stdint.h>
#include <vector>

using namespace std;

// virtual base class
class Pager {
protected:
    int hand;
public:
    virtual Frame * SelectVictimFrame(vector<Frame *> frame_table, vector<Process *> processes, unsigned long timestamp) = 0;
    virtual void ResetPager(int idx) = 0;
};

class FifoPager : public Pager {
public:
    FifoPager();
    Frame * SelectVictimFrame(vector<Frame *> frame_table, vector<Process *> processes, unsigned long timestamp);
    void ResetPager(int idx);
};

class RandomPager : public Pager {
    int ofs;
    vector<int> randoms;
public:
    RandomPager(vector<int> randoms);
    Frame * SelectVictimFrame(vector<Frame *> frame_table, vector<Process *> processes, unsigned long timestamp);
    void ResetPager(int idx);
};

class ClockPager : public Pager {
public:
    ClockPager();
    Frame * SelectVictimFrame(vector<Frame *> frame_table, vector<Process *> processes, unsigned long timestamp);
    void ResetPager(int idx);
};

class NruPager : public Pager {
    int a_option;
    unsigned long timestamp;
public:
    NruPager(int a_option);
    Frame * SelectVictimFrame(vector<Frame *> frame_table, vector<Process *> processes, unsigned long timestamp);
    int GetClass(Pte * pte);
    void ResetPager(int idx);
};

class AgingPager : public Pager {
    int a_option;
    vector<unsigned int> age_table; // each active page (stick into each frame) has its own age
public:
    AgingPager(int a_option, int num_frames);
    Frame * SelectVictimFrame(vector<Frame *> frame_table, vector<Process *> processes, unsigned long timestamp);
    void ResetPager(int idx);
    void ResetAge(int idx);
};

class WorkingSetPager : public Pager {
    int a_option;
    const int kTau = 49;
public:
    WorkingSetPager(int a_option);
    Frame * SelectVictimFrame(vector<Frame *> frame_table, vector<Process *> processes, unsigned long timestamp);
    void ResetPager(int idx);
};


#endif /* pager_hpp */
