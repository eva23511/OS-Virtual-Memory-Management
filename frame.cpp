//
//  frame.cpp
//  mmu
//
//  Created by 賴郁文 on 2020/12/4.
//  Copyright © 2020 賴郁文. All rights reserved.
//

#include "frame.hpp"

Frame::Frame(int idx): idx(idx), pid(-1), vpage(-1), timestamp(-1) {
}

void Frame::ResetFrame() {
    pid = -1;
    vpage = -1;
    timestamp = -1;
}
