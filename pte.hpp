//
//  pte.hpp
//  mmu
//
//  Created by 賴郁文 on 2020/12/4.
//  Copyright © 2020 賴郁文. All rights reserved.
//

#ifndef pte_hpp
#define pte_hpp

#include <stdio.h>

struct Pte {
    unsigned int present:1;
    unsigned int referenced:1;
    unsigned int modified:1;
    unsigned int write_protect:1;
    unsigned int file_mapped:1;
    unsigned int pagedout:1;
    unsigned int physical_frame:7;
    unsigned int free_space:19;
    
    Pte();
    void UpdatePte(char operation);
    void ResetPte();
};

#endif /* pte_hpp */
