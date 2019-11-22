/* 046267 Computer Architecture - Winter 2019/20 - HW #1 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <vector>
#include <math.h>
#include <stdio.h>

const int PC_LENGTH = 32;


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
	SIM_stats stats;  //containing statistics about the btb
} BTB;


BTB* btb ; //global


/*Proxy Functions Delecration: */
bool is_correct_input_BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
							  bool isGlobalHist, bool isGlobalTable, int Shared);
uint32_t get_middle_bits(uint32_t input, int numLSBbits2ignore, int numBits2keep);
bool taken_not_taken(unsigned fsmState);
uint32_t XOR(uint32_t a, uint32_t b);
unsigned compute_btb_size(BTB* btb);


int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared)
{
	if (!is_correct_input_BP_init(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared))
		return -1;

	try
	{
		btb = new BTB;

		btb->btbSize = btbSize;
		btb->historySize = historySize;
		btb->tagSize = tagSize;
		btb->fsmStateAtInit = fsmState;
		btb->isGlobal_history = isGlobalHist;
		btb->isGlobal_fsm = isGlobalTable;
		btb->shared = Shared;
		btb->global_history = 0; //might not be used.
		btb->stats.br_num =0;
		btb->stats.flush_num =0;
		btb->stats.size = compute_btb_size(btb);

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

	int crnt_lineIndex;
	uint32_t crnt_history;
	int crnt_fsmIndex;
	unsigned crnt_fsm;

	uint32_t tag = get_middle_bits(pc, 2, btb->tagSize);

	crnt_lineIndex = (int)get_middle_bits(pc, 2, (int)log2(btb->btbSize));

	uint32_t crntTag = btb->btb_lines[crnt_lineIndex]->tag;
	if (crntTag == tag) //existing tag
	{
		// what is the  "history aquiring method":
		if (btb->isGlobal_history)
			crnt_history = btb->global_history;
		else
			crnt_history = btb->btb_lines[crnt_lineIndex]->local_history;

		//what is the index in fsm to search:
		if (btb->shared == not_shared)
		{
			crnt_fsmIndex = crnt_history;
		}
		else if (btb->shared == share_mid)
		{
			uint32_t cutted_PC = get_middle_bits(pc , 16 , btb->historySize) ;
			crnt_fsmIndex = (int)XOR(cutted_PC,crnt_history);
		}
		else if (btb->shared == share_lsb)
		{
			uint32_t cutted_PC = get_middle_bits(pc , 2 , btb->historySize) ;
			crnt_fsmIndex = (int)XOR(cutted_PC,crnt_history);
		}
		else
		{
			printf("ERROR IN BP_predict: if(shared    .... what the fuck");
		}

		//where is the fsm:
		if (btb->isGlobal_fsm)
			crnt_fsm = btb->global_fsms[crnt_fsmIndex];
		else
			crnt_fsm = btb->btb_lines[crnt_lineIndex]->local_fsms[crnt_fsmIndex];

		isTaken = taken_not_taken(crnt_fsm);
	}
	else // new tag.  assume not_taken;
	{
		crnt_fsm = btb->fsmStateAtInit;
		isTaken = false;
		target_dst = pc + 4;
	}

	/*Conclusion*/

	*dst = target_dst;
	return isTaken;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst)
{
	/*
	should also update the btb->stats
	conatining:
			typedef struct {
				unsigned flush_num;       // Machine flushes
				unsigned br_num;      	  // Number of branch instructions
				unsigned size;		      // Theoretical allocated BTB and branch predictor size
			} SIM_stats;
	*/
	/*
	btb->stats.br_num = ...;
	btb->stats.flush_num ; ...;
	*/
	btb->stats.size = compute_btb_size(btb);
	
	return;
}

void BP_GetStats(SIM_stats *curStats)
{
	*curStats = btb->stats ;
	return;
}

/*=-=-==-=-=-=-=-=-=-==*/
/*  PROXY FUNCTIONS:   */
/*=-=-==-=-=-=-=-=-=-==*/

bool is_correct_input_BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
							  bool isGlobalHist, bool isGlobalTable, int Shared)
{
	double numBTBlines = log2(btbSize);

	if (
		btbSize <= 0  || historySize <= 0 || tagSize <= 0 || fsmState < 0 || fsmState > 3 
		|| Shared < 0 || Shared > 2 
		|| (ceil(numBTBlines) != floor(numBTBlines)) //check if btbSize is a whole power of 2.
	)   return false;
	
	/* shareStyle:
	not_shared = 0,
	share_lsb = 1,
	share_mid = 2
	*/
	// can't be localTable AND shared table.
	if (!isGlobalTable || Shared != not_shared ) return false;

	//if all is good:
	return true;
}

uint32_t get_middle_bits(uint32_t input, int numLSBbits2ignore, int numBits2keep)
{
	input = input / ((uint32_t)pow(2, numLSBbits2ignore));   //ignore last "numLSBbits2ignore" bits;
	uint32_t res = input % ((uint32_t)pow(2, numBits2keep)); //keep last "numBits2keep" bits
	return res;
}

bool taken_not_taken(unsigned fsmState)
{
	if (fsmState <= 1)
		return false; //not taken
	else
		return true; //taken
}

uint32_t XOR(uint32_t a, uint32_t b)
{
	uint32_t res = a^b;
	return res;
}

unsigned compute_btb_size(BTB* btb)
{
	unsigned entries= btb->btbSize;
	unsigned tag 	= btb->tagSize;
	unsigned target = PC_LENGTH - 2;
	unsigned history= btb->historySize;
	unsigned fsm	= 2;

	unsigned res; //result

	if (btb->isGlobal_history) 
	{
		if (btb->isGlobal_fsm) 
		{
			 res = entries*(tag + target  ) + history + fsm*( (int)pow(2,history) ) ; //global-global	
		}
		else // isLocal_fsm
		{
			res = entries*(tag + target + fsm*( (int)pow(2,history) )  )  + history  ;  //global-local
		}
		
	}
	else //isLocal_hisotry
	{
		if (btb->isGlobal_fsm) 
		{
			res = entries*(tag + target + history )  + fsm*( (int)pow(2,history) ) ; //local-global	
		}
		else // isLocal_fsm
		{
			res = entries*(tag + target + history + fsm*( (int)pow(2,history) ) ); //local-local
		}
		
	}
	

	return res;
}