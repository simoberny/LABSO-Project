#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <fcntl.h>
#include <signal.h>

#define ANSI_COLOR_RED "\x1b[31m"

#if (defined TARGET)
	#include <wiringPi.h>
#endif  

//Dichiarazioni pipe
int fd_tens_in;
int fd_tens_out;

int fd_units_in;
int fd_units_out;

//Vettore con la funzione di tradurre un numero nel suo corrispettivo a 7 segmenti
const int segmenti[10][7]={{1,1,1,1,1,1,0},{0,1,1,0,0,0,0},{1,1,0,1,1,0,1},{1,1,1,1,0,0,1},{0,1,1,0,0,1,1},{1,0,1,1,0,1,1},{1,0,1,1,1,1,1},{1,1,1,0,0,0,0,},{1,1,1,1,1,1,1},{1,1,1,1,0,1,1}};  //matrice che mappa ogni  segmento(riga) con ogni numero(colonna)

const int gpioTens[7]={3,12,30,14,13,0,2}; //mappa i pin della libreria wiring con i segmenti per le decine
const int gpioUnits[7]={16,1,21,23,25,15,22}; //mappa i pin della libreria wiring con i segmenti per le unità

/* Codice per generazione 7 segmenti nella bash (Stack Overflow) */
#define C(x) ((_[*n-'0'])>>s)&m&x
#define P putchar

unsigned _[]={476,144,372,436,184,428,492,148,508,188};

void p(int a,char*n,unsigned m,int s){
	for(;isdigit(*n);++n){
		P(C(1)?'|':' ');

		int i;
		for(i=0;i<a;++i){
			P(C(4)?'_':' ');
		}

		P(C(2)?'|':' ');
	}

	P('\n');
}
void l(int a,char*b){
	p(a,b,7,0);
	int i=1;
	for(;i<a;++i){
		p(a,b,3,3);
	}
	p(a,b,7,3);
	i=1;
	for(;i<a;++i){
		p(a,b,3,6);
	}
	p(a,b,7,6);
}
/* __________________________________________________*/

//Inizializza tutti i pin della gpio come uscite e setta il loro valore a alto(7 Segmenti a catodo comune)
void gpioInita(){
	#if (defined TARGET)
		int i;
		for(i=0; i<7; i++){
			wiringPiSetup () ;
	  		pinMode (gpioTens[i], OUTPUT);
	  		digitalWrite (gpioTens[i], HIGH);
	  		pinMode (gpioUnits[i], OUTPUT);
	  		digitalWrite (gpioUnits[i], HIGH);		
		}	
	#endif  
}

//Crea le pipe in ingresso (Ricevono dati da tens e units)
void creazionePipe(){
	//PIPE DI LETTURA DECINE
	unlink("tens_pipe_in");
	mknod ("tens_pipe_in", S_IFIFO, 0);
	chmod ("tens_pipe_in", 0660);
	fd_tens_in = open ("tens_pipe_in", O_NONBLOCK);

	//PIPE DI LETTURA UNITA'
	unlink("units_pipe_in");
	mknod ("units_pipe_in", S_IFIFO, 0);
	chmod ("units_pipe_in", 0660);
	fd_units_in = open ("units_pipe_in", O_NONBLOCK);
}

//Elimina tutte le pipe
void closePipe(){
	close (fd_tens_out);
	close(fd_tens_in);
	close (fd_units_out);
	close(fd_units_in);
	unlink("tens_pipe_in");
	unlink("units_pipe_in");
}

//Legge stringhe in ingresso, carattere per carattere, dato il file descriptor della pipe
int readLine(int fd, char *str){
	int n; 
	do{
		n = read(fd, str, 1);
	}while(n>0 && *str++ != '\0');

	return n>0;
}

//Funzione che dato il nome di un processo restituisce un PID
int getExPid(char* process){
	char comand[29];
	sprintf(comand, "pidof -s %s", process);

	FILE *ls = popen(comand, "r"); //A differenza di system restituisce la risposta del terminale
	char buf[256];
	while (fgets(buf, sizeof(buf), ls) != 0) {}

	pclose(ls);
	return atoi(buf);
}

//Funzione che inizializza il conto alla rovescia
void start(int sec){
	char strin_tens[100];
	char strin_units[100];	
	char strout_tens[100];
	char strout_units[100];

	//Scomposizione secondi in UNITA e DECINE
	int unita = sec%10;
	int decine = (sec)/10;

	printf("\nConto alla rovescia di %d iniziato!\n", sec);

	sprintf(strout_tens, "tens %d", decine);
	sprintf(strout_units, "units %d", unita);

	//Esecuzione figli indipendenti
	system("./tens &");
	system("./units &");

	//Apertura Pipe di scrittura
	do {
		fd_tens_out = open ("tens_pipe_out", O_WRONLY);
	} while (fd_tens_out == -1);

	do {
		fd_units_out = open ("units_pipe_out", O_WRONLY);
	} while (fd_units_out == -1);

	//Invia unità e decine ai rispettivi processi per iniziare il conto alla rovescia
	write (fd_tens_out, strout_tens, strlen(strout_tens) + 1);
	write (fd_units_out, strout_units, strlen(strout_units) + 1);
}

//Funzione che interroga unità e decine sul loro stato e restituisce il tempo rimanente
void elapsed(){
	char decine[10];
	char unita[10];

	//Se i processi sono ancora in esecuzione vengono richiesti i valori

	if(getExPid("tens") != 0){
		write (fd_tens_out, "elapsed", strlen("elapsed") + 1);

		while(!readLine(fd_tens_in, decine)){
		}

	}else{
		strcpy(decine, "0");
	}

	if(getExPid("units") != 0){
		write (fd_units_out, "elapsed", strlen("elapsed") + 1);

		while(!readLine(fd_units_in, unita)){
		}
	}else{
		strcpy(unita, "0");
	}

	//printf("Tempo rimanente: %s%s\n", decine, unita);

	//Stampa più elaborata con rappresentazione a 7 segmenti
	char numero[10];
	sprintf(numero, "%s%s", decine, unita);
	l(3, numero);
}

//Ferma il timer inviando un comando ai due processi
void stop(){
	if(getExPid("tens") != 0){
		write (fd_tens_out, "stop", strlen("stop") + 1);
	}

	if(getExPid("units") != 0){
		write (fd_units_out, "stop", strlen("stop") + 1);
	}

	gpioInita();
}

void printTens(){
	if(getExPid("tens") != 0){
		write (fd_tens_out, "print", strlen("print") + 1);
	}
}

void printUnits(){
	if(getExPid("units") != 0){
		write (fd_units_out, "print", strlen("print") + 1);
	}
}

//Dato il segmento restituisce il suo stato
void getTens(int led){
	if(led > 0 && led <= 7){
		if(getExPid("tens") != 0){
			char message[100];
			sprintf(message, "info %d", led-1);
			write (fd_tens_out, message, strlen(message) + 1);
		}
	}else{
		printf("Il numero non rappresenta un segmento!\n");
	}
}

//Dato il segmento e un colore setta il colore di quel segmento
void setTens(int led, char * color){
	if(led > 0 && led <= 7){
		if(getExPid("tens") != 0){
			char message[100];
			sprintf(message, "color %d %s", led-1, color);
			write (fd_tens_out, message, strlen(message) + 1);
		}
	}else{
		printf("Il numero non rappresenta un segmento!\n");
	}
}

//Dato il segmento restituisce il suo stato
void getUnits(int led){
	if(led > 0 && led <= 7){
		if(getExPid("units") != 0){
			char message[100];
			sprintf(message, "info %d", led-1);
			write (fd_units_out, message, strlen(message) + 1);
		}
	}else{
		printf("Il numero non rappresenta un segmento!\n");
	}
}

//Dato il segmento e un colore setta il colore di quel segmento
void setUnits(int led, char * color){
	if(led > 0 && led <= 7){
		if(getExPid("units") != 0){
			char message[100];
			sprintf(message, "color %d %s", led-1, color);
			write (fd_units_out, message, strlen(message) + 1);
		}
	}else{
		printf("Il numero non rappresenta un segmento!\n");
	}
}

void pipeHandler(int sig){
	//printf("tentato di scrivere su una pipe vuota");
}

void ctrlc(int sig){
	printf("\nProgramma terminato!\n");
	stop();
	usleep(100000);
	exit(0);
}

int main(int argc, char *argv[]){	
	void pipeHandler (int);
	signal (SIGPIPE, pipeHandler);

	void ctrlc (int);
	signal (SIGINT, ctrlc);
	
	gpioInita();

	char comando[100];
	char directory[100];
	char ledcolor[50];
	int secondi = -1;
	int led;
	int secimp = 0;
	
	FILE *fd;
	
	creazionePipe();

	//Se al programma vengono passati i secondi attraverso la console, questi vengono salvati in un file
	if(argc > 1){
		secimp = atoi(argv[1]);
		if(secimp > 0 && secimp < 60){
			fd=fopen("../assets/testmode.txt", "ab+");
			fprintf(fd, "%d", secimp);
			fclose(fd);

			//Conto alla rovescia avviato con i secondi presi dai parametri in ingresso
			start(secimp);
		}
	}

	do{	
		//Piccolo ritardo per visualizzare il risultato dei figli prima del menu dinamico
		usleep(200000);
		printf("\n	Comandi disponibili: \n	-start <secondi>: Avvio countdown\n	-elapsed: Secondi rimasti\n	-stop: Ferma il conto alla rovescia\n	-tens: Decine\n	-units: Unità\n	-tensled info <n>\n	-unitsled info <n>\n	-tensled color <n> <color>\n	-unitsled color <n> <color>\n	-quit\n");
		printf("-----------------\n \x1b[36m Comando > \x1b[0m");
		if (fgets (comando , 100 , stdin) != NULL ){
			system("clear");
			int l= strlen(comando);//togliamo l'invio che prende dentro in automatico
			comando[l-1] = '\0';
			
			if(strncmp(comando, "start", 5) == 0){
				sscanf(comando, "start %d", &secondi);
				if(secondi > 0 && secondi < 60){
					if(getExPid("tens") == 0 && getExPid("unita") == 0){
						start(secondi);
					}else{
						printf("\x1b[31m Timer già avviato!\n \x1b[0m");
					}
				}else{
					printf("\x1b[31m \nInserire un tempo valido\n \x1b[0m");				
				}
			}else if(strcmp(comando, "elapsed") == 0){
				elapsed();
			}else if(strcmp(comando, "stop") == 0){
				stop();
			}else if(strcmp(comando, "tens") == 0){
				printTens();
			}else if(strcmp(comando, "units") == 0){
				printUnits();
			}else if(strncmp(comando, "tensled info", 12) == 0){
				sscanf(comando, "tensled info %d", &led);
				getTens(led);
			}else if(strncmp(comando, "unitsled info", 13) == 0){
				sscanf(comando, "unitsled info %d", &led);
				getUnits(led);
			}else if(strncmp(comando, "tensled color", 13) == 0){
				sscanf(comando, "tensled color %d %s", &led, ledcolor);
				setTens(led, ledcolor);
			}else if(strncmp(comando, "unitsled color", 14) == 0){
				sscanf(comando, "unitsled color %d %s", &led, ledcolor);
				setUnits(led, ledcolor);
			}else if(strcmp(comando, "quit") != 0){
				printf("\x1b[31m \nComando errato\n \x1b[0m");
			}
		}
		
		//Ciclo console dinamica eseguita fino alla ricezione del comando "quit"
	}while(!(strcmp(comando, "quit") == 0));

	stop();
	closePipe();

	return 0;
}
