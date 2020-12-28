//
//  frame.hpp
//  mmu
//
//  Created by 賴郁文 on 2020/12/4.
//  Copyright © 2020 賴郁文. All rights reserved.
//

#ifndef frame_hpp
#define frame_hpp

#include <stdio.h>

struct Frame {
    int idx;
    int pid;
    int vpage;
    unsigned int timestamp;
    
    Frame(int idx);
    void ResetFrame();
};

#endif /* frame_hpp */
