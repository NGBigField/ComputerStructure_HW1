/* 046267 Computer Architecture - Winter 2019/20 - HW #1 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <vector>

typedef struct {
	unsigned size;         
	unsigned fsmStateAtInit;
	std::vector<pBTB_LINE> btb_lines;  //array of pointers to a BRB_LINE struct 
} BTB ;


typedef struct {
	uint32_t tag;
	uint32_t target_pc;
	uint32_t history;
	unsigned*	 fsms;
} BTB_LINE;

typedef BTB_LINE* pBTB_LINE;

extern BTB btb;


int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	
	btb.size = btbSize;
	btb.fsmStateAtInit = fsmState;
	int numStateMachines = 2^historySize;

	for ( int i=0 ; i++ ; i < btbSize)
	{	
		// need to check for errors:
		btb.btb_lines.push_back( new BTB_LINE ) ;
		btb.btb_lines[i].tag = 0;
		btb.btb_lines[i].target_pc = 0;
		btb.btb_lines[i].history = 0;
		btb.btb_lines[i].fsms = new unsigned[numStateMachines];
		for (int j = 0 ; j++ ; j < numStateMachines )
		{
			btb.btb_lines[i].fsms[j] = fsmState;
		}
	}
	return 0;
	//return -1;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	return;
}

void BP_GetStats(SIM_stats *curStats){
	return;
}

