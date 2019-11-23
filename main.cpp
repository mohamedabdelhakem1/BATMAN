#include <bits/stdc++.h>
#include <pthread.h>
#include <unistd.h>
#include<ctime>

#define NORTH 0
#define EAST 1
#define SOUTH 2
#define WEST 3
using namespace std;
// type definition
void initialize();
void *check(void* args);
void *batMan(void * attr);
void *BAT(void *attr);
void arrive(struct bat *b);
void cross(struct bat *b);
void leave(struct bat *b);
struct bat {
    int no;
    int dirNO;
public:
    bat(int _no, int _dirNO): no(_no), dirNO(_dirNO) {
    }
};
//deadlock checker thread
pthread_t deadlock_checker;
/**mutex and condition variables */
pthread_mutex_t bat_mutex;
pthread_cond_t dir_queue[4];
pthread_cond_t dir_First[4];
// mapping letters to directions' numbers
unordered_map <char, int> dir_map;
// mapping directions' numbers to words
unordered_map <int,string> num_map;
/** counters*/
int dir_queue_size[4];
// to keep track of the waiting bats at each direction of the crossing
int cross_waiting_bats = 0;
// to keep track of count of the total threads currently in the system till they exit
int threads_in_system = 0;

void initialize(int n) {
    dir_map = {{'n',NORTH},{'e',EAST},{'s',SOUTH},{'w',WEST},{'N',NORTH},{'N',EAST},{'S',SOUTH},{'W',WEST}};
    num_map = {{0,"NORTH"},{1,"EAST"},{2,"SOUTH"},{3,"WEST"}};
    memset(dir_queue_size, 0,sizeof(dir_queue_size));

    pthread_mutex_init(&bat_mutex,nullptr);
    for(int i = 0 ; i < 4; i++) {
        pthread_cond_init(&dir_queue[i],nullptr);
        pthread_cond_init(&dir_First[i],nullptr);
    }
    threads_in_system = n;
    //creating thread checking for deadlock
    pthread_create(&deadlock_checker,nullptr,check,nullptr);
}
void *check(void* args) {
    bool detected  = false;
    while(threads_in_system) {
        sleep(2);
        if(!detected && cross_waiting_bats > 3/** deadlock condition*/) {
            printf("DEADLOCK: BAT jam detected, signaling NORTH  to go\n");
            pthread_cond_signal(&dir_First[WEST]);
            detected = true;
        } else if(cross_waiting_bats <= 3) {
            detected = false;
        }
    }
    pthread_exit(NULL);
}
void destroy() {
    pthread_mutex_destroy(&bat_mutex);
    for(int i = 0 ; i < 4; i++) {
        pthread_cond_destroy(&dir_queue[i]);
        pthread_cond_destroy(&dir_First[i]);
    }
}
void *batMan(void * attr) {

    char *seq = (char *)attr;
    int n = strlen(seq);
    // initialize the locks and condition variables and the deadlock checker thread
    initialize(n);

    //creating threads for sequence of bats
    pthread_t bat_threads[n];
    for(int i = 0 ; i < n ; i++) {
        struct bat *b = new bat(i,dir_map[seq[i]]);
        pthread_create(&bat_threads[i],nullptr,BAT,(void*)b);
    }
    // joining threads for sequence of bats
    for(int  i = 0 ; i < n ; i++) {
        pthread_join(bat_threads[i],nullptr);
    }
    pthread_join(deadlock_checker,nullptr);

    destroy();

    pthread_exit(nullptr);

}
void *BAT(void *attr) {
    struct bat *b = (struct bat*) attr;
    arrive(b);
    cross(b);
    leave(b);
    pthread_exit(nullptr);
}
void arrive(struct bat *b) {
    pthread_mutex_lock(&bat_mutex);
    dir_queue_size[b->dirNO]++;
    if(dir_queue_size[b->dirNO] > 1) {
        /**for tracing*/
        //printf("at %s BAT %d is waiting in direction queue\n", num_map[b->dirNO].c_str() ,b->no);
        /**for tracing*/

        pthread_cond_wait(&dir_queue[b->dirNO], &bat_mutex); // waiting for my direction queue
    }
    printf("BAT %d from %s arrives at crossing\n",b->no, num_map[b->dirNO].c_str());
    cross_waiting_bats++;
    pthread_mutex_unlock(&bat_mutex);
}
void cross(struct bat *b) {
    pthread_mutex_lock(&bat_mutex);
    if(dir_queue_size[(b->dirNO + 3) %4] > 0) {
        /**for tracing*/
        //printf("BAT %d from %s is waiting for %s\n",b->no, num_map[b->dirNO].c_str(), num_map[(b->dirNO + 3) %4].c_str());
        /**for tracing*/

        pthread_cond_wait(&dir_First[(b->dirNO + 3) %4],&bat_mutex); // waiting for my right
    }
    cross_waiting_bats--;
    printf("BAT %d from %s crossing\n", b->no, num_map[b->dirNO].c_str());
    sleep(1);
    pthread_mutex_unlock(&bat_mutex);
}
void leave(struct bat *b) {
    pthread_mutex_lock(&bat_mutex);
    //updating counters
    threads_in_system--;
    dir_queue_size[b->dirNO]--;

    printf("BAT %d from %s leaving crossing\n",b->no,num_map[b->dirNO].c_str());

    /**for tracing*/
    //printf("BAT %d from %s signal %s\n",b->no, num_map[b->dirNO].c_str(), num_map[(b->dirNO +1) %4].c_str());
    /**for tracing*/

    /** signal my left if it it waiting for me to avoid starvation */
    pthread_cond_signal(&dir_First[b->dirNO]);

    /**for tracing*/
    //printf("BAT %d from %s signal his direction queue\n",b->no, num_map[b->dirNO].c_str());
    /**for tracing*/

    /** signal the waiting in queue for this direction if it it waiting for me to avoid starvation */
    pthread_cond_signal(&dir_queue[b->dirNO]);

    pthread_mutex_unlock(&bat_mutex);
    pthread_exit(NULL);
}

int main() {
    /**uncomment this to write to an external file*/
    freopen("trace2.txt", "w", stdout);
    string input;
    cin>>input;
    char* sequence = strdup(input.c_str());
    pthread_t batManager;
    pthread_create(&batManager,NULL,batMan, (void*)sequence);
    if(pthread_join(batManager, NULL)) {
        cout<<"unable to join bat manager thread";
    }
    return 0;
}
