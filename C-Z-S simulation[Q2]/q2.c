#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<time.h>

#define MAX_COMPANY 1000
#define MAX_ZONE 1000
#define MAX_STUDENT 1000


#define A_NRM  "\x1B[1;0m"
#define A_RED  "\x1B[1;31m"
#define A_GRN  "\x1B[1;32m"
#define A_YEL  "\x1B[1;33m"
#define A_BLU  "\x1B[1;34m"
#define A_MAG  "\x1B[1;35m"
#define A_CYN  "\x1B[1;36m"

struct Company{
    int idx;
    int batch_left;
    int sent_batch_left;
    int batch_capacity;
    float batch_prob;
    pthread_t company_tid;
    pthread_mutex_t company_mutex;
    pthread_cond_t cv_zone;
}company[MAX_COMPANY];

struct Vzone{
    int idx;
    int curr_company;
    float curr_batch_prob;
    int curr_capacity;
    int curr_occupied;
    int curr_slot;
    pthread_t zone_tid;
    pthread_mutex_t zone_mutex;
    pthread_cond_t cv_student;
}vzone[MAX_ZONE];

struct Student{
    int idx;
    pthread_t student_tid;
}student[MAX_STUDENT];

void *company_thread(void* company_arg);
void *vzone_thread(void* vzone_arg);
void *student_thread(void* student_arg);

int companyx,vzonex,studentx,total_current_student;
pthread_mutex_t change_student,S_W_T;
int student_went_home;

int main()
{
    srand(time(0));
    scanf("%d %d %d",&companyx,&vzonex,&studentx);

    if(companyx==0)
    {
        printf("NO COMPANY ,SO NO VACCINES\n");
        return 0;
    }
    else if (studentx==0)
    {
        printf("NO STUDENT ,SO NOT SLOTS\n");
        return 0;
    }
    else if (vzonex==0)
    {
        printf("NO VACCINATION ZONES \n");
        return 0;
    }
    
    


    total_current_student=0;
    student_went_home=0;
    
    for(int i=0;i<companyx;i++)
    {
        float prob;
        scanf("%f",&prob);
        company[i].idx=i+1;
        company[i].batch_prob=prob;
        pthread_mutex_init(&(company[i].company_mutex),NULL);
    }


    for(int i=0;i<vzonex;i++) 
    { 
        vzone[i].idx=i+1;
        pthread_mutex_init(&(vzone[i].zone_mutex),NULL);
    }

    for(int i=0;i<studentx;i++) { student[i].idx=i+1; };

    printf("%sSIMULATION BEGIN\n\n%s",A_RED,A_NRM);

    for(int i=0;i<companyx;i++) { pthread_create(&(company[i].company_tid),NULL,company_thread,&company[i]); }
    for(int i=0;i<studentx;i++) { pthread_create(&(student[i].student_tid),NULL,student_thread,&student[i]); }
    for(int i=0;i<vzonex;i++) { pthread_create(&(vzone[i].zone_tid),NULL,vzone_thread,&vzone[i]); }

    for(int i=0;i<studentx;i++) { pthread_join(student[i].student_tid,0); }

    printf("\n%sSIMULATION OVER\n%s",A_RED,A_NRM);


    for(int i=0;i<companyx;i++) 
    { 
        pthread_mutex_destroy(&(company[i].company_mutex)); 
        pthread_cancel(company[i].company_tid);
    }
    for(int i=0;i<vzonex;i++) 
    { 
        pthread_mutex_destroy(&(vzone[i].zone_mutex)); 
        pthread_cancel(vzone[i].zone_tid);
    }
    
    return 0;
}

int random_num(int low,int high)
{
    // Carefull low and high can take only non-negetive integers. And high >= low.
    int increment;
    if(high<=low-1) return 0;
    increment = rand()%(high-low+1);
    return increment+low;
}

void *company_thread(void* company_arg)
{
    struct Company *curr_company = (struct Company*)company_arg;
    int flag=1;
    while(flag)
    {
        if(student_went_home==studentx)  {return NULL;}

        int r = random_num(1,5);   // Number of batches to prepare.
        int w = random_num(2,5);   // Time taken to prepare all 'r' the batches.
        int p = random_num(10,20); // Number of vaccine in each batch.

        printf("%sPharmaceutical Company %d is preparing %d batches of vaccines each with capacity %d, which have success probability %f%s\n",A_GRN,curr_company->idx,r,p,curr_company->batch_prob,A_NRM);
        sleep(w);
        pthread_mutex_lock(&(curr_company->company_mutex));
        curr_company->batch_left=r;
        curr_company->batch_capacity=p;
        curr_company->sent_batch_left=0;
        printf("%sPharmaceutical Company %d has prepared %d batches of vaccines each with capacity %d, which have success probability %f%s\n",A_BLU,curr_company->idx,r,p,curr_company->batch_prob,A_NRM);
        
        while((curr_company->batch_left) + (curr_company->sent_batch_left))
        {
            pthread_cond_wait(&(curr_company->cv_zone),&(curr_company->company_mutex));
        }
        printf("%sAll the vaccines prepared by pharmaceutical company %d are emptied. Resuming production now\n%s",A_GRN,curr_company->idx,A_NRM);
        pthread_mutex_unlock(&(curr_company->company_mutex));
    }
    return NULL;
}


void *vzone_thread(void* vzone_arg)
{
    struct Vzone *curr_vzone = (struct Vzone*)vzone_arg;
    int work_always=1;
    while(work_always)
    {
        int got_batch=0;
        for(int i=0;i<companyx;i++)
        {
            pthread_mutex_lock(&(company[i].company_mutex));
            if( 0 < company[i].batch_left )
            {
                got_batch=1;
                curr_vzone->curr_company=company[i].idx;
                curr_vzone->curr_batch_prob=company[i].batch_prob;
                curr_vzone->curr_capacity=company[i].batch_capacity;
                company[i].batch_left--;
                company[i].sent_batch_left++;
                printf("%sPharmaceutical Company %d has delivered vaccines to Vaccination zone %d, resuming vaccinations now\n%s",A_YEL,company[i].idx,curr_vzone->idx,A_NRM);
                pthread_cond_signal(&(company[i].cv_zone));
                pthread_mutex_unlock(&(company[i].company_mutex));
                break;
            }
            pthread_cond_signal(&(company[i].cv_zone));
            pthread_mutex_unlock(&(company[i].company_mutex));
        }

        while(got_batch)
        {
            pthread_mutex_lock(&(curr_vzone->zone_mutex));
            if(curr_vzone->curr_capacity == 0)
            {
                if(student_went_home==studentx)  {return NULL;}

                printf("%sVaccination Zone %d has run out of vaccines\n%s",A_MAG,curr_vzone->idx,A_NRM);
                company[(curr_vzone->curr_company)-1].sent_batch_left--;
                pthread_cond_signal(&(company[(curr_vzone->curr_company)-1].cv_zone));
                pthread_mutex_unlock(&(curr_vzone->zone_mutex));
                break;
            }

            int max_slot = 8;
            if(curr_vzone->curr_capacity < max_slot) { max_slot=curr_vzone->curr_capacity; };
            if(total_current_student < max_slot) { max_slot=total_current_student; };

            curr_vzone->curr_occupied=0;
            curr_vzone->curr_slot = random_num(1,max_slot);
            curr_vzone->curr_capacity -= curr_vzone->curr_slot;

            if(curr_vzone->curr_slot!=0)
            {                    
                printf("%sVaccination Zone %d is ready to vaccinate with %d slots%s\n",A_YEL,curr_vzone->idx,curr_vzone->curr_slot,A_NRM);
                printf("%sVaccination Zone %d entering Vaccination Phase%s\n",A_YEL,curr_vzone->idx,A_NRM);
            }

            while(curr_vzone->curr_slot != curr_vzone->curr_occupied)
            {
                pthread_cond_wait(&(curr_vzone->cv_student),&(curr_vzone->zone_mutex));
            }

            pthread_mutex_unlock(&(curr_vzone->zone_mutex));
        }
    }
    return NULL;
}

int probability(float prob)
{
    float ran_num = (float)(rand()%101)/100;
    if((ran_num <= prob) && (prob!=0)) return 0;    //Student vaccinated successfully.
    else return 1;  //Student vaccination failed.
}

void *student_thread(void* student_arg)
{
    struct Student *curr_student = (struct Student*)student_arg;
    int arrival_time = random_num(0,15);
    sleep(arrival_time);
    int entry_type =0;

    while(entry_type < 3)
    {
        pthread_mutex_lock(&change_student);
        total_current_student++;
        entry_type++;
        printf("%sStudent %d has arrived for his round %d of Vaccination%s\n",A_MAG,curr_student->idx,entry_type,A_NRM);
        printf("%sStudent %d is waiting to be allocated a slot on a Vaccination Zone\n%s",A_MAG,curr_student->idx,A_NRM);
        pthread_mutex_unlock(&change_student);

        int got_slot=0,V_zone;
        float succes_prob;
        while(!got_slot)
        {
            for(int i=0;i<vzonex;i++)
            {
                pthread_mutex_lock(&(vzone[i].zone_mutex));
                if(vzone[i].curr_slot > vzone[i].curr_occupied)
                {
                    vzone[i].curr_occupied++;
                    got_slot=1;
                    V_zone=vzone[i].idx;
                    succes_prob=vzone[i].curr_batch_prob;

                    pthread_mutex_lock(&change_student);
                    total_current_student--;
                    pthread_mutex_unlock(&change_student);

                    printf("%sStudent %d assigned a slot on the Vaccination Zone %d and waiting to be vaccinated\n%s",A_CYN,curr_student->idx,vzone[i].idx,A_NRM);
                    printf("%sStudent %d on Vaccination Zone %d is getting vaccinated which has success probability %f\n%s",A_CYN,curr_student->idx,vzone[i].idx,vzone[i].curr_batch_prob,A_NRM);
                    sleep(2);
                    printf("%sStudent %d on Vaccination Zone %d has been vaccinated which has success probability %f\n%s",A_CYN,curr_student->idx,V_zone,succes_prob,A_NRM);

                    if(probability(succes_prob))
                    {
                        printf("%sStudent %d has tested negetive for antibodies\n%s",A_RED,curr_student->idx,A_NRM);
                    }
                    else
                    {
                        printf("%sStudent %d has tested positive for antibodies\n%s",A_GRN,curr_student->idx,A_NRM);
                        entry_type=4;
                    }



                    pthread_cond_signal(&(vzone[i].cv_student));
                    pthread_mutex_unlock(&(vzone[i].zone_mutex));
                    break;
                }

                pthread_cond_signal(&(vzone[i].cv_student));
                pthread_mutex_unlock(&(vzone[i].zone_mutex));
            }
        }
        
    }
    pthread_mutex_lock(&S_W_T);
    student_went_home++;
    pthread_mutex_unlock(&S_W_T);
    return NULL;
}