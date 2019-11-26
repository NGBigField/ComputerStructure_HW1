/* 046267 Computer Architecture - Winter 2019/20 - HW #1 */
/* Main program                  					 	 */
/* Usage: ./bp_main <trace filename>  				 	 */

/*         PASSWORD:             */
/*            compm              */


//Hallo Yotam;
//  This is Nir
//Hi shynet...........

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "bp.cpp"
#include <stdint.h>


int main()
{
    printf("88");

    unsigned btbSize = 8;
    unsigned historySize = 3;
    unsigned tagSize = 3;
    unsigned fsmState = 1;
    bool     isGlobalHist =  false;
    bool     isGlobalTable = false;
    int      Shared  = 0 ; 

    if (BP_init(btbSize, historySize, tagSize, fsmState, isGlobalHist,
                isGlobalTable, Shared) < 0)
    {
        fprintf(stderr, "Predictor init failed\n");
        exit(8);
    }
    
    
    uint32_t pc = 24;
    uint32_t targetPc = 28;
    bool taken = true;
    uint32_t dst = 64;
    BP_update(pc, targetPc, taken, dst);

   
    return 0;
}
