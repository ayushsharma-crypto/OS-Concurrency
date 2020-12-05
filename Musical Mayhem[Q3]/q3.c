#define _POSIX_C_SOURCE 199309L //required for clock

#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<semaphore.h>
#include<unistd.h>
#include<time.h>


// Colors for print statements.
#define A_NRM  "\x1B[1;0m"
#define A_RED  "\x1B[1;31m"
#define A_GRN  "\x1B[1;32m"
#define A_YEL  "\x1B[1;33m"
#define A_BLU  "\x1B[1;34m"
#define A_MAG  "\x1B[1;35m"
#define A_CYN  "\x1B[1;36m"


// Semaphore for number of waiting musicians to collect t-shirts.
// It will get +1 when a musician starts waiting for t-shirt collection.
sem_t tshirt_ready;


// All posssible status of the musician
// already descriptive names so no need to specify.
enum m_status {
    not_yet_arived,
    waiting_to_perform,
    performing_solo,
    performing_not_solo,
    waiting_for_tshirt,
    tshirt_collected,
    exited
};

typedef struct musician{
    int idx; // ID of the musician
    char *name; // Name of musician
    char type;  // Type of musician i.e. p,g,v,b,s
    int arrival_time;   // when musician arrived
    enum m_status stat;   // status of musician
    int stage_type; // stage of musician on which he/she performed.It will be initiated after simulation starts.
    int stage_num;  // stage num allocated to musician for performance.
    int perform_time;   // performance time of the musician on the alloted stage.
    sem_t T_collecting; // semaphore for T-shirt collection. That is a person will wait for this semaphore.
    pthread_t musician_tid; // thread id for muscician thread
    pthread_mutex_t musician_mutex; // mutex variable for accessing same musician mutually exclusively by other threads.
}MU;

// All possible state of a stage
enum s_status{
    empty,  // When no one is performing on the stage.
    mu,     // when only 1 musician (non-singer)performs on the stage.
    sing,   // when only 1 singer perform on the stage.
    both    // when 1 non-singer and 1-singer perform on the stage.
};

typedef struct stage{
    int idx;    // ID of the stage
    int type;   // type of the stage i.e. a or e
    enum s_status status;  // will be 0 if empty,1 if only non-singer,2 if singer and 3 if both
    int musician; // if status!={0,2} then id of musician performer otherwise -1
    int singer; // if status!={0,1} then id of singer performer otherwise -1
    int perform_start_time; // time for performance started by the current Musician.
    pthread_mutex_t stage_mutex;    // Mutex variable for the stage for mutual exclusion by other threads.
}ST;


typedef struct co_ordinator{
    int idx;    // ID of the co-ordinator
    pthread_t co_tid; // Thread ID of that co-ordinator
}CO;

MU artist[1000];
ST stage[1000];
CO co_ordinator[1000];

int artistx,ax,ex,co_ordinatorx,t1,t2,max_wait;

void* musician_thread(void* mu_args);
void* co_ordinator_thread(void* co_args);

int main()
{
    // Taking first line input
    scanf("%d %d %d %d %d %d %d",&artistx,&ax,&ex,&co_ordinatorx,&t1,&t2,&max_wait);

    // Handling few zero cases
    if(artistx==0) { printf("No artist to perform\n"); return -1; }
    else if(ax==0 && ex==0) { printf("No stage to perform, everyone went home\n"); return -1; }
    else if(co_ordinatorx==0) { printf("No co-ordinator to setup stage\n"); return -1; }			// ........... changed from submission
    else if(max_wait==0) { printf("All musician went home\n"); return -1; }
    if(t1==0 && t2==0) { printf("Musician were not happy with time of performance, So went home.\n"); return -1; }

    
    // Initialising artists struct array.
    for(int i=0;i<artistx;i++)
    {
        char* name = (char*)malloc(100);
        char s_type;
        int arrival_time;
        scanf("%s %c %d",name,&s_type,&arrival_time);

        // Data initialisation of the musician
        artist[i].idx=i+1;
        artist[i].name=name;
        artist[i].type=s_type;
        artist[i].arrival_time=arrival_time;
        artist[i].stat=not_yet_arived;

        // Semaphore and mutex initialisation
        pthread_mutex_init(&(artist[i].musician_mutex),NULL);
        sem_init(&(artist[i].T_collecting),0,0);

        // // Artist's stage preference .
        // if(s_type=='v') { artist[i].stage_type='a';}
        // else if(s_type=='b') {artist[i].stage_type='e';}
        // else { artist[i].stage_type='b'; }
    }

    // Initialising co_ordinator struct array.
    for(int i=0;i<co_ordinatorx;i++)
    {
        // Data initialisation of the co-ordinator
        co_ordinator[i].idx=i+1;
    }
    
    // Initialising stage struct array.
    for(int stage_count=0;stage_count<ax+ex;stage_count++)
    {
        // Data initialisation of the acoustic stage
        stage[stage_count].idx=stage_count+1;
        stage[stage_count].status=empty;
        stage[stage_count].musician=-1;
        stage[stage_count].singer=-1;
        stage[stage_count].perform_start_time=-1;

        if(stage_count < ax) { stage[stage_count].type='a'; }
        else { stage[stage_count].type='e'; }
        
        // mutex initialisation
        pthread_mutex_init(&(stage[stage_count].stage_mutex),NULL);
    }

    printf("%s\nSIMULATION BEGIN\n\n%s",A_RED,A_NRM);

    for(int i=0;i<artistx;i++) { pthread_create(&(artist[i].musician_tid),NULL,musician_thread,&artist[i]); }

    for(int i=0;i<co_ordinatorx;i++) { pthread_create(&(co_ordinator[i].co_tid),NULL,co_ordinator_thread,&co_ordinator[i]); }

    for(int i=0;i<artistx;i++) { pthread_join(artist[i].musician_tid,0); }

    printf("\n%sSIMULATION OVER\n\n%s",A_RED,A_NRM);


    for(int i=0;i<ax+ex;i++)
    { 
        pthread_mutex_destroy(&(stage[i].stage_mutex));
    }


    for(int i=0;i<co_ordinatorx;i++)
    {
        pthread_cancel(co_ordinator[i].co_tid); 
    }

    return 0;
}


int random_num(int low,int high)
{
    // Carefull low and high can take only non-negetive integers. And high >= low.
    int increment;
    if(high<=low-1) return 0;
    increment = rand()%(high-low+1);
    return (increment+low);
}


void* musician_thread(void* mu_args)
{
    MU* curr_musician = (MU*) mu_args;
    sleep(curr_musician->arrival_time);


    // updating the status of musician to waiting_to _perform
    curr_musician->stat=waiting_to_perform;
    printf("%s%s %c arrives at Srujana%s\n",A_GRN,curr_musician->name,curr_musician->type,A_NRM);

    // Finding current time of arrival.
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    long double st = ts.tv_nsec/(1e9)+ts.tv_sec;


    int get_stage=0;
    while(!get_stage)
    {

        // Finding current time.
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
        long double en = ts.tv_nsec/(1e9)+ts.tv_sec;

        // handling quit condition of impatience.
        if( max_wait <= (int)(en-st) )
        {
            // Updating status of musician to exited.
            curr_musician->stat=exited;
            printf("%s%s %c left because of impatience%s\n",A_RED,curr_musician->name,curr_musician->type,A_NRM);
            return NULL;
        }

        for(int i=0;i<ax+ex;i++)
        {
            // locking the stage mutex for mutual exclusion s.t. no two musician access it together.
            pthread_mutex_lock(&(stage[i].stage_mutex));

            if( ( curr_musician->type=='v' && stage[i].type=='e' ) || ( curr_musician->type=='b' && stage[i].type=='a' ) )
            {
                pthread_mutex_unlock(&(stage[i].stage_mutex));
                continue;
            }

            if(stage[i].status == empty )
            {
                get_stage=1;
                pthread_mutex_lock(&(curr_musician->musician_mutex));

                // Updating current musician struct data by locking it for Mutual exclusion
                curr_musician->stat=performing_solo;
                curr_musician->stage_type=stage[i].type;
                curr_musician->stage_num=stage[i].idx;
                curr_musician->perform_time=random_num(t1,t2);

                pthread_mutex_unlock(&(curr_musician->musician_mutex));

                // Updating stage struct data
                if(curr_musician->type=='s')
                {
                    stage[i].status=sing;
                    stage[i].singer=curr_musician->idx;
                    stage[i].musician=-1;
                } 
                else
                {
                    stage[i].status=mu;
                    stage[i].singer=-1;
                    stage[i].musician=curr_musician->idx;

                }


                struct timespec RAW_TIME;
                clock_gettime(CLOCK_MONOTONIC_RAW, &RAW_TIME);
                stage[i].perform_start_time=(int)(RAW_TIME.tv_nsec/(1e9)+ RAW_TIME.tv_sec);

                printf("%s%s perorming %c at [%c] stage[%d] for %d sec.%s\n",A_BLU,curr_musician->name,curr_musician->type,curr_musician->stage_type,curr_musician->stage_num,curr_musician->perform_time,A_NRM);

                pthread_mutex_unlock(&(stage[i].stage_mutex));
                break;
            }
            else if(stage[i].status == mu && curr_musician->type=='s')
            {
                get_stage=1;
                int remain;
                // Updating the musician data who was performing while this singer joins him/her.
                pthread_mutex_lock(&(artist[stage[i].musician-1].musician_mutex));


                struct timespec RAW_TIME;
                clock_gettime(CLOCK_MONOTONIC_RAW, &RAW_TIME);
                int now_time=(int)(RAW_TIME.tv_nsec/(1e9)+ RAW_TIME.tv_sec);

                int used_time = now_time - stage[i].perform_start_time;

                remain =  artist[stage[i].musician-1].perform_time - used_time;     
                artist[stage[i].musician-1].perform_time += 2;
                artist[stage[i].musician-1].stat=performing_not_solo;


                pthread_mutex_lock(&(curr_musician->musician_mutex));

                // Updating current musician struct data by locking it for Mutual exclusion
                curr_musician->stat=performing_not_solo;
                curr_musician->stage_type=stage[i].type;
                curr_musician->stage_num=stage[i].idx;
                curr_musician->perform_time=remain + 2;
    
                pthread_mutex_unlock(&(curr_musician->musician_mutex));
                
                pthread_mutex_unlock(&(artist[stage[i].musician-1].musician_mutex));

                // Updating stage struct data
                stage[i].status=both;
                stage[i].singer=curr_musician->idx;

                printf("%sSinger %s joined the Musician %s performance%s\n",A_MAG,curr_musician->name,artist[stage[i].musician-1].name,A_NRM);

                pthread_mutex_unlock(&(stage[i].stage_mutex));
                break;
            }

            pthread_mutex_unlock(&(stage[i].stage_mutex));
        }
    }


    // sleep for the performance time
    // printf("%s will perform for %d seconds of time\n",curr_musician->name,curr_musician->perform_time);
    sleep(curr_musician->perform_time);

    if((curr_musician->stat == performing_not_solo) && (curr_musician->type!='s') )
    {
        sleep(2);
    }


    printf("%s%s perormance %c at [%c] stage[%d] finished.%s\n",A_BLU,curr_musician->name,curr_musician->type,curr_musician->stage_type,curr_musician->stage_num,A_NRM);


    if(stage[curr_musician->stage_num-1].status!=empty)
    {
        stage[curr_musician->stage_num-1].perform_start_time=-1;
        stage[curr_musician->stage_num-1].status=empty;
        stage[curr_musician->stage_num-1].singer=-1;
        stage[curr_musician->stage_num-1].musician=-1;
    }

    // if(curr_musician->stat==performing_solo)
    // {
    // }
    // else if(curr_musician->stat == performing_not_solo)
    // {
    //     curr_musician->stat=waiting_for_tshirt;
    //     if(curr_musician->type=='s')
    //         artist[stage[curr_musician->stage_num-1].musician].stat=waiting_for_tshirt;
    //     else
    //         artist[stage[curr_musician->stage_num-1].singer].stat=waiting_for_tshirt;
    // }
    
    curr_musician->stat=waiting_for_tshirt;

    if(co_ordinatorx==0)
    {
        curr_musician->stat=exited;
        printf("%s%s %c returned home :(, No T-shirts %s\n",A_RED,curr_musician->name,curr_musician->type,A_NRM);
        return NULL;
    }

    sem_post(&(tshirt_ready));

    printf("%s%s collecting t-shirt%s\n",A_YEL,curr_musician->name,A_NRM);

    sem_wait(&(curr_musician->T_collecting));
    curr_musician->stat=exited;
    printf("%s%s %c returned home happily :)...Yayy!!%s\n",A_GRN,curr_musician->name,curr_musician->type,A_NRM);

    return NULL;

}

void* co_ordinator_thread(void* co_args)
{
    CO* curr_co_ordinator = (CO*) co_args;
    while(1)
    {
        sem_wait(&tshirt_ready);
        for(int i=0;i<artistx;i++)
        {
            pthread_mutex_lock(&(artist[i].musician_mutex));

            if(artist[i].stat==waiting_for_tshirt)
            {
                sleep(2);
                artist[i].stat=tshirt_collected;
                printf("%s%s %c collected t-shirt from co-ordinator %d%s\n",A_CYN,artist[i].name,artist[i].type,curr_co_ordinator->idx,A_NRM);
                sem_post(&(artist[i].T_collecting));
                pthread_mutex_unlock(&(artist[i].musician_mutex));
                break;
            }

            pthread_mutex_unlock(&(artist[i].musician_mutex));
        }
    }
    return NULL;
}
