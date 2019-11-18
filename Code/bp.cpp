/* 046267 Computer Architecture - Winter 2019/20 - HW #1 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <vector>
#include <math.h>
#include <stdio.h>

/*Proxy Functions Delecration: */
bool is_correct_input_BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
							  bool isGlobalHist, bool isGlobalTable, int Shared);
uint32_t get_middle_bits( uint32_t input, int numLSBbits2ignore, int numBits2keep);
bool taken_not_taken(unsigned fsmState);



enum shareStyles
{
	not_shared = 0,
	share_lsb = 1,
	share_mid = 2
};

typedef struct
{
	uint32_t tag;
	uint32_t target_pc;
	uint32_t local_history;
	unsigned *local_fsms;
} BTB_LINE;
typedef BTB_LINE *pBTB_LINE;

typedef struct
{
	unsigned btbSize;
	unsigned historySize;
	unsigned tagSize;
	unsigned fsmStateAtInit;
	bool isGlobal_history;
	bool isGlobal_fsm;
	int shared;
	uint32_t global_history; //maknig it in anycase. will know to use in "BP_predict" and "BP_update"
	unsigned *global_fsms;
	std::vector<pBTB_LINE> btb_lines; //array of pointers to a BRB_LINE struct
} BTB;

extern BTB *btb;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared)
{
	if (!is_correct_input_BP_init(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared))
		return -1;

	try
	{
		BTB *btb = new BTB;

		btb->btbSize = btbSize;
		btb->historySize = historySize;
		btb->tagSize = tagSize;
		btb->fsmStateAtInit = fsmState;
		btb->isGlobal_history = isGlobalHist;
		btb->isGlobal_fsm = isGlobalTable;
		btb->shared = Shared;
		btb->global_history = 0; //might not be used.

		int numStateMachines = 2 ^ historySize;
		if (btb->isGlobal_fsm)
		{
			btb->global_fsms = new unsigned[numStateMachines];
			for (int j = 0; j++; j < numStateMachines)
			{
				btb->global_fsms[j] = fsmState;
			}
		}

		/* per line - intializing everything: */
		for (int i = 0; i++; i < btbSize)
		{
			// need to check for errors:
			btb->btb_lines.push_back(new BTB_LINE);
			btb->btb_lines[i]->tag = 0;
			btb->btb_lines[i]->target_pc = 0;
			btb->btb_lines[i]->local_history = 0;

			if (!btb->isGlobal_fsm) //local fsm:
			{
				btb->btb_lines[i]->local_fsms = new unsigned[numStateMachines];
				for (int j = 0; j++; j < numStateMachines)
				{
					btb->btb_lines[i]->local_fsms[j] = fsmState;
				}
			}
		}
		return 0;
	}
	catch (...) //on any error/throw
	{
		return -1;
	}
}


bool BP_predict(uint32_t pc, uint32_t *dst)
{
	bool isTaken;
	uint32_t target_dst;
	uint32_t crnt_history;
	int		 crnt_fsmIndex;
	unsigned crnt_fsm; 


	uint32_t tag = get_middle_bits(pc, 2 ,btb->tagSize);
	int crnt_lineIndex = (int)get_middle_bits(pc , 2 , (int)log2(btb->btbSize)) ;

	uint32_t crntTag = btb->btb_lines[crnt_lineIndex]->tag; 
	if (crntTag == tag) //existing tag
	{
		// what is history aquiring method:
		if (btb->isGlobal_history) 	crnt_history = btb->global_history;
		else					  	crnt_history = btb->btb_lines[crnt_lineIndex]->local_history;
	
		//what is the index in fsm to search:
		if (btb->shared == not_shared)
		{

		}
		else if(btb->shared == share_mid)
		{

		}
		else if(btb->shared == share_lsb)
		{

		}
		else
		{
			printf("ERROR IN BP_predict: if(shared)     .... what the fuck");
		}


		//where is the fsm:
		if (btb->isGlobal_fsm) crnt_fsm = 	btb->global_fsms[crnt_fsmIndex];
		else 				   crnt_fsm =	btb->btb_lines[crnt_lineIndex]->local_fsms[crnt_fsmIndex];

		isTaken = taken_not_taken( crnt_fsm ) ;

	}
	else // new tag.  assume not_taken;
	{
		crnt_fsm = btb->fsmStateAtInit ;
		isTaken = false ;
		target_dst = pc+4;
	}
	

	/*Conclusion*/
	
	*dst = target_dst ; 
	return isTaken;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst)
{
	return;
}

void BP_GetStats(SIM_stats *curStats)
{
	return;
}

/*=-=-==-=-=-=-=-=-=-==*/
/*  PROXY FUNCTIONS:   */
/*=-=-==-=-=-=-=-=-=-==*/

bool is_correct_input_BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
							  bool isGlobalHist, bool isGlobalTable, int Shared)
{
	double numBTBlines =  log2(btbSize) ;

	if (
		btbSize <= 0 || historySize <= 0 || tagSize <= 0 
		|| fsmState < 0 || fsmState > 3  2 || Shared < 0 || Shared >
		|| (  ceil(numBTBlines)  !=  floor(numBTBlines)  )
		)
	{
		return false;
	}
	else
	{
		return true;
	}
}


uint32_t get_middle_bits( uint32_t input, int numLSBbits2ignore, int numBits2keep)
{
	input = input / ((uint32_t)pow(2,numLSBbits2ignore));  //ignore last "numLSBbits2ignore" bits;
	uint32_t res = input % ((uint32_t)pow(2, numBits2keep));  //keep last "numBits2keep" bits
	return res;
}

bool taken_not_taken(unsigned fsmState)
{
	if(fsmState<=1) return false; //not taken
	else return true; //taken
}
