#define _POSIX_C_SOURCE 199309L //required for clock

#include<stdio.h>               // General requirement for I/O function
#include<sys/shm.h>             // For shared memory used function : shmget,shmat
#include<pthread.h>             // For using thread functions.
#include<stdlib.h>              // Standard linrary for malloc.
#include<time.h>                // For time functions.
#include<unistd.h>
#include<wait.h>

#define LLI long long int

struct array{
    LLI L_INDEX,R_INDEX;
    LLI *ARRAY;
};

LLI* get_shared_mem(int arr_size)
{
    // This function returns a pointer to (type casted to int) shared memory
    // which gets share between the childs of a process or between threads.
    // shmtget is used to allocate the memory segment.
    // shmat is used to attach that shared segment to the return int pointer.
    // Errors have been handled.

    key_t share_mem_key = IPC_PRIVATE;

    LLI shared_mem_id = shmget(share_mem_key,8*arr_size,IPC_CREAT | 0666);
    if(shared_mem_id == -1)
    {
        perror("SHMGET");
        return NULL;
    }

    LLI *shared_mem_at = shmat(shared_mem_id,NULL,0);
    if(shared_mem_at == (LLI *)-1)
    {
        perror("SHMAT");
        return NULL;
    }
    
    return shared_mem_at;
}
void print_arr(LLI arr_sz,LLI* arr)
{
    for(LLI i=0;i<arr_sz;i++)
    {
        printf("%lld ",arr[i]);
    }
    printf("\n");
}

void selection_sort(LLI* s_arr,LLI low,LLI high);
void* multiprocess_mergesort(LLI* ARR,LLI low,LLI high);
void* threaded_mergesort(void* T_ARR);
void* normal_mergesort(LLI * BRR,LLI low, LLI high);

int main()
{
    // Getting size of the array to apply merge sort.
    LLI arr_size;
    printf("\tGive size of array : ");
    scanf("%lld",&arr_size);

    if(arr_size<=0)
    {
        printf("\tArray Size not valid\n");
        return 0;
    }

    // Getting shared memory for processses implementation
    // of merge sort. This array ARR will get used in multi-process
    // mergesort.
    LLI *ARR = get_shared_mem(arr_size);
    if(!ARR)
    {
        printf("\tReturning...\n");
        return -1;
    }

    // BRR is just a copy of ARR array but it is not shared memory.
    // BRR will be used for normal-mergesort
    LLI BRR[arr_size];

    // Initialising struct T_ARR which will be used for 
    // multi-threaded mergesort.
    struct array T_ARR;
    T_ARR.L_INDEX=0;
    T_ARR.R_INDEX=arr_size-1;
    T_ARR.ARRAY=(LLI*)malloc(sizeof(LLI)*arr_size);


    // Initialising the array ARR via INPUT.
    printf("\tEnter %lld integers with spaces in between : ",arr_size);
    for(LLI i=0;i<arr_size;i++)
    {
        scanf("%lld",&ARR[i]);
        BRR[i]=ARR[i];
        T_ARR.ARRAY[i]=ARR[i];
    }


    // Multiprocess Mergesort Function call and it's performance duration.
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    long double st = ts.tv_nsec/(1e9)+ts.tv_sec;

    printf("\n\tStarting multiprocess mergesort\n");
    printf("\t-------------------------------\n");

    multiprocess_mergesort(ARR,0,arr_size-1);
    printf("\tFinal Array = ");

    print_arr(arr_size,ARR);

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    long double en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("\tMultiprocess-Mergesort time = %Lf\n", en - st);
    long double t3 = en-st;
    shmdt(ARR);


    // Multi-Threaded Mergesort Function call and it's performance duration.
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec/(1e9)+ts.tv_sec;

    printf("\n\tStarting multithreaded mergesort\n");
    printf("\t--------------------------------\n");

    pthread_t tid;
    pthread_create(&tid,NULL,threaded_mergesort,&T_ARR);
    pthread_join(tid,NULL);
    printf("\tFinal Array = ");

    print_arr(arr_size,T_ARR.ARRAY);

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("\tMultithreaded-Mergesort time = %Lf\n", en - st);
    long double t2 = en-st;


    // Normal Mergesort Function call and it's performance duration.
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec/(1e9)+ts.tv_sec;

    printf("\n\tStarting Normal mergesort\n");
    printf("\t-------------------------\n");

    normal_mergesort(BRR,0,arr_size-1);
    printf("\tFinal Array = ");

    print_arr(arr_size,BRR);

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("\tNormal Mergesort time = %Lf\n", en - st);
    long double t1 = en-st;
    printf("\n");


    // Comaprison of above three method o merge-sort algo.
    printf("\tNormal mergesort is %Lf times faster than Threaded mergesort\n",t2/t1);
    printf("\tNormal mergesort is %Lf times faster than Multi-Process mergesort\n",t3/t1);
    printf("\tThreaded mergesort is %Lf times faster than Multi-Process mergesort\n",t3/t2);
    return 0;
}

// Implementation of selection sort
void selection_sort(LLI* s_arr,LLI low,LLI high)
{
    for(int i=low;i<high;i++)
    {
        LLI min=i;
        for(int j=i+1;j<=high;j++)
            if(s_arr[min]>s_arr[j])
                min=j;
        LLI temp=s_arr[i];
        s_arr[i]=s_arr[min];
        s_arr[min]=temp;
    }
}

// Implmentation of normal merge-sort
void merge(LLI* BRR,LLI low,LLI mid,LLI high)
{
    LLI l1=mid-low+1,l2=high-mid;
    LLI templ[l1],tempr[l2];
    for(int i=low;i<=mid;i++) templ[i-low]=BRR[i];
    for(int i=mid+1;i<=high;i++) tempr[i-mid-1]=BRR[i];
 
    LLI i=0,j=0,k=low;
    while((i<l1) && (j<l2))
    {
        if(templ[i]<tempr[j]) BRR[k++]=templ[i++];
        else BRR[k++]=tempr[j++]; 
    }

    while (i<l1) BRR[k++]=templ[i++];
    while (j<l2) BRR[k++]=tempr[j++];
}

void* normal_mergesort(LLI * BRR,LLI low, LLI high)
{
    // if(high-low+1<=0) return NULL;

    if(high-low+1<5)
    {
        selection_sort(BRR,low,high);
        return NULL;
    }

    LLI mid = low + (high-low)/2;
    normal_mergesort(BRR,low,mid);
    normal_mergesort(BRR,mid+1,high);
    merge(BRR,low,mid,high);
    return NULL;
}

// Implmentation of multi-process merge-sort
void* multiprocess_mergesort(LLI* ARR,LLI low,LLI high)
{
    // if(high-low<=0) return NULL;

    if(high-low+1<5)
    {
        selection_sort(ARR,low,high);
        return NULL;
    }

    LLI mid = low + (high-low)/2;
    LLI pid1=fork();
    if(pid1 < 0) 
    {
        perror("FORK1");
        return NULL;
    }
    else if(pid1==0)
    {
        multiprocess_mergesort(ARR,low,mid);
        _exit(1);
    }
    else
    {
        LLI pid2=fork();
        if(pid2 < 0)
        {
            perror("FORK2");
            return NULL;
        }
        else if (pid2==0)
        {
            multiprocess_mergesort(ARR,mid+1,high);
            _exit(1);
        }
        else
        {
            int status;
            waitpid(pid1,&status,0);
            waitpid(pid2,&status,0);
            merge(ARR,low,mid,high);
        }
    }
    return NULL;
}

// Implmentation of multi-threaded merge-sort
void* threaded_mergesort(void* T_ARR)
{
    // Extracting the struct out of parameter T_ARR
    struct array * T_arr = (struct array*) T_ARR;
    
    // Extracting values of Array to be sorted.
    LLI l=T_arr->L_INDEX;
    LLI r=T_arr->R_INDEX;
    LLI *arr=T_arr->ARRAY;

    // Checking for single item array.
    // if(r-l <=0) return NULL;

    if(r-l+1<5)
    {
        selection_sort(arr,l,r);
        return NULL;
    }

    // Finding middle index from where to divide the array.
    LLI mid = l + (r-l)/2;

    // Creating struct for left half of the provided array
    struct array L_arr;
    L_arr.L_INDEX=l;
    L_arr.R_INDEX=mid;
    L_arr.ARRAY=arr;

    // Creating struct for right half of the provided array
    struct array R_arr;
    R_arr.L_INDEX=mid+1;
    R_arr.R_INDEX=r;
    R_arr.ARRAY=arr;

    // Creating threads for both half.
    pthread_t tid1,tid2;
    pthread_create(&tid1,NULL,threaded_mergesort,&L_arr);
    pthread_create(&tid2,NULL,threaded_mergesort,&R_arr);
    pthread_join(tid1,NULL);
    pthread_join(tid2,NULL);

    // merge the two half.
    merge(arr,l,mid,r);
}