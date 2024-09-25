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
We want to have at most 64KB for our bimodal predictor per CPU
We will use 2-bit saturating counter
So entry size = 2 bits
Number of entries = 64 * 8 K bits / 2 bits
                  = 256 K
                  = 262144
Largest prime less than 262144 = 262139

*/
#define BIMODAL_TABLE_SIZE 262144
#define BIMODAL_PRIME 262139
#define MAX_COUNTER 3
int bimodal_table[NUM_CPUS][BIMODAL_TABLE_SIZE];

void O3_CPU::initialize_branch_predictor()
{
    cout << "CPU " << cpu << " Bimodal branch predictor" << endl;

    for(int i = 0; i < BIMODAL_TABLE_SIZE; i++)
        bimodal_table[cpu][i] = 0;
}

uint8_t O3_CPU::predict_branch(uint64_t ip)
{
    uint32_t hash = ip % BIMODAL_PRIME;
    uint8_t prediction = (bimodal_table[cpu][hash] >= ((MAX_COUNTER + 1)/2)) ? 1 : 0;

    return prediction;
}

void O3_CPU::last_branch_result(uint64_t ip, uint8_t taken)
{
    uint32_t hash = ip % BIMODAL_PRIME;

    if (taken && (bimodal_table[cpu][hash] < MAX_COUNTER))
        bimodal_table[cpu][hash]++;
    else if ((taken == 0) && (bimodal_table[cpu][hash] > 0))
        bimodal_table[cpu][hash]--;
}
