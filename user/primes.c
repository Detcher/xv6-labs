/**
 * @author: Detcher
 * @time: 2023/5/2 14:40
 * @description: lab1.3
*/

#include "kernel/types.h"
#include "user/user.h"

void transit( int pipe_in[] )
{
    close(pipe_in[1]);
    int p, n;
    int pipe_out[2];

    read( pipe_in[0], &p, sizeof(int) );
    printf("prime %d\n", p);
    if( read( pipe_in[0], &n, sizeof(int) ) == 0 ) {
        close(pipe_in[0]);
        exit(0);
    }

    pipe(pipe_out);

    while(1) {
        if( n % p != 0 ) {
            write( pipe_out[1], &n, sizeof(int) );
        }
        
        if( read( pipe_in[0], &n, sizeof(int) ) == 0 ) {
            close(pipe_in[0]);
            break;
        }
    }

    if( fork() == 0 ) {
        transit( pipe_out );
    }
    else {
        close(pipe_out[0]);
        close(pipe_out[1]);

        wait(0);
    }
}

int
main( int argc, char *argv[] )
{
    int pid;
    int pipe_in[2];
    pipe(pipe_in);

    int buf_in[34];
    for( int i = 0; i < 34; ++i ) {
        buf_in[i] = i + 2;
    }
    if( write( pipe_in[1], buf_in, sizeof(buf_in) ) != sizeof(buf_in) ) {
        fprintf( 2, "ERROR: write error:)\n" );
        exit(1);
    }

    if( (pid = fork()) < 0 ) {
        fprintf( 2, "ERROR: fork error:)\n" );

        close(pipe_in[0]);
        close(pipe_in[1]);
        exit(1);
    }
    else if( pid == 0 ) {
        transit( pipe_in );
    }
    else {
        close(pipe_in[0]);
        close(pipe_in[1]);
        wait(0);
    }

    exit(0);
}
