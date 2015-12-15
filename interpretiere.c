#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>


#include "utils.h"
#include "listen.h"
#include "wortspeicher.h"
#include "kommandos.h"
#include "frontend.h"
#include "parser.h"
#include "variablen.h"
#include "process.h"

int interpretiere(Kommando k, int forkexec);

Process *h; /*wird von shell.c initialisiert*/
Process *p;

Process *newProcess(pid_t pid, char * string){
  Process *neu = malloc(sizeof (struct process));
  neu->pid = pid;
  neu->status = -1;
  neu->next = NULL;
  neu->name = string;
  return neu;
}

void append(Process *h, Process *p){
  p->next = h->next; 
  h->next = p;
  
}

void show(Process *h){
  Process *p = h;
  fputs("PID      PGID      STATUS       KOMMANDO\n", stderr);
  while(p != NULL){
    switch (p->status){
      case -1:
        printf("%d \t %d \t running      %s\n", p->pid, getpgid(p->pid), p->name);
        break;
      case 0:
        printf("%d \t %d \t exit(0)      %s\n", p->pid, getpgid(p->pid), p->name);
        break;
      case 1:
        printf("%d \t %d \t exit(1)      %s\n", p->pid, getpgid(p->pid), p->name);
        break;
      case 2:
        printf("%d \t %d \t exit(2)    %s\n", p->pid, getpgid(p->pid), p->name);
        break;
      case 3:
        printf("%d \t %d \t exit(3)    %s\n", p->pid, getpgid(p->pid), p->name);
        break;
      default:
        printf("%d \t signal(%02d)   %s\n", p->pid, p->status,p->name);
        break;
    }
    
    p = p->next;
  }
}

void cleanList(Process *h){
  Process *p = h;
  while(p != NULL){
    if(p->next != NULL && p->next->status != -1){
      p->next = p->next->next;
    }else{
      p = p->next;
    }
  }
}

void init(Process *p){
  h = p;
}

void do_execvp(int argc, char **args){
  execvp(*args, args);
  perror("exec-Fehler"); 
  fprintf(stderr, "bei Aufruf von \"%s\"\n", *args);
  exit(1);
}

int interpretiere_pipeline(Kommando k){ //???????
  /* NOT IMPLEMENTED */
  fputs("NOT IMPLEMENTED", stderr);
  return 0;
}

int umlenkungen(Kommando k){
  /* Umlenkungen bearbeiten */
  Liste ul = k->u.einfach.umlenkungen;
  Umlenkung *u;
  int fd;
  while(ul){
      u = listeKopf(ul);
      fd = -1;
      if(u->modus==READ){
        fd = open(u->pfad, O_RDONLY, 0640);
      }
      if(u->modus==WRITE){
        fd = open(u->pfad, O_WRONLY | O_CREAT | O_TRUNC, 0640);
      }
      if(u->modus==APPEND){
        fd = open(u->pfad, O_WRONLY | O_CREAT | O_APPEND, 0640);
      }
      if(fd==-1){perror("Could not open file! Does it exist?\n");}
      if(dup2(fd,u->filedeskriptor)==-1){perror("Error in dup2()!\n");};
      u = listeKopf(ul);
      fprintf(stderr, "%d%s %s\n", u->filedeskriptor, u->modus==READ ? "< " : u->modus==WRITE ? "> " : ">> ", u->pfad);
      ul = listeRest(ul);
  }

  if(ul)fputs("umlenkung(Kommando k)!\n", stderr);
  return 0;
}

int aufruf(Kommando k, int forkexec){
  /* Programmaufruf im aktuellen Prozess (forkexec==0)
     oder Subprozess (forkexec==1)
  */
  int child_state;
  int ret_wait;
  pid_t pid;
  pid_t pgid;
  char  * worte = malloc(100 * sizeof(char)); /*20 Zeichen*/
  strcpy(worte, k->u.einfach.worte[0]);

  if(forkexec == 1){
    pid=fork();
    pgid=pid;
    p = newProcess(0, worte);
    append(h,p);

    switch (pid){
    case -1:
      perror("Fehler bei fork"); 
      return(-1);
    case 0:
      if(umlenkungen(k)) 
        exit(1);
      do_execvp(k->u.einfach.wortanzahl, k->u.einfach.worte);
      abbruch("interner Fehler 001"); /* sollte nie ausgeführt werden */
    default:
      if(k->endeabwarten){
        ret_wait = waitpid(pid, &child_state, 0);
        if(ret_wait == pid){
          p->status = 0;
          if(child_state == 256){
            p->status = 1;
            return 1;
          }else{
            p->status = 0;
            return 0;
          }
        } 
      } else {
          fprintf(stderr, "PID: %d\n", pid);
          fprintf(stderr, "PGID: %d\n", setpgid(pid, pgid)); //??????
          return 0;
        }
    }
  } else if (forkexec == 0){
    /* nur exec, kein fork */
    if(umlenkungen(k))
      exit(1);
    do_execvp(k->u.einfach.wortanzahl, k->u.einfach.worte);
    abbruch("interner Fehler 001"); /* sollte nie ausgeführt werden */
    exit(1);
  }
  return 0;
}


int interpretiere_einfach(Kommando k, int forkexec){

  char **worte = k->u.einfach.worte;
  int anzahl=k->u.einfach.wortanzahl;

  if (strcmp(worte[0], "exit")==0) {
    switch(anzahl){
    case 1:
      exit(0);
    case 2:
      exit(atoi(worte[1]));
    default:
      fputs( "Aufruf: exit [ ZAHL ]\n", stderr);
      return -1;
    }
  }
  if (strcmp(worte[0], "cd")==0) {
    switch(anzahl){
    case 1:
      return chdir(getenv("HOME"));
    case 2: 
      return chdir(worte[1]);
    default:
      fputs("NOT IMPLEMENTED", stderr);
      return -1;
    }
  }
  if (strcmp(worte[0], "status")==0) {
    show(h);
    cleanList(h);
    return 0;
  }
  if (strcmp(worte[0], "clean")==0) {
    cleanList(h);
    return 0;
  }

  return aufruf(k, forkexec);
}

int interpretiere(Kommando k, int forkexec){
  int status = 0;

  switch(k->typ){
  case K_LEER:
    return 0;
  case K_EINFACH:
    return interpretiere_einfach(k, forkexec);
  case K_SEQUENZ:
    {
      Liste l = k->u.sequenz.liste;
      while(!listeIstleer(l)){
	       status=interpretiere ((Kommando)listeKopf(l), forkexec);
	       l=listeRest(l);
      }
    }
    return status;
  case K_PIPE:
    {
      status=interpretiere_pipeline(k);
    }
    return status;
  case K_UND:
    { 
      Liste l = k->u.sequenz.liste;
      while(!listeIstleer(l) && status == 0){
         status=interpretiere ((Kommando)listeKopf(l), forkexec);
         l=listeRest(l);
      }
      return status;
    }
  case K_ODER:
    {
      Liste l = k->u.sequenz.liste;
      while(!listeIstleer(l)){
         status=interpretiere ((Kommando)listeKopf(l), forkexec);
         l=listeRest(l);
         if(status == 0){
           return status;
         }
      }
      return status;
    }
  default:
    fputs("unbekannter Kommandotyp, Bearbeitung nicht implementiert\n", stderr);
    break;
  }
  return 0;
}

