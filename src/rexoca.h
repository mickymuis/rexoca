#ifndef LINEXA_H
#define LINEXA_H
#include <stdio.h>

#define MAX_STATESIZE 16
#define MAX_STATESIZE_STR "16"

struct ttable_entry_t {
    char input[4];
    char output;
};

struct ttable_t {
    struct ttable_entry_t *entries;
    int count;
};

struct rexoca_level_t {
    char* state;
    size_t size;
    struct rexoca_level_t* next;
};

struct rexoca_t {
    int mode;
    struct ttable_t transitionTable;
    struct rexoca_level_t* top;
    char error[4];
};

struct rexoca_t*
rexoca_create( int mode );

void
rexoca_initialize( struct rexoca_t*, const char* sequence );

int
rexoca_readTransitionTable( struct rexoca_t*, FILE* );

void
rexoca_genTransitionTable( struct rexoca_t*, const char* labels, unsigned long rule );

char
rexoca_advance( struct rexoca_t* );

void
rexoca_free( struct rexoca_t* );

#endif
