

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
	std::vector<unsigned> local_fsms;
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


BTB* btb; //global


		  /*Proxy Functions Delecration: */
bool is_correct_input_BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
	bool isGlobalHist, bool isGlobalTable, int Shared);
uint32_t get_middle_bits(uint32_t input, int numLSBbits2ignore, int numBits2keep);
bool taken_not_taken(unsigned fsmState);
uint32_t XOR(uint32_t a, uint32_t b);
unsigned compute_btb_size(BTB* btb);
unsigned update_fsm(unsigned old_fsm, bool taken);
void init_line(int crnt_lineIndex);
void update_line(uint32_t pc, int lineIndex, bool was_taken);
int calc_fsm_index(uint32_t pc, uint32_t crnt_history);



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
		btb->stats.br_num = 0;
		btb->stats.flush_num = 0;
		btb->stats.size = compute_btb_size(btb);

		int numStateMachines = (int)pow(2, historySize);
		if (btb->isGlobal_fsm)
		{
			btb->global_fsms = new unsigned[numStateMachines];
			for (int j = 0; j < numStateMachines; j++)
			{
				btb->global_fsms[j] = fsmState;
			}
		}

		/* per line - intializing everything: */
		for (int i = 0; i < (int)btbSize; i++)
		{
			// need to check for errors:
			btb->btb_lines.push_back(new BTB_LINE);
			btb->btb_lines[i]->tag = 0;
			btb->btb_lines[i]->target_pc = 0;
			btb->btb_lines[i]->local_history = 0;

			if (!btb->isGlobal_fsm) //local fsm:
			{
				//old:
				//btb->btb_lines[i]->local_fsms = new unsigned[numStateMachines];
				for (int j = 0; j < numStateMachines; j++)
				{
					//New:
					btb->btb_lines[i]->local_fsms.push_back(fsmState);
					//old:
					//btb->btb_lines[i]->local_fsms[j] = fsmState;
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

		crnt_fsmIndex = calc_fsm_index(pc, crnt_history);

		//where is the fsm:
		if (btb->isGlobal_fsm)
			crnt_fsm = btb->global_fsms[crnt_fsmIndex];
		else
			crnt_fsm = btb->btb_lines[crnt_lineIndex]->local_fsms[crnt_fsmIndex];

		isTaken = taken_not_taken(crnt_fsm);
		if (isTaken)
		{
			target_dst = 4 * btb->btb_lines[crnt_lineIndex]->target_pc; //add two zeros
		}
		else
		{
			target_dst = pc + 4;
		}
	}
	else // new tag.  assume not_taken;
	{
		//crnt_fsm = btb->fsmStateAtInit;
		isTaken = false;
		target_dst = pc + 4;
	}

	/*Conclusion*/

	*dst = target_dst;
	return isTaken;
}






void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst)
{
	int crnt_lineIndex = (int)get_middle_bits(pc, 2, (int)log2(btb->btbSize));
	/*  Check PIPE and Flush if neeeded:*/
	if ((taken && (targetPc != pred_dst)) || (!taken && (pred_dst != (pc + 4)))   )btb->stats.flush_num++;
	


	/* Check tag collision: */
	uint32_t new_tag = get_middle_bits(pc, 2, btb->tagSize);
	uint32_t crnt_tag = btb->btb_lines[crnt_lineIndex]->tag;
	if (crnt_tag != new_tag) //tag collision
	{
		init_line(crnt_lineIndex);
		update_line(pc, crnt_lineIndex, taken);
		//Update the new lines with correct uint32_t stuff:
		btb->btb_lines[crnt_lineIndex]->tag = new_tag;
		uint32_t newTargetPC = get_middle_bits(targetPc, 2, PC_LENGTH - 2);
		btb->btb_lines[crnt_lineIndex]->target_pc = newTargetPC;
	}
	else  //no colision:
	{
		update_line(pc, crnt_lineIndex, taken);

	}


	/* Update stats:*/
	btb->stats.br_num++;

	return;
}



void BP_GetStats(SIM_stats *curStats)
{
	*curStats = btb->stats;
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
		btbSize <= 0 || historySize <= 0 || tagSize <= 0 || fsmState < 0 || fsmState > 3
		|| Shared < 0 || Shared > 2
		|| (ceil(numBTBlines) != floor(numBTBlines)) //check if btbSize is a whole power of 2.
		)   return false;

	/* shareStyle:
	not_shared = 0,
	share_lsb = 1,
	share_mid = 2
	*/
	// can't be localTable AND shared table:
	if (!isGlobalTable && Shared != not_shared) return false;

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


unsigned update_fsm(unsigned old_fsm, bool taken)
{
	if (taken)
	{
		if (old_fsm<3) return (old_fsm + 1);
		return 3;
	}
	else
	{
		if (old_fsm>0) return (old_fsm - 1);
		return 0;
	}
}


uint32_t XOR(uint32_t a, uint32_t b)
{
	uint32_t res = a^b;
	return res;
}

unsigned compute_btb_size(BTB* btb)
{
	unsigned entries = btb->btbSize;
	unsigned tag = btb->tagSize;
	unsigned target = PC_LENGTH - 2;
	unsigned history = btb->historySize;
	unsigned fsm = 2;

	unsigned res; //result

	if (btb->isGlobal_history)
	{
		if (btb->isGlobal_fsm)
		{
			res = entries*(tag + target) + history + fsm*((int)pow(2, history)); //global-global	
		}
		else // isLocal_fsm
		{
			res = entries*(tag + target + fsm*((int)pow(2, history))) + history;  //global-local
		}

	}
	else //isLocal_hisotry
	{
		if (btb->isGlobal_fsm)
		{
			res = entries*(tag + target + history) + fsm*((int)pow(2, history)); //local-global	
		}
		else // isLocal_fsm
		{
			res = entries*(tag + target + history + fsm*((int)pow(2, history))); //local-local
		}

	}


	return res;
}


void init_line(int crnt_lineIndex)
{

	if (!btb->isGlobal_history)
	{
		btb->btb_lines[crnt_lineIndex]->local_history = 0;
	}
	if (!btb->isGlobal_fsm)
	{
		int numStateMachines = (int)pow(2, btb->historySize);
		for (int i = 0; i < numStateMachines; i++)
		{
			btb->btb_lines[crnt_lineIndex]->local_fsms[i] = btb->fsmStateAtInit;
		}
	}
}

void update_line(uint32_t pc, int lineIndex, bool was_taken)
{

	int crnt_fsmIndex;
	uint32_t crnt_history;

	// what is the  "history aquiring method":
	if (btb->isGlobal_history)
		crnt_history = btb->global_history;
	else
		crnt_history = btb->btb_lines[lineIndex]->local_history;


	//finding the fsm index
	crnt_fsmIndex = calc_fsm_index(pc, crnt_history);

	//updating the fsm:
	if (btb->isGlobal_fsm)
		btb->global_fsms[crnt_fsmIndex] = update_fsm(btb->global_fsms[crnt_fsmIndex], was_taken);
	else
		btb->btb_lines[lineIndex]->local_fsms[crnt_fsmIndex] = update_fsm(btb->btb_lines[lineIndex]->local_fsms[crnt_fsmIndex], was_taken);

	// updates history according to the "history aquiring method":
	// By throwing the oldest bit and update the new one
	
	uint32_t updated_history = 2 * get_middle_bits(crnt_history, 0, btb->historySize - 1  ) + (uint32_t)was_taken;
	if (btb->isGlobal_history)
		btb->global_history = updated_history;
	else
		btb->btb_lines[lineIndex]->local_history = updated_history;
}

int calc_fsm_index(uint32_t pc, uint32_t crnt_history)
{
	if (btb->shared == not_shared)
	{
		return crnt_history;
	}
	else if (btb->shared == share_mid)
	{
		uint32_t cutted_PC = get_middle_bits(pc, 16, btb->historySize);
		return (int)XOR(cutted_PC, crnt_history);
	}
	else if (btb->shared == share_lsb)
	{
		uint32_t cutted_PC = get_middle_bits(pc, 2, btb->historySize);
		return (int)XOR(cutted_PC, crnt_history);
	}
	else
	{
		printf("ERROR IN BP_predict: if(shared    .... what the fuck");
		return -1;
	}
}
