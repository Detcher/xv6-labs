/**
 * @author: Detcher
 * @time: 2023/5/1 20:09
 * @description: lab1.2
*/

#include "kernel/types.h"
#include "user/user.h"

int
main( int argc, char *argv[] )
{
    int n, pid, exit_status;
    int parent2child[2], child2parent[2];
    char tmp[2], buf[2] = {'P', 'Q'};

    pipe(parent2child);
    pipe(child2parent);

    pid = fork();
    if( pid < 0 ) {
        fprintf( 2, "ERROR: fork error:)\n" );

        close(parent2child[0]);
        close(parent2child[1]);
        close(child2parent[0]);
        close(child2parent[1]);
        exit(1);
    }
    else if( pid == 0 ) {
        // child
        close(parent2child[1]);
        close(child2parent[0]);
        
        if( (n = read( parent2child[0], &tmp[0], 1 )) < 0 ) {
            fprintf( 2, "ERROR: read error:)\n" );
            // exit(1);
            exit_status = 1;
        }
        else {
            printf("%d: received ping\n", getpid());
            if( write( child2parent[1], &buf[1], 1 ) != 1 ) {
                fprintf( 2, "ERROR: write error:)\n" );
                // exit(1);
                exit_status = 1;
            }
            // exit(0);
            exit_status = 0;
        }

        close(parent2child[0]);
        close(child2parent[1]);
        exit(exit_status);
    }
    else {
        // parent
        close(parent2child[0]);
        close(child2parent[1]);

        if( write( parent2child[1], &buf[0], 1 ) != 1 ) {
            fprintf( 2, "ERROR: write error:)\n" );
            // exit(1);
            exit_status = 1;
        }
        close(parent2child[1]);

        n = read( child2parent[0], &tmp[1], 1 );
        close(child2parent[0]);
        if( n < 0 ) {
            fprintf( 2, "ERROR: read error:)\n" );
            // exit(1);
            exit_status = 1;
        }
        else {
            printf("%d: received pong\n", getpid());
            // exit(0);
            exit_status = 0;
        }

        exit(exit_status);
    }
}
