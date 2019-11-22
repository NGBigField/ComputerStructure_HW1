/* 046267 Computer Architecture - Winter 2019/20 - HW #1 */
/* Main program                  					 	 */
/* Usage: ./bp_main <trace filename>  				 	 */

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
    
    unsigned btbSize = 4;
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
    
    /*
    uint32_t pc = 16;
    uint32_t dst = 43;
    bool is_taken =BP_predict(pc, &dst);

    printf("%d" , (int)is_taken);
    */
   
    return 0;
}
