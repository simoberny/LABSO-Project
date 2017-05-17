#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h> /* For S_IFIFO */
#include <fcntl.h>

int readLine(int fd, char *str){
	int n; 
	do{
		n = read(fd, str, 1);
	}while(n>0 && *str++ != '\0');

	return n>0;
}

int main(){
	int fd_tens_in;
	int fd_tens_out;

	char str[100];
	char message[100];
	do {
		fd_tens_out = open ("tens_pipe_in", O_WRONLY);
	} while (fd_tens_out == -1);
	unlink("tens_pipe_out");
	mknod ("tens_pipe_out", S_IFIFO, 0);
	chmod ("tens_pipe_out", 0660); 
	fd_tens_in = open ("tens_pipe_out", O_RDONLY);

	while (readLine (fd_tens_in, str)) {
		sprintf(message, "%s\n ", str);

		write (fd_tens_out, message, strlen(message) + 1);
	}

	close(fd_tens_in);
	close (fd_tens_out);
	unlink("tens_pipe_out");
	
}

