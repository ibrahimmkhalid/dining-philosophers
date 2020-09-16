#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "common.h"

#define LEFT(x) (((x) + N - 1) % N)
#define RIGHT(x) (((x) + 1) % N)
#define STATE_THINKS 0
#define STATE_HUNGRY 1
#define STATE_EATING 2

// philosopher related function declarations
void* philosopher(void*);  // Thread for each of the philosophers' logic
void take_forks(int);      // philosopher tries to eat
void put_forks(int);       // philosopher stops eating, allowing others to eat
void checkStates(int);     // philosopher checks left and right of them before eating
void randomDelay();        // delay function random time between 1 to 3 seconds (inclusive)

// viewer thread function declarations
void* internal_viewer();   // thread to view philosopher states in main terminal
void* external_viewer();   // thread to view philosopher states in their own terminals

// global program flow control variables
int kill_threads = 0;      // when 1, threads stop and end
int external_display = 0;  // boolean to control code for external viewer vs internal viewer
int viewer_sync = 1;       // when 0, update new states

// global variables
sem_t mutex;               // mutual exclusion semaphore
sem_t* phil_sem;           // semaphore to signify whether philosopher is waiting to eat (hungry)
int* state;                // stores states for every philosopher
int N;                     // number of philosophers
int checkAllStarted = 0;   // makes sure all philosophers have started before taking user input

int main(int argc, char* argv[]) {
  // check for proper input format
  if (argc != 3) {
    printf("Invalid Parameters!\n");
    printf("Usage:\n");
    printf("\t./main [e|i] n\n\n");
    printf("options:\n");
    printf("\t[e|i]\t <e> - external terminal windows for each philosopher\n");
    printf("\t\t <i> - internal terminal windows for all philosophers\n");
    printf("\tn\t Number of philosophers\n\n");
    return -1;
  }

  N = atoi(argv[2]);
  if (N < 1) return -1;
  char c = *argv[1];

  // check viewer option
  if (c == 'e')
    external_display = 1;
  else if (c == 'i')
    external_display = 0;
  else {
    printf("Invalid Parameters!\n");
    printf("Usage:\n");
    printf("\t./main [e|i] n\n\n");
    printf("options:\n");
    printf("\t[e|i]\t <e> - external terminal windows for each philosopher\n");
    printf("\t\t <i> - internal terminal windows for all philosophers\n");
    printf("\tn\t Number of philosophers\n\n");
    return -1;
  }

  // allocations and initializations
  time_t t;
  srand((unsigned)time(&t));  // true random seed for rand() function
  phil_sem = (sem_t*)malloc(N * sizeof(sem_t));
  state = (int*)malloc(N * sizeof(int));
  sem_init(&mutex, 0, 1);
  for (int i = 0; i < N; i++) sem_init(&phil_sem[i], 0, 0);
  int id[N];
  for (int i = 0; i < N; i++) id[i] = i;

  // start external display processes if opted
  pid_t helper_pid[N];
  if (external_display) {
    // Create pipe files for communication
    for (int i = 0; i < N; i++) {
      char* fn = getFifoName(i);
      mkfifo(fn, 0666);
    }
    printf("Created Fifo files\n");
    for (int i = 0; i < N; i++) {
      helper_pid[i] = fork();
      if (helper_pid[i] == 0) {
        printf("Process for external viewer of philosopher %d started\n", i);
        char command[MAXSTR];
        sprintf(command, "xterm -e \"./helper %d\"", i);
        system(command);  // start xterm for external viewer program
        printf("Process for external viewer of philosopher %d exiting\n", i);
        exit(0);
      }
    }
  }

  // start multithreaded
  pthread_t T_philosopher[N];
  for (int i = 0; i < N; i++) pthread_create(&T_philosopher[i], NULL, philosopher, (void*)&id[i]);

  while (checkAllStarted < N)
    ;
  printf("Press Enter to terminate all processes and threads\n");

  pthread_t T_viewer;
  if (external_display)
    pthread_create(&T_viewer, NULL, external_viewer, NULL);
  else
    pthread_create(&T_viewer, NULL, internal_viewer, NULL);

  getchar();
  kill_threads = 1;  // signal to all threads to end
  viewer_sync = 0;   // prevents unnecessary outputs

  printf("Please wait while all processes and threads are terminated\n");
  printf("This can take a while\n");
  for (int i = 0; i < N; i++) pthread_join(T_philosopher[i], NULL);
  pthread_join(T_viewer, NULL);
  printf("All processes and threads have ended properly\n");

  if (external_display) {
    for (int i = 0; i < N; i++) {
      char* fn = getFifoName(i);
      unlink(fn);
    }
    printf("Unlinked Fifo files\n");
  }
}

void* philosopher(void* T_arg) {
  int t_id = *(int*)T_arg;
  printf("Thread for philosopher %d started\n", t_id);
  sem_wait(&mutex);
  checkAllStarted++;
  sem_post(&mutex);
  while (1) {
    if (kill_threads) break;
    randomDelay();  // simulating time to think
    take_forks(t_id);
    randomDelay();  // simulating time to eat
    put_forks(t_id);
  }
  printf("Thread for philosopher %d exiting\n", t_id);
  pthread_exit(NULL);
}

void take_forks(int id) {
  sem_wait(&mutex);
  state[id] = STATE_HUNGRY;
  checkStates(id);
  viewer_sync = 0;
  sem_post(&mutex);
  sem_wait(&phil_sem[id]);
}

void put_forks(int id) {
  sem_wait(&mutex);
  state[id] = STATE_THINKS;
  viewer_sync = 0;
  checkStates(LEFT(id));
  checkStates(RIGHT(id));
  sem_post(&mutex);
}

void checkStates(int id) {
  if (state[id] == STATE_HUNGRY && state[LEFT(id)] != STATE_EATING &&
      state[RIGHT(id)] != STATE_EATING) {
    state[id] = STATE_EATING;
    sem_post(&phil_sem[id]);
  }
}

void randomDelay() { sleep(rand() % 3 + 1); }

void* internal_viewer() {
  char* S = (char*)malloc(MAXSTR * sizeof(char));
  char c;
  printf("Philosopher ID |");
  for (int i = 0; i < N; i++) {
    printf("\t%d", i);
  }
  printf("\n");
  while (1) {
    while (viewer_sync)
      ;
    viewer_sync = 1;
    if (kill_threads) break;
    sprintf(S, "States         |");
    for (int i = 0; i < N; i++) {
      if (state[i] == STATE_EATING)
        c = 'E';
      else if (state[i] == STATE_HUNGRY)
        c = 'H';
      else if (state[i] == STATE_THINKS)
        c = 'T';
      sprintf(S, "%s\t%c", S, c);
    }
    printf("\n%s", S);
    fflush(stdout);
  }
  free(S);
}

void* external_viewer() {
  char* S;
  char* pfn[N];
  int pfd[N];
  for (int i = 0; i < N; i++) {
    pfn[i] = getFifoName(i);
    pfd[i] = open(pfn[i], O_WRONLY);
  }
  while (1) {
    while (viewer_sync)
      ;
    viewer_sync = 1;
    if (kill_threads) {
      int wc = 0;
      for (int i = 0; i < N; i++) {
        wc = write(pfd[i], EXITSTR, strlen(EXITSTR));
        printf("Writing %s to /tmp/phil_%d - return value: %d\n", EXITSTR, i, wc);
        close(pfd[i]);
      }
      break;
    }
    for (int i = 0; i < N; i++) {
      if (state[i] == STATE_EATING)
        S = "I am eating right now  \n";
      else if (state[i] == STATE_HUNGRY)
        S = "I am hungry right now  \n";
      else if (state[i] == STATE_THINKS)
        S = "I am thinking right now\n";
      write(pfd[i], S, strlen(S));
    }
  }
  for (int i = 0; i < N; i++) {
    free(pfn[i]);
    close(pfd[i]);
  }
}
