// --------------------------------------------------------------------- //
// For part B, you will need to modify this file.                        //
// You may add any code you need, as long as you correctly implement the //
// three required BPred methods already listed in this file.             //
// --------------------------------------------------------------------- //

// bpred.cpp
// Implements the branch predictor class.

#include "bpred.h"

/**
 * Construct a branch predictor with the given policy.
 * 
 * In part B of the lab, you must implement this constructor.
 * 
 * @param policy the policy this branch predictor should use
 */
BPred::BPred(BPredPolicy policy)
{
    // TODO: Initialize member variables here.
    this->policy = policy;
    pht.resize(4096,2);
    ghr = 0;
    pattern = 0;
    // As a reminder, you can declare any additional member variables you need
    // in the BPred class in bpred.h and initialize them here.
}

/**
 * Get a prediction for the branch with the given address.
 * 
 * In part B of the lab, you must implement this method.
 * 
 * @param pc the address (program counter) of the branch to predict
 * @return the prediction for whether the branch is taken or not taken
 */
BranchDirection BPred::predict(uint64_t pc)
{
    // TODO: Return a prediction for whether the branch at address pc will be
    // TAKEN or NOT_TAKEN according to this branch predictor's policy.
    // Note that you do not have to handle the BPRED_PERFECT policy here; this
    // function will not be called for that policy.
    if(this->policy == 1){
        return TAKEN;
    }
    else {
        uint16_t pc_lower = pc & 0xFFF;
        uint16_t ghr_lower = ghr & 0xFFF;
        pattern = pc_lower ^ ghr_lower;
        prediction = pht[pattern];
        if(prediction == 2 || prediction == 3){
            return TAKEN;
        }
        else{
            return NOT_TAKEN;
        }
    }
   
}


/**
 * Update the branch predictor statistics (stat_num_branches and
 * stat_num_mispred), as well as any other internal state you may need to
 * update in the branch predictor.
 * 
 * In part B of the lab, you must implement this method.
 * 
 * @param pc the address (program counter) of the branch
 * @param prediction the prediction made by the branch predictor
 * @param resolution the actual outcome of the branch
 */
void BPred::update(uint64_t pc, BranchDirection prediction,
                   BranchDirection resolution)
{
    // TODO: Update the stat_num_branches and stat_num_mispred member variables
    // according to the prediction and resolution of the branch.
    stat_num_branches++;
    if(prediction != resolution){
        stat_num_mispred++;
        
    }
    if(resolution){
        if(pht[pattern] != 3){
            pht[pattern]++;
        }
    }
    else{
        if(pht[pattern] != 0){
            pht[pattern]--;
        }
    }
    ghr = ghr << 1;
    if(resolution){
        ghr |= 0x1;
    }
    // TODO: Update any other internal state you may need to keep track of.

    // Note that you do not have to handle the BPRED_PERFECT policy here; this
    // function will not be called for that policy.
}
