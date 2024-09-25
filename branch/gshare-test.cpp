#include "ooo_cpu.h"

/*
Original Credits: (Thanks for making it publicly available!)
https://github.com/casperIITB/ChampSim
https://github.com/ChampSim/ChampSim

Adapted By:
Akash Maji (akashmaji@iisc.ac.in)
https://github.com/akashmaji946/ChampSim-IISc
*/

/*
-------------------
Storage Cost Estimation:
-------------------
We want to have at most 64KB for our gshare predictor per CPU
We will use 2-bit saturating counter
So entry size = 2 bits
Number of entries = 64 * 8 K bits / 2 bits
                  = 256 K
                  = 262144

*/

#define GLOBAL_HISTORY_LENGTH 16
#define GLOBAL_HISTORY_MASK (1 << GLOBAL_HISTORY_LENGTH) - 1
int branch_history_vector[NUM_CPUS];

#define GS_HISTORY_TABLE_SIZE 262144
int gs_history_table[NUM_CPUS][GS_HISTORY_TABLE_SIZE];
int my_last_prediction[NUM_CPUS];

void O3_CPU::initialize_branch_predictor()
{
    cout << "CPU " << cpu << " GSHARE branch predictor" << endl;

    branch_history_vector[cpu] = 0;
    my_last_prediction[cpu] = 0;

    for(int i=0; i<GS_HISTORY_TABLE_SIZE; i++)
        gs_history_table[cpu][i] = 2; // 2 is slightly taken
}

unsigned int gs_table_hash(uint64_t ip, int bh_vector)
{
    unsigned int hash = ip^(ip>>GLOBAL_HISTORY_LENGTH)^(ip>>(GLOBAL_HISTORY_LENGTH*2))^bh_vector;
    hash = hash%GS_HISTORY_TABLE_SIZE;

    //printf("%d\n", hash);

    return hash;
}

uint8_t O3_CPU::predict_branch(uint64_t ip)
{
    int prediction = 1;

    int gs_hash = gs_table_hash(ip, branch_history_vector[cpu]);

    if(gs_history_table[cpu][gs_hash] >= 2)
        prediction = 1;
    else
        prediction = 0;

    my_last_prediction[cpu] = prediction;

    return prediction;
}

void O3_CPU::last_branch_result(uint64_t ip, uint8_t taken)
{
    int gs_hash = gs_table_hash(ip, branch_history_vector[cpu]);

    if(taken == 1) {
        if(gs_history_table[cpu][gs_hash] < 3)
            gs_history_table[cpu][gs_hash]++;
    } else {
        if(gs_history_table[cpu][gs_hash] > 0)
            gs_history_table[cpu][gs_hash]--;
    }

    // update branch history vector
    branch_history_vector[cpu] <<= 1;
    branch_history_vector[cpu] &= GLOBAL_HISTORY_MASK;
    branch_history_vector[cpu] |= taken;
}
