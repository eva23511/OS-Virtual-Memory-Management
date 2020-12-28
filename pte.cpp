//
//  pte.cpp
//  mmu
//
//  Created by 賴郁文 on 2020/12/4.
//  Copyright © 2020 賴郁文. All rights reserved.
//

#include "pte.hpp"

Pte::Pte(): present(0), referenced(0), modified(0), write_protect(0), file_mapped(0), pagedout(0), physical_frame(0), free_space(0) {
}

void Pte::UpdatePte(char operation) {
    present = 1;
    referenced = 1;
    
    if (operation == 'w' && !write_protect) {
        modified = 1;
    }
}

void Pte::ResetPte() {
    present = 0;
    referenced = 0;
    modified = 0;
    write_protect = 0;
    file_mapped = 0;
    pagedout = 0;
    physical_frame = 0;
}
