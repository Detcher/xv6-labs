/**
 * @author: Detcher
 * @time: 2023/5/3 13:50
 * @description: lab1.5
*/

#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

#define is_blank(ch) (ch == ' ' || ch == '\t')

int
main( int argc, char *argv[] )
{
    char ch, buf[64];
    char *args[MAXARG];
    int offset = 0;

    if( argc > MAXARG ) {
        fprintf( 2, "xargs: too many arguments passed:)\n" );
        return -1;
    }

    args[0] =  argv[1];
    for( int i = 1; i < argc - 1; ++i ) {
        args[i] = argv[i + 1];
    }

    while( read(0, &ch, 1) != 0 ) {
        if( is_blank(ch) == 1 ) {
            buf[offset++] = ' ';
            continue;
        }

        if( ch == '\n' ) {
            buf[offset] = 0;
            args[argc - 1] = buf;
            offset = 0;

            if( fork() == 0 ) {
                exec( argv[1], args );

                fprintf( 2, "xargs: exec error:)\n" );
                exit(1);
            }
            wait(0);
        }
        else {
            buf[offset++] = ch;
        }
    }

    return 0;
}
