/**
 * @author: Detcher
 * @time: 2023/5/1 19:15
 * @description: lab1.1
*/

#include "kernel/types.h"
#include "user/user.h"

int
main( int argc, char *argv[] )
{
    // exception handling
    if( argc != 2 ) {
        fprintf( 2, "ERROR: argument's format wrong:)\n" );
        exit(1);
    }

    sleep( atoi(argv[1]) );
    exit(0);
}
