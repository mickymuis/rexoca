#include "rexoca.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <math.h>

struct output_format_t {
    bool colors;
    bool masonry;
}; 

void printLabel( FILE* file, char label, bool colors ) {
    if( colors ) 
        fprintf( file, "\e[38;5;%dm\u2588\e[0m", (int)label );
    else
        fprintf( file, "%c", label );
}

void printLevel( FILE* file, struct rexoca_level_t* l, struct output_format_t format, int mode, int n ) {
    if( format.masonry )
        for( int i=0; i < n; i++ )
            fprintf( file, " " );

    for( int i=0; i < l->size; i++ ) {
        printLabel( file, *(l->state+i), format.colors );
        if( mode == 2 && format.masonry ) {
            if( format.colors )
                printLabel( file, *(l->state+i), format.colors );
            else
                fprintf( file, " " );
        }
    }
    fprintf( file, "\n" );
}

void printRexoca( FILE* file, struct rexoca_t* rexoca, struct output_format_t format  ) {
    struct rexoca_level_t* next = rexoca->top;
    int n =0;

    while( next ) {
        printLevel( file, next, format, rexoca->mode, n++ );
        next =next->next;
    }
}

void print_help( char* argv0) {
    printf( "RExOCA - Recursively Expanding One-dimensional Cellular Automata\n \
usage: %s N [-t transition_table | -r rule -l labels -m mode] -i initial_sequence [-o output] [-cb]\n \
\n \
\t N             \t Number of iterations. \n \
\t -r \n \
\t --rule        \t Set the rule number, used in conjunction with --labels and --mode\n \
\t -l \n \
\t --labels      \t Specify any number of labels. A labels is exactly one character long.\n \
\t -m \n \
\t --mode [2|3]  \t Sets neighborhood size of 2 or 3 when generating transition tables.\n \
\t -t \n \
\t --ttable      \t Specify file that contains white-space separated table of transitions.\n \
\t -i \n \
\t --initial     \t Specifies initial sequence.\n \
\t -o \n \
\t --output      \t Optional output file.\n \
\t -c \n \
\t --colors      \t Color labels according to their ASCII value.\n \
\t -b \n \
\t --masonry     \t Print output in masonry composition.\n \
\t --help        \t This help.\n \
");
}

int
main( int argc, char** argv ) {

    bool has_N =false, has_rule =false, has_labels =false, has_ttable =false, has_initial =false, 
         has_output =false, use_color =false, has_mode =false;
    unsigned int N =0;
    int index_labels =0, index_ttable =0, index_initial =0, index_output =0, set_mode =0;
    unsigned long set_rule =0l;
    struct output_format_t format;
    format.colors =false;
    format.masonry =false;

    for( int i = 1; i < argc; i++ ) {
        if( strncmp( argv[i], "-m", 2 ) == 0 || strncmp( argv[i], "--mode", 6 ) == 0 ) {
            has_mode =true;
            set_mode =atoi( argv[++i] );
            if( set_mode != 2 && set_mode != 3 ) {
                fprintf( stderr, "Invalid mode specified.\n" );
                return -1;
            }
        } else if( strncmp( argv[i], "-r", 2 ) == 0 || strncmp( argv[i], "--rule", 6 ) == 0 ) {
            has_rule =true;
            set_rule =atoi( argv[++i] );
        } else if( strncmp( argv[i], "-l", 2 ) == 0 || strncmp( argv[i], "--labels", 8 ) == 0 ) {
            has_labels =true;
            index_labels =++i;
        } else if( strncmp( argv[i], "-t", 2 ) == 0 || strncmp( argv[i], "--ttable", 8 ) == 0 ) {
            has_ttable =true;
            index_ttable =++i;
        } else if( strncmp( argv[i], "-i", 2 ) == 0 || strncmp( argv[i], "--initial", 9 ) == 0 ) {
            has_initial =true;
            index_initial =++i;
        } else if( strncmp( argv[i], "-o", 2 ) == 0 || strncmp( argv[i], "--output", 8 ) == 0 ) {
            has_output =true;
            index_output =++i;
        } else if( strncmp( argv[i], "--color", 7 ) == 0 ) {
            format.colors =true;
        } else if( strncmp( argv[i], "--masonry", 9 ) == 0 ) {
            format.masonry =true;
        } else if( strncmp( argv[i], "--help", 6 ) == 0 ) {
            print_help ( *argv );
            return 0;
        } else if( argv[i][0] == '-' ) {
            for( int j =1; j < strlen( argv[i] ); j++ ) {
                if( argv[i][j] == 'b' )
                    format.masonry =true;
                else if( argv[i][j] == 'c' )
                    format.colors =true;
                else {
                    fprintf( stderr, "Invalid flag '%c'.\n", argv[i][j] );
                    return -1;
                }
            }
        }
        else {
            N =atoi( argv[i] );
            if( !N || has_N ) {
                fprintf( stderr, "Invalid argument.\n" ); 
                return -1;
            }
            else has_N =true;
        }
    }
    if( !has_N || !has_ttable && (!has_rule || !has_labels || !has_mode) || !has_initial ) {
        fprintf( stderr, "Insufficient arguments.\n" );
        return -1;
    }
    if( has_rule && has_ttable ) {
        fprintf( stderr, "Both rule and transition table given.\n" );
        return -1;
    }

    FILE* ttable;
    if( has_ttable ) {
        ttable =fopen( argv[index_ttable], "r" );
        if( !ttable ) {
            fprintf( stderr, "Error opening %s - %s\n", argv[index_ttable], strerror(errno) );
            return -1;
        }
    }

    int mode;

    if( has_mode )
        mode =set_mode;
    else if( has_ttable ) {
        char temp[4];
        if( !fscanf( ttable, "%3s", temp ) ) {
            fprintf( stderr, "Unexpected format of transition table." );
            return -1;
        }
        mode = strlen( temp );
        rewind( ttable );
    }
    printf( "mode: %d\n", mode );

    unsigned long max_rules =0;
    if( has_rule ) {
        max_rules =(unsigned long)pow( strlen( argv[index_labels] ), pow( strlen( argv[index_labels] ), mode ) );
        printf( "number of rules: %d\n", max_rules );
        if( set_rule >= max_rules ) {
            fprintf( stderr, "Given impossible rule #%d, only %d combinations possible.\n", set_rule, max_rules );
            return -1;
        }
    }

    struct rexoca_t* rexoca;
    rexoca =rexoca_create( mode ); 
    
    if( has_ttable ) {
        int size =rexoca_readTransitionTable( rexoca, ttable );
        if( size <= 0 ) {
            fprintf( stderr, "Error reading transition table.\n" );
            return -1;
        }
    } else if( has_rule ) {
        rexoca_genTransitionTable( rexoca, argv[index_labels], set_rule );
    }

    // Set initial state
    if( strlen( argv[index_initial] ) < mode ) {
        fprintf( stderr, "Error: intial state to small.\n" );
        return -1;
    }
    rexoca_initialize( rexoca, argv[index_initial] );
    printRexoca( stdout, rexoca, format );

    for( int i =0; i < N; i++ ) {
        if( rexoca_advance( rexoca ) == 0 ) {
            fprintf( stderr, "Error: no transition for '%s'\n", rexoca->error );
            return -1;
        }
    }

    printRexoca( stdout, rexoca, format );
    
    rexoca_free( rexoca );

    return 0;
};
