/* 
  proclist.h - definiert den Datentyp "Prozess" zur internen 
                Repräsentation eines Prozesses
*/

typedef struct process {
	int pid, status;
	char * name;
	struct process *next;
}Process;
