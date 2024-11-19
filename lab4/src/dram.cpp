///////////////////////////////////////////////////////////////////////////////
// You will need to modify this file to implement parts B and C.             //
///////////////////////////////////////////////////////////////////////////////

// dram.cpp
// Defines the functions used to implement DRAM.

#include "dram.h"
#include <stdio.h>
#include <stdlib.h>
// You may add any other #include directives you need here, but make sure they
// compile on the reference machine!

///////////////////////////////////////////////////////////////////////////////
//                                 CONSTANTS                                 //
///////////////////////////////////////////////////////////////////////////////

/** The fixed latency of a DRAM access assumed in part B, in cycles. */
#define DELAY_SIM_MODE_B 100

/** The DRAM activation latency (ACT), in cycles. (Also known as RAS.) */
#define DELAY_ACT 45

/** The DRAM column selection latency (CAS), in cycles. */
#define DELAY_CAS 45

/** The DRAM precharge latency (PRE), in cycles. */
#define DELAY_PRE 45

/**
 * The DRAM bus latency, in cycles.
 * 
 * This is how long it takes for the DRAM to transmit the data via the bus. It
 * is incurred on every DRAM access. (In part B, assume that this is included
 * in the fixed DRAM latency DELAY_SIM_MODE_B.)
 */
#define DELAY_BUS 10

/** The row buffer size, in bytes. */
#define ROW_BUFFER_SIZE 1024

/** The number of banks in the DRAM module. */
#define NUM_BANKS 16

///////////////////////////////////////////////////////////////////////////////
//                    EXTERNALLY DEFINED GLOBAL VARIABLES                    //
///////////////////////////////////////////////////////////////////////////////

/**
 * The current mode under which the simulation is running, corresponding to
 * which part of the lab is being evaluated.
 */
extern Mode SIM_MODE;

/** The number of bytes in a cache line. */
extern uint64_t CACHE_LINESIZE;

/** Which page policy the DRAM should use. */
extern DRAMPolicy DRAM_PAGE_POLICY;

///////////////////////////////////////////////////////////////////////////////
//                           FUNCTION DEFINITIONS                            //
///////////////////////////////////////////////////////////////////////////////

// As described in dram.h, you are free to deviate from the suggested
// implementation as you see fit.

// The only restriction is that you must not remove dram_print_stats() or
// modify its output format, since its output will be used for grading.

/**
 * Allocate and initialize a DRAM module.
 * 
 * This is intended to be implemented in part B.
 * 
 * @return A pointer to the DRAM module.
 */
DRAM *dram_new()
{
    // TODO: Allocate memory to the data structures and initialize the required
    //       fields. (You might want to use calloc() for this.)
    #ifdef DEBUG
        printf("Creating DRAM!\n");
    #endif
    DRAM *newDram = (DRAM*)calloc(1, sizeof(DRAM));
    newDram->RowbufEntries = (RowBuffer*)calloc(NUM_BANKS, sizeof(RowBuffer));
    for(unsigned int i = 0; i < NUM_BANKS; i++)
    {
        newDram->RowbufEntries[i].rowId = 0;
        newDram->RowbufEntries[i].valid = false;
    }
    uint64_t num = ROW_BUFFER_SIZE / CACHE_LINESIZE;
    int column_bits = 0;
    while(num >>= 1)
    {
        column_bits++;
    }
    num = NUM_BANKS;
    int bank_bits = 0;
    while(num >>=1)
    {
        bank_bits++;
    }
    int row_bits = 32 - bank_bits - column_bits;
    uint64_t bank_mask = (1ULL << bank_bits) - 1;
    bank_mask <<= column_bits;
    uint64_t row_mask = (1ULL << row_bits) - 1;
    row_mask <<= (bank_bits + column_bits);
    newDram->column_bits = column_bits;
    newDram->bank_bits = bank_bits;
    newDram->bank_mask = bank_mask;
    newDram->row_mask = row_mask;
    newDram->stat_read_access = 0;
    newDram->stat_read_delay = 0;
    newDram->stat_write_access = 0;
    newDram->stat_write_delay = 0;
    return newDram;
}

/**
 * Access the DRAM at the given cache line address.
 * 
 * Return the delay in cycles incurred by this DRAM access. Also update the
 * DRAM statistics accordingly.
 * 
 * Note that the address is given in units of the cache line size!
 * 
 * This is intended to be implemented in parts B and C. In parts C through F,
 * you may delegate logic to the dram_access_mode_CDEF() functions.
 * 
 * @param dram The DRAM module to access.
 * @param line_addr The address of the cache line to access (in units of the
 *                  cache line size).
 * @param is_dram_write Whether this access writes to DRAM.
 * @return The delay in cycles incurred by this DRAM access.
 */
uint64_t dram_access(DRAM *dram, uint64_t line_addr, bool is_dram_write)
{
    
    uint64_t delay = DELAY_SIM_MODE_B;
    #ifdef DEBUG
        printf("\tAccessing DRAM! Calculating delay...\n");
    #endif
    // TODO: Update the appropriate DRAM statistics.
    // TODO: Call the dram_access_mode_CDEF() function as needed.
    // TODO: Return the delay in cycles incurred by this DRAM access.
    if(SIM_MODE != SIM_MODE_B)
    {
        delay = dram_access_mode_CDEF(dram, line_addr, is_dram_write);
    }
    if(is_dram_write)
    {
        dram->stat_write_access++;
        dram->stat_write_delay+= delay;
    }
    else
    {
        dram->stat_read_access++;
        dram->stat_read_delay+= delay;
    }
    #ifdef DEBUG
        printf("\t\tDRAM delay: %ld, is_dram_write: %d\n", delay, is_dram_write);
    #endif
    return delay;
}

/**
 * For parts C through F, access the DRAM at the given cache line address.
 * 
 * Return the delay in cycles incurred by this DRAM access. It is intended that
 * the calling function will be responsible for updating DRAM statistics
 * accordingly.
 * 
 * Note that the address is given in units of the cache line size!
 * 
 * This is intended to be implemented in part C.
 * 
 * @param dram The DRAM module to access.
 * @param line_addr The address of the cache line to access (in units of the
 *                  cache line size).
 * @param is_dram_write Whether this access writes to DRAM.
 * @return The delay in cycles incurred by this DRAM access.
 */
uint64_t dram_access_mode_CDEF(DRAM *dram, uint64_t line_addr,
                               bool is_dram_write)
{
    // Assume a mapping with consecutive lines in the same row and consecutive
    // row buffers in consecutive rows.
    // TODO: Use this function to track open rows.
    // TODO: Compute the delay based on row buffer hit/miss/empty.
    uint64_t delay = DELAY_BUS;
    
    uint64_t bank_index = line_addr & dram->bank_mask;
    bank_index >>= dram->column_bits;
    uint64_t rowId = line_addr & dram->row_mask;
    rowId >>= (dram->bank_bits + dram->column_bits);
    #ifdef DEBUG
        printf("\t\tbank index: %ld, row index: %ld\n", bank_index, rowId);
        if(DRAM_PAGE_POLICY == CLOSE_PAGE)
        {
            printf("\t\tUsing CLOSE_PAGE policy!\n");
        }
        if(DRAM_PAGE_POLICY == OPEN_PAGE)
        {
            printf("\t\tUsing OPEN_PAGE policy!\n");
            if(dram->RowbufEntries[bank_index].valid)
            {
                printf("\t\tValid bank found!\n");
                if(dram->RowbufEntries[bank_index].rowId == rowId)
                {
                    printf("\t\tRow index matched!\n");
                }
                else
                {
                    printf("\t\tRow did not match! Updating row index.\n");
                }
            }
            else
            {
                printf("\t\tBank not valid! Setting to valid and updating row index.\n");
            }
        }
        
    #endif
    if(DRAM_PAGE_POLICY == CLOSE_PAGE)
    {
        delay = DELAY_SIM_MODE_B;
    }
    if(DRAM_PAGE_POLICY == OPEN_PAGE)
    {
        // if(is_dram_write)
        // {
        //     delay += DELAY_ACT + DELAY_CAS + DELAY_PRE;
        //     dram->RowbufEntries[bank_index].valid = false;
        // }
        // else 
        if(dram->RowbufEntries[bank_index].valid)
        {
            if(rowId == dram->RowbufEntries[bank_index].rowId)
            {
                delay += DELAY_CAS;
            }
            else
            {
                dram->RowbufEntries[bank_index].rowId = rowId;
                delay += DELAY_ACT + DELAY_CAS + DELAY_PRE;
            }
        }
        else
        {
            dram->RowbufEntries[bank_index].valid = true;
            dram->RowbufEntries[bank_index].rowId = rowId;
            delay += DELAY_ACT + DELAY_CAS;
        }
    }
    return delay;
}

/**
 * Print the statistics of the DRAM module.
 * 
 * This is implemented for you. You must not modify its output format.
 * 
 * @param dram The DRAM module to print the statistics of.
 */
void dram_print_stats(DRAM *dram)
{
    double avg_read_delay = 0.0;
    double avg_write_delay = 0.0;

    if (dram->stat_read_access)
    {
        avg_read_delay = (double)(dram->stat_read_delay) /
                         (double)(dram->stat_read_access);
    }

    if (dram->stat_write_access)
    {
        avg_write_delay = (double)(dram->stat_write_delay) /
                          (double)(dram->stat_write_access);
    }

    printf("\n");
    printf("DRAM_READ_ACCESS     \t\t : %10llu\n", dram->stat_read_access);
    printf("DRAM_WRITE_ACCESS    \t\t : %10llu\n", dram->stat_write_access);
    printf("DRAM_READ_DELAY_AVG  \t\t : %10.3f\n", avg_read_delay);
    printf("DRAM_WRITE_DELAY_AVG \t\t : %10.3f\n", avg_write_delay);
}
