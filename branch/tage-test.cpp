/*
Original Credits: (Thanks for making it publicly available!)
https://github.com/Hiren6/TAGE-Branch-Predictor

Adapted By:
Akash Maji (akashmaji@iisc.ac.in)
https://github.com/akashmaji946/ChampSim-IISc
*/

#include "ooo_cpu.h"
#include <bitset>

#define TAKEN 1
#define NOT_TAKEN 0
#define SET 1
#define RESET 0

// Global History //
#define GHIST_MAX_LEN 641 // History length is taken from L-TAGE paper.
#define GHIST bitset<GHIST_MAX_LEN>
#define PATH_HISTORY_LEN 16 // modified path history. L-TAGE says only 16. Increasing it improves performance. 
#define USE_ALT_CTR_INIT 8
#define USE_ALT_CTR_MAX 15

// Bimodal Table
#define BIMODAL_BITS 13
#define BIMODAL_PRED_CTR_INIT 2
#define BIMODAL_PRED_CTR_MAX 3 // 2 bit counter.
typedef struct bimodal {
    uint32_t ctr; // Prediction counter of 2 bits
} bimodal_t;

// Global TAG Tables
#define WEAK_NOT_TAKEN  3
#define WEAK_TAKEN  4
#define NUM_TABLES 12   // Total number of TAGE tables
#define TAGE_BITS 11    // Total number of entries per table = 2^TAGE_BITS
#define TAGE_PRED_CTR_INIT 0
#define TAGE_PRED_CTR_MAX  7 // since 3 bit counter
#define TAGE_USEFUL_CTR_MAX 3  // since 2 bit counter
#define NOT_FOUND 12

typedef struct global {
    uint32_t ctr; // 3 bits signed prediction counter
    uint32_t tag; // Tag for each entry
    uint32_t u; // 2 bit unsigned useful bits
} global_t;

typedef struct fold_history {
    uint32_t comp;
    uint32_t comp_length;
    uint32_t orig_length;
} fold_history_t;

typedef struct prediction {
    // Bimodal prediction.
    bool bimodal;
    // Primary prediction.
    bool primary;
    int primary_table;
    // Alternate prediction.
    bool alt;
    int alt_table;
} prediction_t;

GHIST ghist; // Global history with max 641 bits
unsigned long long phist; // Path History of 32 bits

prediction_t pred;///NUM_CPUS??
uint32_t use_alt; // Global 4 bit counter to check whether to use alternate prediction or not.
     
/*
Storage Budget Justifiction:
---------------------------
We are using 12 TAGE tables with entry sizes as:
[24, 22, 20, 20, 18, 18, 16, 16, 14, 14, 12, 12]
as the tag_width are:
[19, 17, 15, 15, 13, 13, 11, 11, 9, 9, 7, 7]
and 5 bits for 'ctr' and 'u' fields

There are 2^11 entries in each TAGE table. TAGE_BITS=11
So TAGE Tables Size = (sum[24, 22, 20, 20, 18, 18, 16, 16, 14, 14, 12, 12] bits * 2^11)
                    = 206 bits * 2048
                    = 52 KB

Bimodal Table Size = 2^13 entries each of 2 bits
                   = 2 ^ 14 bits
                   = 2 ^ 11 B
                   = 4 KB

Total Size = 52 + 4 KB = 56 KB which is withing 64 KB budget

*/

// Bimodal Table. Indexed by PC.
uint32_t bimodal_len=(1 << BIMODAL_BITS);///change
bimodal_t bimodal[NUM_CPUS][(1 << BIMODAL_BITS)];///change
uint32_t bimodalIndex;

// Global TAGE tables.
global_t tage[NUM_CPUS][NUM_TABLES][1 << TAGE_BITS];///////array of 12 tage tables(each table is an global_t array)
uint32_t tage_len = (1 << TAGE_BITS);


// Global history lengths according to the adjustment by me
uint32_t history_length[NUM_TABLES]={640, 500, 400, 200, 100, 75, 50, 24, 16, 8, 6, 4};
// Global tag lengths according to the adjustment by me
uint32_t tag_width[NUM_TABLES]={19, 17, 15, 15, 13, 13, 11, 11, 9, 9, 7, 7};

// Aid to history folding.
fold_history_t folded_index[NUM_CPUS][NUM_TABLES];
fold_history_t folded_tag[NUM_CPUS][2][NUM_TABLES];

// Store the index and tags for each table.
uint32_t tage_index[NUM_CPUS][NUM_TABLES];
uint32_t tage_tag[NUM_CPUS][NUM_TABLES];
bool toggle;
uint32_t clock_time;

uint32_t limit_increase(uint32_t &a, uint32_t lim) {
    if(a < lim) ++a;
    return a;
}

uint32_t limit_decrease(uint32_t &a) {
    if(a > 0) --a;
    return a;
}

void BitsFoldingInit(fold_history_t *f,  uint32_t original_length, uint32_t compressed_length) {
    f->comp = 0;
    f->orig_length = original_length;
    f->comp_length = compressed_length;
}

void BitsFolding(fold_history_t *f, GHIST ghist) {
    f->comp = (f->comp << 1) | ghist[0];
    f->comp ^= ghist[f->orig_length] << (f->orig_length % f->comp_length);
    f->comp ^= (f->comp >> f->comp_length);
    f->comp &= (1 << f->comp_length) - 1;
}

int GetIndex(uint32_t PC, int table,int cpu) {
    int index = PC ^ (PC >> (TAGE_BITS - table )) ^ folded_index[cpu][table].comp ^ (phist>> (TAGE_BITS - table));
    return (index & ((1 << TAGE_BITS) - 1));
}

int GetTag(uint32_t PC, int table, int tag_width,int cpu) {
    int tag = (PC ^ folded_tag[cpu][0][table].comp ^ (folded_tag[cpu][1][table].comp << 1));
    return (tag & ((1 << tag_width) - 1));
}

void O3_CPU::initialize_branch_predictor(void) {
    ghist.reset();///initializing the global hitstory array
    phist = 0;
    //for(int i=0;i<NUM_CPUS;i++)
        //bimodal[i] = new bimodal_t[bimodal_len];///initialize bimodal table,Each entry contains 2 bit number ctr
    // Initialize the bimodal table.
    for (uint32_t i = 0; i< bimodal_len; i++) {
        bimodal[cpu][i].ctr = BIMODAL_PRED_CTR_INIT;///initialize the counter value
    }
    // Init the tag tables and corresponding helper tables.
    for (int i = 0; i < NUM_TABLES ; i++) {///for 12 tables
        //tage[cpu][i] = new global_t[tage_len];///allocate memory for the tables
        for (uint32_t j = 0; j < tage_len; j++) {
            tage[cpu][i][j].ctr = TAGE_PRED_CTR_INIT;///initialize the values
            tage[cpu][i][j].tag = RESET;
            tage[cpu][i][j].u = RESET;
        }
        BitsFoldingInit(&folded_index[cpu][i], history_length[i], TAGE_BITS);
        BitsFoldingInit(&folded_tag[cpu][0][i], history_length[i], tag_width[i]);
        BitsFoldingInit(&folded_tag[cpu][1][i], history_length[i], tag_width[i] - 1);

        tage_index[cpu][i] = RESET;
        tage_tag[cpu][i] = RESET;
    }
    use_alt = USE_ALT_CTR_INIT;///initialising altpred(USE_ALT_ON_NAx)
    clock_time = RESET;
    toggle = SET;
}

uint8_t O3_CPU::predict_branch(uint64_t PC) {
    pred.primary = NOT_TAKEN;
    pred.alt = NOT_TAKEN;
    pred.primary_table = NOT_FOUND;
    pred.alt_table = NOT_FOUND;
    pred.bimodal = NOT_TAKEN;///ADD
    int pt=0,at=0;
    // Get the index and tags for the current branch from all tables
    for (int i = 0 ; i < NUM_TABLES; i++) {
        tage_tag[cpu][i] = GetTag(PC, i, tag_width[i],cpu);
        tage_index[cpu][i] = GetIndex(PC, i,cpu);
    }
    // Primary prediction
    for (int i = 0; i < NUM_TABLES; i++) {
        if (tage[cpu][i][tage_index[cpu][i]].tag == tage_tag[cpu][i]) {
            pred.primary_table = i;
            pt=i;
            break;
        }
    }
    //alternate prediction is lesser than length of primary history tables.
    for (int i = pt + 1; i < NUM_TABLES; i++) {
        if (tage[cpu][i][tage_index[cpu][i]].tag == tage_tag[cpu][i]) {
            pred.alt_table = i;
            at=i;
            break;
        }
    }
    // the bimodal prediction.
    bimodalIndex = (PC & ((1 << BIMODAL_BITS ) - 1));
    if (bimodal[cpu][bimodalIndex].ctr > BIMODAL_PRED_CTR_MAX/2) {
        pred.bimodal =  TAKEN;
    } 

///PREDICTION START
    if (pred.primary_table == NOT_FOUND) {//no primary prediction
        return pred.bimodal;
    } else {
        if (pred.alt_table == NOT_FOUND) {
            pred.alt = pred.bimodal;
        } else {
            if (tage[cpu][at][tage_index[cpu][at]].ctr>= TAGE_PRED_CTR_MAX/2) {
                pred.alt = TAKEN;
            } 
        }
        at=tage[cpu][pt][tage_index[cpu][pt]].ctr;
        if ((at!= WEAK_NOT_TAKEN) ||(at != WEAK_TAKEN) ||(tage[cpu][pt][tage_index[cpu][pt]].u != RESET) ||(use_alt < 8)) {
            if (at > TAGE_PRED_CTR_MAX/2) {
                pred.primary = TAKEN;
            } 
            return pred.primary;
        } else {//weak entry-->alternate prediction.
            return pred.alt;
        }
    }
}

void O3_CPU::last_branch_result(uint64_t ip, uint8_t taken) {
    //define prediction here
    uint8_t predicted = predict_branch(ip);
    
    uint8_t outcome = taken;
    int pt = pred.primary_table, at = pred.alt_table;
    // Steal an entry only if misprediction and if we did not use the longest history to predict
    if ((predicted != outcome) && (pt > 0)) {
        bool found_empty = false;
        for (int i = 0; i < pt; i++) {
            if (tage[cpu][i][tage_index[cpu][i]].u == 0) {
                found_empty = true;
                break;
            }
        }

        if (found_empty == true) {
            int random_num = rand() % 100;
            for (int i = pt - 1; i >= 0; i--) {
                if (tage[cpu][i][tage_index[cpu][i]].u == 0) {
                    // Take biased decision whether to update the table or not.
                    if (random_num <= 99/(pt-i+1)) {
                        if (outcome == TAKEN) 
                            tage[cpu][i][tage_index[cpu][i]].ctr = 4;
                        else 
                            tage[cpu][i][tage_index[cpu][i]].ctr = 3;
                        tage[cpu][i][tage_index[cpu][i]].tag = tage_tag[cpu][i];
                        tage[cpu][i][tage_index[cpu][i]].u = 0;
                    }
                }
            }
        } 
        else {
            // Choose a random table and steal it.
            int rand_idx = rand() % NUM_TABLES;
            if(outcome == TAKEN)
                tage[cpu][rand_idx][tage_index[cpu][rand_idx]].ctr = 4;
            else
                tage[cpu][rand_idx][tage_index[cpu][rand_idx]].ctr = 3;
            tage[cpu][rand_idx][tage_index[cpu][rand_idx]].tag = tage_tag[cpu][rand_idx];
            tage[cpu][rand_idx][tage_index[cpu][rand_idx]].u = 0;
        }
    }

    if (pt == NOT_FOUND) {  // if no tag matches in any table then use default bimodal
        // Bimodal prediction used as default
        if(outcome == TAKEN) 
            bimodal[cpu][bimodalIndex].ctr = limit_increase(bimodal[cpu][bimodalIndex].ctr, BIMODAL_PRED_CTR_MAX);
        else 
            bimodal[cpu][bimodalIndex].ctr = limit_decrease(bimodal[cpu][bimodalIndex].ctr);
    } 
    else {
        // Update the counters in the primary table
        if (outcome == TAKEN) 
            tage[cpu][pt][tage_index[cpu][pt]].ctr = limit_increase(tage[cpu][pt][tage_index[cpu][pt]].ctr, TAGE_PRED_CTR_MAX);
        else 
            tage[cpu][pt][tage_index[cpu][pt]].ctr = limit_decrease(tage[cpu][pt][tage_index[cpu][pt]].ctr);
        
        // Update the useful counters of the primary table
        if (pred.primary == outcome) 
            tage[cpu][pt][tage_index[cpu][pt]].u = limit_increase(tage[cpu][pt][tage_index[cpu][pt]].u, TAGE_USEFUL_CTR_MAX);
        else 
            tage[cpu][pt][tage_index[cpu][pt]].u = limit_decrease(tage[cpu][pt][tage_index[cpu][pt]].u);

        // Check for weak entries
        if (((tage[cpu][pt][tage_index[cpu][pt]].ctr == WEAK_NOT_TAKEN) || (tage[cpu][pt][tage_index[cpu][pt]].ctr == WEAK_TAKEN)) && (tage[cpu][pt][tage_index[cpu][pt]].u == RESET)) {
            if ((pred.primary != outcome) && (pred.alt == outcome)) {
                // Alternate prediction was good
                limit_increase(use_alt, USE_ALT_CTR_MAX);
                if (at != NUM_TABLES) {
                    if (outcome == TAKEN) 
                        tage[cpu][at][tage_index[cpu][at]].u = limit_increase(tage[cpu][at][tage_index[cpu][at]].u, TAGE_USEFUL_CTR_MAX);
                    else 
                        tage[cpu][at][tage_index[cpu][at]].u = limit_decrease(tage[cpu][at][tage_index[cpu][at]].u);

                    // If the alternate prediction came from a TAGE table, then increment the useful counter 
                    tage[cpu][at][tage_index[cpu][at]].u = limit_increase(tage[cpu][at][tage_index[cpu][at]].u, TAGE_USEFUL_CTR_MAX);
                }
            } 
            else {
                limit_decrease(use_alt);
                if (at != NUM_TABLES) {
                    // If the alternate prediction came from a TAGE table, then decrement the useful counter 
                    tage[cpu][at][tage_index[cpu][at]].u = limit_decrease(tage[cpu][at][tage_index[cpu][at]].u);
                }
            }
        }
    }

    // update the Global history and path history.
    ghist = (ghist << 1);
    phist = (phist << 1);
    if (outcome == TAKEN) {
        ghist.set(0,1);
        phist = phist + 1;
    }

    phist = phist % PATH_HISTORY_LEN;

    for (int i = 0; i < NUM_TABLES; i++) {
        BitsFolding(&folded_index[cpu][i], ghist);
        BitsFolding(&folded_tag[cpu][0][i], ghist);
        BitsFolding(&folded_tag[cpu][1][i], ghist);
    }

    clock_time++;
    // RESET the useful bits after every 256K instructions
    if (clock_time == 256 * 1024) { 
        if (toggle == SET) {
            for (int i = 0; i < NUM_TABLES; i++) {
                for (uint32_t j = 0; j < tage_len; j++) 
                    tage[cpu][i][j].u = tage[cpu][i][j].u & 1;
            }
            toggle = RESET;
        } 
        else {
            for (int i = 0; i < NUM_TABLES; i++) {
                for (uint32_t j = 0; j < tage_len; j++) 
                    tage[cpu][i][j].u = tage[cpu][i][j].u & 2;
            }
            toggle = SET;
        }
        clock_time = RESET;
    }
}

