/**
 * @author: Detcher
 * @time: 2023/5/3 10:31
 * @description: lab1.4
*/
/**
 * TEST Test
*/
#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

static const char *file_name;

void
find_in_dir(int fd, char *path)
{
    struct dirent de;
    struct stat st;
    char buf[512], *p;
    int recur_fd;

    if( strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf) ){
        printf("find_in_dir: path too long:)\n");
        return;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';

    while( read(fd, &de, sizeof(de)) == sizeof(de) ) {
        // inum == 0 means the entry in Directory is free, thus skip:)
        if( de.inum == 0 ) {
            continue;
        }

        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;

        if( strcmp(de.name, file_name) == 0 ) {
            printf("%s\n", buf);
        }

        recur_fd = open( buf, O_RDONLY );
        if( fstat(recur_fd, &st) < 0 ){
            printf("find_in_dir: cannot stat %s:)\n", buf);
            continue;
        }
        if( st.type == 1 && strcmp(de.name,".") != 0 && strcmp(de.name,"..") != 0 ) {
            find_in_dir( recur_fd, buf );
        }
        close(recur_fd);
    }
}

void 
find(char *path)
{
    int fd;
    struct stat st;

    if( (fd = open(path, 0)) < 0 ) {
        fprintf( 2, "find: cannot open %s:)\n", path );
        exit(1);
    }

    if( fstat(fd, &st) < 0 ){
        fprintf( 2, "find: cannot stat %s:)\n", path );
        close(fd);
        exit(1);
    }
    
    switch(st.type) {
        case T_DEVICE:
        case T_FILE:
            fprintf( 2, "find: INPUT isn't directory:)" );
            break;
        case T_DIR:
            find_in_dir( fd, path );
            break;
    }

    close(fd);
}

int
main( int argc, char *argv[] )
{
    if( argc != 3 ) {
        fprintf( 2, "ERROR: argument's format wrong:)\n" );
        exit(1);
    }

    file_name = argv[2];
    find(argv[1]);

    exit(0);
}
