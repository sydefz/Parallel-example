#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#define MAXGRID 258   /* maximum grid size */
#define MAXWORKERS 160000  /* maximum number of worker threads */

void *Worker(void *); 
void InitializeGrids();

typedef struct {
	pthread_mutex_t		count_lock;		/* mutex semaphore for the barrier */
	pthread_cond_t		ok_to_proceed;	/* condition variable for leaving */
	int				count;		/* count of the number who have arrived */
} mylib_barrier_t;

void mylib_barrier_init(mylib_barrier_t *b) {
	b -> count = 0;
	pthread_mutex_init(&(b -> count_lock), NULL);
	pthread_cond_init(&(b -> ok_to_proceed), NULL);
}

void mylib_barrier(mylib_barrier_t *b, int num_threads) {	
	/* please implement this function */  
	pthread_mutex_lock(&(b->count_lock));
	b->count=b->count+1;
	if( (b->count) == num_threads)
	{
		(b->count)=0;
		pthread_cond_broadcast(&(b->ok_to_proceed));
	}
	else
	{
		pthread_cond_wait(&(b->ok_to_proceed),&(b->count_lock));
	}
	pthread_mutex_unlock(&(b->count_lock));
}

void mylib_barrier_destroy(mylib_barrier_t *b) {		
	/* please implement this function */  
	pthread_mutex_destroy(&(b -> count_lock));
	pthread_cond_destroy (&(b -> ok_to_proceed));
}

int gridSize, numIters, stripSize, numWorkers, p;
float occupy;

int finishFlag;
int rangeOverFlag[MAXWORKERS][2];
int grid1[MAXGRID][MAXGRID], grid2[MAXGRID][MAXGRID];

/* barrier */
mylib_barrier_t	barrier;

/* main() -- read command line, initialize grids, and create threads
when the threads are done, print the results */

void InitializeGrids() {
  /* initialize the grids (grid1 and grid2) */
     
  int i, j;
  int redc =0;
  int bluec = 0;
  int whitec = 0;
  int redNum = gridSize * gridSize / 3;
  int blueNum = gridSize * gridSize / 3;
  int whiteNum = gridSize * gridSize - redNum - blueNum;
  
  srand((unsigned)time(NULL));

  for (i = 0; i < gridSize; i++){
	  for(j = 0;j < gridSize;j ++){
		  grid1[i][j] = rand() % 3; 
		  grid2[i][j] = 0;
		  if(grid1[i][j] == 0){
			  if(whitec < whiteNum){
				  whitec ++;
			  }
			  else{
				  if(redc < redNum){
					  grid1[i][j] = 1;
					  redc ++;
				  }
				  else{
					  grid1[i][j] = 2;
					  bluec ++;
				  }
			  }
		  }
		  else if(grid1[i][j] == 1){
			  if(redc < redNum){
				  redc ++;
			  }
			  else{
				  if(bluec < blueNum){
					  grid1[i][j] = 2;
					  bluec ++;
				  }
				  else{
					  grid1[i][j] = 0;
					  whitec ++;
				  }
			  }
		  }
		  else{
			  if(bluec < blueNum){
				  bluec ++;
			  }
			  else{
				  if(whitec < whiteNum){
					  grid1[i][j] = 0;
					  whitec ++;
				  }
				  else{
					  grid1[i][j] = 1;
					  redc ++;
				  }
			  }
		  }
	  }
  }
  printf("********************************************");
  printf("\nGenerate random color grid:\n");
  for(i = 0;i < gridSize;i ++){
	  for(j = 0;j < gridSize;j ++){
		  printf("%d ",grid1[i][j]);
	  }
	  printf("\n");
  }
  printf("red has %d, blue has %d\n\n",redc,bluec);
  
}

int main(int argc, char *argv[]) {
	/* thread ids and attributes */
	pthread_t workerid[MAXWORKERS];
	pthread_attr_t attr;
	int red=0;
	int blue=0;
	int i, j, tids[MAXWORKERS];
	FILE *results;

	/* make threads joinable */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	/* initialize barrier */
	mylib_barrier_init(&barrier);

	/* read command line and initialize grids */
	gridSize = atoi(argv[1]);//gridsize
	stripSize = atoi(argv[2]);//tile size
	numIters = atoi(argv[3]);//max
	occupy = atof(argv[4]);//c
	numWorkers = (gridSize/stripSize)*(gridSize/stripSize);
	p = gridSize/stripSize;
	InitializeGrids();

	/* create the workers, then wait for them to finish */
	for (i = 0; i < numWorkers; i++){
		tids[i] = i;
		pthread_create(&workerid[i], &attr, Worker, (void *) &tids[i]);
	}

	for (i = 0; i < numWorkers; i++)
		pthread_join(workerid[i], NULL);

	printf("The tile which meet the requirements:\n");

	/* print the results */
	for (i = 0; i < numWorkers; i++)
	{
		if(rangeOverFlag[i][0]==1)
		{
			printf("The tile %d has more than %f red color\n",i,occupy);
		}
		if(rangeOverFlag[i][1]==1)
		{
			printf("The tile %d has more than %f blue color\n",i,occupy);
		}
		else if (rangeOverFlag[i][0]!=1 && rangeOverFlag[i][1]!=1){
			printf("The tile %d has not enough red/blue color\n",i);
		}
	}

	printf("\nThe grid after movement:\n");

	results = fopen("results.txt", "w");
	

	for (i = 0; i < gridSize; i++) {
		for (j = 0; j < gridSize; j++) {
			fprintf(results, "%d ", grid1[i][j]);
			if(grid1[i][j]==1)
			{
				red++;
			}
			if(grid1[i][j]==2)
			{
				blue++;
			}
			printf("%d ",grid1[i][j]);
		}
		fprintf(results, "\n");
		printf("\n");
	}
	printf("\nResults also stored in results.txt\n");
	printf("********************************************\n");
	/* Clean up and exit */
	pthread_attr_destroy(&attr);
	mylib_barrier_destroy(&barrier); /* destroy barrier object */
	pthread_exit (NULL);
}


/* Each Worker computes values in one strip of the grids.
The main worker loop does two computations to avoid copying from
one grid to the other.  */

void *Worker(void *tid) {
	int *myid;
	int iters;
	int first, last;
	int start, end;
	float ratioRed=0;
	float ratioBlue=0;
	int temp=0;
	int redcount=0, bluecount=0;
	int i, j;
	int flag=0;


	/* determine first and last rows of my strip of the grids */
	myid = (int *) tid;
	first = ((*myid)%p)*stripSize;
	last = first + stripSize - 1;

	for (iters = 1; iters <= numIters; iters++) {

			start = ((*myid)/p)*stripSize;
			end = ((*myid)/p)*stripSize+stripSize-1;

		if(finishFlag)
		{
			pthread_exit (NULL);
		}

		flag=0;
		redcount=0;
		bluecount=0;


		/* red movement */
		/* update my points */
		for (i = start; i <= end; i++) {
			for (j = first; j <= last; j++) {
				if (grid1[i][j] == 1){
					//redcount++;
					if (grid1[i][(j+1)%gridSize] == 0){
						grid2[i][(j+1)%gridSize] = 3;
						grid2[i][j] = 5;
					}
					else{
						grid2[i][j]=1;}
				}
			}
		}

		mylib_barrier(&barrier, numWorkers);

		/* blue movement */
		/* update grid again */
		
		for (i = start; i <= end; i++) {
			for (j = first; j <= last; j++) {
				if (grid1[i][j] == 2){
					//bluecount++;
					if(((grid2[(i+1)%gridSize][j]==0)||(grid2[(i+1)%gridSize][j]==5))
						&&(grid1[(i+1)%gridSize][j]!=2)){
						grid2[(i+1)%gridSize][j] = 4;
						grid2[i][j] = 6;
					}
					else{
						grid2[i][j]=2;
					}
				}
			}
		}
		mylib_barrier(&barrier, numWorkers);

		for (i = start; i <= end; i++) {
			for (j = first; j <= last; j++) {
				if(grid2[i][j]==3)
				{
					temp=1;
				}
				else if(grid2[i][j]==5)
				{
					temp=0;
				}
				else if(grid2[i][j]==4)
				{
					temp=2;
				}
				else if(grid2[i][j]==6)
				{
					temp=0;
				}
				else
				{
					temp=grid2[i][j];
				}
				grid1[i][j]=temp;
				grid2[i][j]=0;
				if(grid1[i][j]==1)
				{
					redcount++;
				}
				if(grid1[i][j]==2)
				{
					bluecount++;
				}
			}
		}
		mylib_barrier(&barrier, numWorkers);

		ratioRed=((float)redcount)/((float)(stripSize*stripSize));
		ratioBlue=((float)bluecount)/((float)(stripSize*stripSize));
		rangeOverFlag[*myid][0]=0;
		rangeOverFlag[*myid][1]=0;
		
		if(ratioRed>occupy){
			rangeOverFlag[*myid][0] = 1;
			flag=1;
		}
		if(ratioBlue>occupy){
			rangeOverFlag[*myid][1] = 1;
			flag=1;
		}
		if(flag>0)
		{
			finishFlag = 1;
		}
		mylib_barrier(&barrier, numWorkers);
	}
	/* thread exit */
	pthread_exit(NULL);
}
