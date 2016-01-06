/* 
  proclist.h - definiert den Datentyp "Prozess" zur internen 
                Repräsentation eines Prozesses
*/

typedef struct process {
	int pid, pgid, status;
	char * name;
	struct process *next;
}Process;

Process* newProcList(void);
void show(Process *h);
Process *newProcess(int pid, int pgid, char *string);
void append(Process *h, Process *p);
Process *lookup(Process *h, int pid);
Process *next(Process *p);
void cleanList(Process *h);
