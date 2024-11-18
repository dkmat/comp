///////////////////////////////////////////////////////////////////////////////
// You will need to modify this file to implement part A and, for extra      //
// credit, parts E and F.                                                    //
///////////////////////////////////////////////////////////////////////////////

// cache.cpp
// Defines the functions used to implement the cache.

#include "cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <random>
#include <ctime>
// You may add any other #include directives you need here, but make sure they
// compile on the reference machine!

///////////////////////////////////////////////////////////////////////////////
//                    EXTERNALLY DEFINED GLOBAL VARIABLES                    //
///////////////////////////////////////////////////////////////////////////////

/**
 * The current clock cycle number.
 * 
 * This can be used as a timestamp for implementing the LRU replacement policy.
 */
extern uint64_t current_cycle;

/**
 * For static way partitioning, the quota of ways in each set that can be
 * assigned to core 0.
 * 
 * The remaining number of ways is the quota for core 1.
 * 
 * This is used to implement extra credit part E.
 */
extern unsigned int SWP_CORE0_WAYS;

///////////////////////////////////////////////////////////////////////////////
//                           FUNCTION DEFINITIONS                            //
///////////////////////////////////////////////////////////////////////////////

// As described in cache.h, you are free to deviate from the suggested
// implementation as you see fit.

// The only restriction is that you must not remove cache_print_stats() or
// modify its output format, since its output will be used for grading.

/**
 * Allocate and initialize a cache.
 * 
 * This is intended to be implemented in part A.
 *
 * @param size The size of the cache in bytes.
 * @param associativity The associativity of the cache.
 * @param line_size The size of a cache line in bytes.
 * @param replacement_policy The replacement policy of the cache.
 * @return A pointer to the cache.
 */
Cache *cache_new(uint64_t size, uint64_t associativity, uint64_t line_size,
                 ReplacementPolicy replacement_policy)
{
    // TODO: Allocate memory to the data structures and initialize the required
    //       fields. (You might want to use calloc() for this.)
    Cache *newCache = (Cache*)calloc(1, sizeof(Cache));
    newCache->ways = (associativity <= 16) ? associativity : 16;
    newCache->sets = (size / line_size) / newCache->ways;
    uint64_t num = newCache->sets;
    int index_bits = 0;
    while(num >>= 1)
    {
        index_bits++;    
    }
    int tag_bits = 26 - index_bits;
    uint64_t index_mask = (1ULL << index_bits) - 1;
    uint64_t tag_mask = (1ULL << tag_bits) - 1;
    tag_mask <<= index_bits;
    newCache->index_bits = index_bits;
    newCache->index_mask = index_mask;
    newCache->tag_mask = tag_mask;
    newCache->replacementPolicy = replacement_policy;
    newCache->cacheSets = (CacheSet*)calloc(newCache->sets, sizeof(CacheSet));
    for(unsigned int i = 0; i < newCache->sets; i++)
    {
        newCache->cacheSets[i].cacheLines = (CacheLine*)calloc(newCache->ways, sizeof(CacheLine));
    }
    newCache->stat_dirty_evicts = 0;
    newCache->stat_read_access = 0;
    newCache->stat_read_miss = 0;
    newCache->stat_write_access = 0;
    newCache->stat_write_miss = 0;
    for(unsigned int i = 0; i < newCache->sets; i++)
    {
        for(unsigned int j = 0; j < newCache->ways; j++)
        {
            newCache->cacheSets[i].cacheLines[j].valid = false;
            newCache->cacheSets[i].cacheLines[j].dirty = false;
            newCache->cacheSets[i].cacheLines[j].accessTime = 0;
            newCache->cacheSets[i].cacheLines[j].coreId = -1;
            newCache->cacheSets[i].cacheLines[j].tag = 0;
        }
    }
    #ifdef DEBUG
        printf("Creating cache (# sets: %ld, # ways: %ld)\n", newCache->sets, newCache->ways);
    #endif
    return newCache;
}

/**
 * Access the cache at the given address.
 * 
 * Also update the cache statistics accordingly.
 * 
 * This is intended to be implemented in part A.
 * 
 * @param c The cache to access.
 * @param line_addr The address of the cache line to access (in units of the
 *                  cache line size, i.e., excluding the line offset bits).
 * @param is_write Whether this access is a write.
 * @param core_id The CPU core ID that requested this access.
 * @return Whether the cache access was a hit or a miss.
 */
CacheResult cache_access(Cache *c, uint64_t line_addr, bool is_write,
                         unsigned int core_id)
{
    // TODO: Return HIT if the access hits in the cache, and MISS otherwise.
    // TODO: If is_write is true, mark the resident line as dirty.
    // TODO: Update the appropriate cache statistics.
   
    uint64_t index = line_addr & c->index_mask;
    uint64_t tag = line_addr & c->tag_mask;
    tag >>= c->index_bits;
    CacheResult result = MISS;
    for(unsigned int i = 0; i < c->ways; i++)
    {
        result = (c->cacheSets[index].cacheLines[i].tag == tag)? HIT : MISS;
        if(result == HIT) 
        {
            c->cacheSets[index].cacheLines[i].accessTime = current_cycle;
            if(is_write)
            {
                c->cacheSets[index].cacheLines[i].dirty = true;
            }
            break;
        }
    }
    if(is_write)
    {
        c->stat_write_access++;
        if(result == MISS) c->stat_write_miss++;
    }
    else
    {
        c->stat_read_access++;
        if(result == MISS) c->stat_read_miss++;
    }
    #ifdef DEBUG
        printf("\t\tindex: %ld, tag: %ld, is_write: %d, core_id: %d\n", index, tag, is_write, core_id);
        if(result == MISS)
        {
            printf("\t\tMISS!\n");
        }
        else
        {
            printf("\t\tHit in the cache --> is_write: %d\n", is_write);
        }
    #endif
    return result;
}

/**
 * Install the cache line with the given address.
 * 
 * Also update the cache statistics accordingly.
 * 
 * This is intended to be implemented in part A.
 * 
 * @param c The cache to install the line into.
 * @param line_addr The address of the cache line to install (in units of the
 *                  cache line size, i.e., excluding the line offset bits).
 * @param is_write Whether this install is triggered by a write.
 * @param core_id The CPU core ID that requested this access.
 */
bool cache_install(Cache *c, uint64_t line_addr, bool is_write,
                   unsigned int core_id)
{
    // TODO: Use cache_find_victim() to determine the victim line to evict.
    // TODO: Copy it into a last_evicted_line field in the cache in order to
    //       track writebacks.
    // TODO: Initialize the victim entry with the line to install.
    // TODO: Update the appropriate cache statistics.
    bool dirty = false;
    uint64_t index = line_addr & c->index_mask;
    #ifdef DEBUG
        printf("\t\tInstalling into a cache (index: %ld)\n", index);
    #endif
    uint64_t way = cache_find_victim(c, static_cast<int>(index), core_id);
    c->lastLine = c->cacheSets[index].cacheLines[way];
    if(c->lastLine.dirty) 
    {
        dirty = true;
        c->stat_dirty_evicts++;
    }
    c->cacheSets[index].cacheLines[way].valid = true;
    c->cacheSets[index].cacheLines[way].tag = (line_addr & c->tag_mask) >> c->index_bits;
    c->cacheSets[index].cacheLines[way].accessTime = current_cycle;
    c->cacheSets[index].cacheLines[way].coreId = core_id;
    c->cacheSets[index].cacheLines[way].dirty = false;
    #ifdef DEBUG
        printf("\t\tNew cache line installed (dirty: %d, tag: %ld, core_id: %d, last_access_time: %ld)\n",
        c->cacheSets[index].cacheLines[way].dirty,c->cacheSets[index].cacheLines[way].tag, 
        c->cacheSets[index].cacheLines[way].coreId,c->cacheSets[index].cacheLines[way].accessTime);
    #endif
    return dirty;
}

/**
 * Find which way in a given cache set to replace when a new cache line needs
 * to be installed. This should be chosen according to the cache's replacement
 * policy.
 * 
 * The returned victim can be valid (non-empty), in which case the calling
 * function is responsible for evicting the cache line from that victim way.
 * 
 * This is intended to be initially implemented in part A and, for extra
 * credit, extended in parts E and F.
 * 
 * @param c The cache to search.
 * @param set_index The index of the cache set to search.
 * @param core_id The CPU core ID that requested this access.
 * @return The index of the victim way.
 */
unsigned int cache_find_victim(Cache *c, unsigned int set_index,
                               unsigned int core_id)
{
    // TODO: Find a victim way in the given cache set according to the cache's
    //       replacement policy.
    // TODO: In part A, implement the LRU and random replacement policies.
    // TODO: In part E, for extra credit, implement static way partitioning.
    // TODO: In part F, for extra credit, implement dynamic way partitioning.
    unsigned int way = 0;
    #ifdef DEBUG
        printf("\t\tLooking for victim to evict (policy: %d)...\n", c->replacementPolicy);
    #endif
    if(c->replacementPolicy == LRU)
    {
        uint64_t least = (1ULL << 63) - 1;
        for(unsigned int i = 0; i < c->ways; i++)
        {
            if(!c->cacheSets[set_index].cacheLines[i].valid)
            {
                way = i;
                break;
            }
            if(c->cacheSets[set_index].cacheLines[i].accessTime < least)
            {
                least = c->cacheSets[set_index].cacheLines[i].accessTime;
                way = i;
            }
        }
    }
    else if(c->replacementPolicy == RANDOM)
    {
        std::srand(std::time(0));
        way = std::rand() % (c->ways + 1);
    }
    #ifdef DEBUG
        if(!c->cacheSets[set_index].cacheLines[way].valid)
        {
            printf("\t\tFound a naive victim (valid bit not set, idx: %d)\n", way);
        }
        else
        {
            printf("\t\tFound a victim (policy_num: %d, idx: %d)\n", c->replacementPolicy, way);
            if(c->cacheSets[set_index].cacheLines[way].dirty)
            {
                printf("\t\tVictim was dirty!\n");
            }
        }  
    #endif
    return way;
}

/**
 * Print the statistics of the given cache.
 * 
 * This is implemented for you. You must not modify its output format.
 * 
 * @param c The cache to print the statistics of.
 * @param label A label for the cache, which is used as a prefix for each
 *              statistic.
 */
void cache_print_stats(Cache *c, const char *header)
{
    double read_miss_percent = 0.0;
    double write_miss_percent = 0.0;

    if (c->stat_read_access)
    {
        read_miss_percent = 100.0 * (double)(c->stat_read_miss) /
                            (double)(c->stat_read_access);
    }

    if (c->stat_write_access)
    {
        write_miss_percent = 100.0 * (double)(c->stat_write_miss) /
                             (double)(c->stat_write_access);
    }

    printf("\n");
    printf("%s_READ_ACCESS     \t\t : %10llu\n", header, c->stat_read_access);
    printf("%s_WRITE_ACCESS    \t\t : %10llu\n", header, c->stat_write_access);
    printf("%s_READ_MISS       \t\t : %10llu\n", header, c->stat_read_miss);
    printf("%s_WRITE_MISS      \t\t : %10llu\n", header, c->stat_write_miss);
    printf("%s_READ_MISS_PERC  \t\t : %10.3f\n", header, read_miss_percent);
    printf("%s_WRITE_MISS_PERC \t\t : %10.3f\n", header, write_miss_percent);
    printf("%s_DIRTY_EVICTS    \t\t : %10llu\n", header, c->stat_dirty_evicts);
}
