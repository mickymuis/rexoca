#include "rexoca.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <math.h>

#define TTABLE_GROW 4

struct rexoca_level_t*
createLevel( void  ) {
    struct rexoca_level_t* l =(struct rexoca_level_t*)malloc( sizeof( struct rexoca_level_t ) );
    l->next =0;
    l->state =NULL;
    l->size =0;
    return l;
}

void
addTo( struct rexoca_level_t* l, const char* extension, size_t length ) {
    size_t offset =0;
    if( l->state == NULL ) {
        l->state =(char*)malloc( sizeof( char ) * length );
        l->size =length;
    }
    else {
        offset =l->size;
        l->size += length;
        l->state =(char*)realloc( l->state, sizeof( char ) * l->size );
    }
    memcpy( l->state + offset, extension, length );
}

const char*
tail( struct rexoca_level_t* l, int mode ) {
    return l->size <= mode ? l->state : l->state + (l->size -  mode);
}

void
freeRecursive( struct rexoca_level_t* l ) {
    if( !l ) return;
    freeRecursive( l->next );
    free( l );
}

struct rexoca_t*
rexoca_create( int mode ) {
    struct rexoca_t* rexoca =(struct rexoca_t*)malloc( sizeof( struct rexoca_t ) );
    
    rexoca->mode =mode;
    rexoca->top =NULL;

    rexoca->transitionTable.entries =NULL;
    rexoca->transitionTable.count =0;

    return rexoca;
}

char
rexoca_push( struct rexoca_t* rexoca, char label ) {

    struct rexoca_level_t* level =rexoca->top;
    if( rexoca->transitionTable.entries == NULL )
        return -1;
    if( level == NULL )
        level = rexoca->top = createLevel();

    // Push the new label to the top level
    addTo( level, &label, 1 );
    
    // Expand all other levels
    while( 1 ) {
        label =0;

        if( level->size < rexoca->mode )
            return 0;

        for( int i =0; i < rexoca->transitionTable.count; i++ ) {
            struct ttable_entry_t* entry =rexoca->transitionTable.entries + i;
            if( strncmp( entry->input, tail( level, rexoca->mode ), rexoca->mode) == 0 ) {
                label =entry->output;
             break;
            }
        }

        if( label == 0 ) {
            // No match
            strncpy( rexoca->error, tail( level, rexoca->mode ), rexoca->mode );
            return 0;
        }

        if( level->next ) {
            level = level->next;
        //    printf( "Adding %c\n", label );
            addTo( level, &label, 1 );
        }
        else
            break;
    }

    // Add the new bottom level
    level = level->next = createLevel();
    addTo( level, &label, 1 );

    return label;
}

void
rexoca_initialize( struct rexoca_t* rexoca, const char* sequence ) {
    if( rexoca->top != NULL ) {
        freeRecursive( rexoca->top );
        rexoca->top =NULL;
    }
    size_t n =strlen( sequence );
    for( int i =0; i < n; i++ )
        rexoca_push( rexoca, sequence[i] );
}

int
rexoca_readTransitionTable( struct rexoca_t* rexoca, FILE* file ) {
    
    if( rexoca->transitionTable.entries )
        free( rexoca->transitionTable.entries );

    rexoca->transitionTable.count =0;

    int elems =0, i =0;
    struct ttable_entry_t* entries =(struct ttable_entry_t*)malloc( TTABLE_GROW * sizeof( struct ttable_entry_t ) );
    elems =TTABLE_GROW;

    while( !ferror( file ) && !feof( file ) ) {
        // grow if neccesary
        if( elems == i ) {
            entries =realloc( entries, (elems+=TTABLE_GROW)*sizeof( struct ttable_entry_t ) );
        }
        struct ttable_entry_t* entry = entries + i;
        int count;

        // input field
        count = fscanf( file, "%3s", &entry->input );
        if( count != 1 ) {
            if( count == EOF )
                break;
            return -1;
        }

        // output field
        if( fscanf( file, "%*c%c%*c", &entry->output ) != 1 ) {
            return -1;
        }

        fprintf( stderr, "'%s' -> '%c'\n", entry->input, entry->output );

        i++;
    }
    rexoca->transitionTable.count =i;
    rexoca->transitionTable.entries =entries;
    return i;
}

void 
varbase_set( int A[], int base, int length, int decimal ) {
    memset( A, 0, length * sizeof( int ) );
    int i =length -1;
    while( i >= 0 && decimal > 0 ) {
        A[i] = decimal % base;
        decimal = decimal / base;
        i--;
    }
}

void
varbase_incr( int A[], int base, int length ) {
    for( int i =length-1; i >= 0; i-- ) {
        if( (++(A[i])) < base ) 
            break;
        else
            A[i] =0;
    }
}

void
str_permute( char* dst, const char* src, int indices[], int length ) {
    for( int i =0; i < length; i++ ) {
        dst[i] = src[indices[i]];
    }
}

void
rexoca_genTransitionTable( struct rexoca_t* rexoca, const char* labels, int rule ) {

        const int base =strlen( labels );               // number of different labels
        const int mode =rexoca->mode;                   // mode, i.e. the neighborhood size
        const int rule_size =pow( base, mode );         // number of transitions in one rule
        const int max_rules =(int)pow( base, rule_size );
        if( rule >= max_rules ) {
           return;
        }

        int rule_base[rule_size];
        varbase_set( rule_base, base, rule_size, rule );

        int state_base[mode];
        memset( state_base, 0, mode*sizeof(int) );

        char state_str[mode+1];
        state_str[mode] = '\0';
        char output;

        // debug
        char rule_str[rule_size+1];
        rule_str[rule_size] ='\0';
        str_permute( rule_str, labels, rule_base, rule_size );
        printf( "Rule #%d -> '%s'\n", rule, rule_str );

        struct ttable_entry_t* entries =(struct ttable_entry_t*)malloc( TTABLE_GROW * sizeof( struct ttable_entry_t ) );
        
        for( int i = rule_size-1; i >= 0; i-- ) {
            printf( "state_base: %d %d \n", state_base[0], state_base[1] );
            str_permute( state_str, labels, state_base, mode );
            output =labels[rule_base[i]];

            printf( "'%s' -> '%c'\n", state_str, output ); 
            bzero( entries[i].input, 4 );
            memcpy( entries[i].input, state_str, mode );
            entries[i].output =output;

            varbase_incr( state_base, base, mode );
        }

        rexoca->transitionTable.entries =entries;
        rexoca->transitionTable.count =rule_size;
}

char
rexoca_advance( struct rexoca_t* rexoca ) {
    char label =0;
    struct rexoca_level_t* level =rexoca->top;
    if( !level ) return 0;

    while( level->next ) level =level->next; 
    if( level->size != 1 ) return 0;
    if( (label = level->state[0]) == 0 ) return 0;
    return rexoca_push( rexoca, label );
}

void
rexoca_free( struct rexoca_t* rexoca ) {

    freeRecursive( rexoca->top );
    if( rexoca->transitionTable.entries )
        free( rexoca->transitionTable.entries );
    free( rexoca );
}
