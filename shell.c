/*
  Shell-Beispielimplementierung

  Die Implementierung verzichtet aus Gründen der Einfachheit
  des Parsers auf eine vernünftige Heap-Speicherverwaltung:

  Falls der Parser die Analyse eines Kommandos wegen eines
  Syntaxfehlers abbricht, werden die ggf. vorhandenen
  Syntaxbäume für erfolgreich analysierte Unterstrukturen des
  fehlerhaften Kommandos nicht gelöscht.

  Beispiel: if test ! -d /tmp; then mkdir /tmp; else echo "/tmp vorhanden" fi

  Die Analyse wird mit Fehlermeldung abgebrochen, weil vor dem "fi" das
  obligatorische Semikolon fehlt. Im Heap stehen zu diesem Zeitpunkt die
  Bäume für das test- und das mkdir-Kommando. Diese verbleiben als Müll
  im Heap, da die Verweise ausschließlich auf dem Parser-Stack stehen,
  der im Fehlerfall nicht mehr ohne weiteres zugänglich ist.

  Um dies zu beheben, müsste man 
  a) sich beim Parsen die Zeiger auf die Wurzeln aller
     konstruierten Substruktur-Bäume solange in einer globalen Liste merken, 
     bis die Analyse der kompletten Struktur ERFOLGREICH beendet ist
  oder
  b) die Grammatik mit Fehlerregeln anreichern, in denen die Freigabe
     im Fehlerfall explizit vorgenommen wird.

  Da beides die Grammatik aber stark aufbläht, wird darauf verzichtet.
*/

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "utils.h"
#include "listen.h"
#include "wortspeicher.h"
#include "kommandos.h"
#include "frontend.h"
#include "parser.h"
#include "variablen.h"
#include "process.h"

extern int yydebug;
extern int yyparse(void);
extern int interpretiere(Kommando k, int forkexec);
extern void init(Process *p);

int shellpid;
Process *p;

Process *hshell;

Process *newProcList(){
  Process *neu = malloc(sizeof (struct process));
  neu->pid = shellpid;
  neu->pgid = getpgid(getpid());
  neu->status = -1;
  neu->next = NULL;
  neu->name = "shell";
  return neu;
}

Process *lookup(Process *h, int pid){
  Process *p = h;
  while(p->next != NULL){
    if(p->pid == pid){
      return p;
    }
    p = p->next;
  }
  return p;
}

Process *next(Process *p){
  return p->next;
}

void endesubprozess (int sig){
  pid_t pid;
  int child_state;
  pid = waitpid(-1, &child_state, WNOHANG);
  if(pid > 0){
    Process *p = lookup(hshell, pid);
    if(p == NULL){
      hshell->next->status = WEXITSTATUS(child_state) + WTERMSIG(child_state);
    }else{
      p->status = WEXITSTATUS(child_state) + WTERMSIG(child_state);
    }
  }
}

void sig_handler(int signum) {
  switch(signum) {
  case SIGINT: 
    fprintf(stderr, "  Beenden mit exit\n");
    fputs("Florian/Kevin>> ", stdout);
    fflush(stdout);
    break;
  case SIGTSTP:
    p->status = 4;
    kill(p->pid, SIGTERM);
    fprintf(stderr, "  Prozess: %d mit STRG Z Beendet\n", p->pgid);
    fputs("Florian/Kevin>> ", stdout);
    fflush(stdout);
    break;
/*case SIGCHLD:
    int ret_wp;
    while(ret_wp > 1) {
      ret_wp = waitpid(-1, &child_state, WNOHANG|WUNTRACED|WCONTINUED);
    }
    break; */
  default:
    fprintf(stderr, "Nothing happend\n");
  }
}

void init_signalbehandlung(){
  struct sigaction action;
  action.sa_handler = endesubprozess;
  action.sa_flags = SA_RESTART | SA_NODEFER;
  action.sa_flags &= ~SA_RESETHAND;
  if(sigaction(SIGCHLD,&action,NULL)==-1){
    perror("Error in Sigaction");
    exit(1);
  }
}

int main(int argc, char *argv[]){
  int zeigen=0, ausfuehren=1;
  int status, i;

  init_signalbehandlung();
  shellpid = getpid();
  hshell = newProcList();
  init(hshell);

  yydebug=0;

  for(i=1; i<argc; i++){
    if (!strcmp(argv[i],"--zeige"))
      zeigen=1;
    else if  (!strcmp(argv[i],"--noexec"))
      ausfuehren=0;
    else if  (!strcmp(argv[i],"--yydebug"))
      yydebug=1;
    else {
      fprintf(stderr, "Aufruf: %s [--zeige] [--noexec] [--yydebug]\n", argv[0]);
      exit(1);
    }
  }


  wsp=wortspeicherNeu();

  while(1){
    signal(SIGINT, sig_handler);
    signal(SIGTSTP, sig_handler);
    int res;
    fputs("Florian/Kevin>> ", stdout);
    fflush(stdout);
    res=yyparse();
    if(res==0){
      if(zeigen) 
        kommandoZeigen(k);
      if(ausfuehren) 
        status=interpretiere(k, 1);
      if(zeigen) 
        fprintf(stderr, "Status: %d\n", status);
      kommandoLoeschen(k);
    }
    else
      fputs("Fehlerhaftes Kommando\n", stderr);
    wortspeicherLeeren(wsp);
  }
}

