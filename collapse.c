#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE 64

int main(int argc, char **argv) {

	char * buf= (char *)calloc(BUF_SIZE,sizeof(char));
	char * bufNew= (char *)calloc(BUF_SIZE,sizeof(char));
	char * bufRev = (char *)calloc(BUF_SIZE,sizeof(char));

	char acsii=0;
	int position=0; //store content into bufNew
	int myid, numprocs, source ,destination;
	int processID=0;

	
	int i=0, k=0, t=0;

	int content=0;

	int jobStack=0;
	
	int* num = (int *)calloc(BUF_SIZE,sizeof(int));
	int number;
	int sum=0;
	char subCollapse;

	MPI_Status status;
	MPI_Init(&argc,&argv); 
	MPI_Comm_size(MPI_COMM_WORLD,&numprocs); 
	MPI_Comm_rank(MPI_COMM_WORLD,&myid);	

	if (numprocs == 1) {
		/* For single processer */
        FILE *fp = fopen(argv[argc-1],"r");
            while(!feof(fp)){
            fread(&acsii,1,1,fp);
            if(acsii>=48&&acsii<=57)
                sum=sum+(acsii-48);
            if(sum>9)
                sum=sum/10+sum%10;
         }
         printf("The result by single processer is %d\n",sum);
	} else if (myid == 0) {
		/*************************************** Master Process ***************************************/
		/* read a block of chars to buffer */
		FILE *fp = fopen(argv[argc-1],"r");

		/* start allocation */
		jobStack = 0;
		for(i = 0; i < numprocs-1; i++) {
			content = buf_read(buf, buf + BUF_SIZE, fp);
			if (content){
				/* allocate a job to proc i */
				jobStack++;
				if(content==BUF_SIZE){
					MPI_Send(buf, BUF_SIZE, MPI_CHAR, i+1 , 1, MPI_COMM_WORLD);
					content=0;
				}
				else
				{
					buf[content]='\t';
					MPI_Send(buf, BUF_SIZE, MPI_CHAR, i+1 , 1, MPI_COMM_WORLD);
					content=0;
				}
				/* Stop reading if not long enough */
			}
			else{
				/* send a zero length array to terminate the worker */
				buf[content]='\t';
				MPI_Send(buf, BUF_SIZE, MPI_CHAR, i+1 , 1, MPI_COMM_WORLD);
			}
		}

		/* if job been allocated or received */
		while(jobStack) {
			/*since sending jobs, we book a blocking socket and wait for return*/
			MPI_Recv(&bufNew[position], 1, MPI_CHAR, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
			position++;
			/* status.MPI_SOURCE contains the sending process ID. We obtain the value received,
			and add them to the result */
			processID=status.MPI_SOURCE;
			jobStack--;
			//minius jobStack if been processed

			if(position==BUF_SIZE){
				MPI_Send(bufNew, BUF_SIZE, MPI_CHAR, processID , 1, MPI_COMM_WORLD);
				jobStack++;
				position=0;
			}
			else{
				content = buf_read(buf, buf + BUF_SIZE, fp);
				if (content){
					if(content==BUF_SIZE){
						content=0;
						MPI_Send(buf, BUF_SIZE, MPI_CHAR, processID , 1, MPI_COMM_WORLD);
					}
					else
					{
						buf[content]='\t';
						content=0;
						MPI_Send(buf, BUF_SIZE, MPI_CHAR, processID , 1, MPI_COMM_WORLD);

					}
					jobStack++;
					/* Stop reading if not long enough */
				}
				else{
					/* send empty array to terminate the worker processer*/
					buf[content]='\t';
					MPI_Send(buf, BUF_SIZE, MPI_CHAR, processID, 1, MPI_COMM_WORLD);
				}
			}
		}
		if(position>=1)
		{
			bufNew[position]='\t';
			MPI_Send(bufNew, BUF_SIZE, MPI_CHAR, 1 , 1, MPI_COMM_WORLD);
		}
	}
	else {
		/*************************************** Worker processes ***************************************/
			while(1){
				MPI_Recv(bufRev, BUF_SIZE, MPI_CHAR, 0, 1, MPI_COMM_WORLD, &status);
				for (k=0; k<BUF_SIZE;k++)
				{
					if (bufRev[k]=='\t')
					{
						number=k;
						num[k]=0;
						break;
					}
					else
					{
						num[k]=bufRev[k]-48;
						number=BUF_SIZE;
					}
				}
				sum=0;
				sum=sumUp(num,number);
				if(number==0)
				{
					break;
				}
				subCollapse= (char)(sum+48);
				MPI_Send(&subCollapse, 1, MPI_CHAR, 0 , 1, MPI_COMM_WORLD);
			}

			/* The last worker sum up and send back to master*/
			if(myid==1)
			{
				MPI_Recv(bufRev, BUF_SIZE, MPI_CHAR, 0, 1, MPI_COMM_WORLD, &status);
				for (k=0; k<BUF_SIZE;k++)
				{
					if (bufRev[k]=='\t')
					{
						number=k;
						num[k]=0;
						break;
					}
					else
					{
						num[k]=bufRev[k]-48;
						number=BUF_SIZE;
					}
				}
				sum=0;
				sum=sumUp(num,number);
				subCollapse= (char)(sum+48);
				MPI_Send(&subCollapse, 1, MPI_CHAR, 0 , 1, MPI_COMM_WORLD);
			}
	}

	if (myid == 0 &&numprocs!=1) {
		
		MPI_Recv(&bufNew[position], 1, MPI_CHAR,1, 1, MPI_COMM_WORLD, &status);
		printf("Collapse result by parallel computing is %c\n",bufNew[position]);
	}
	MPI_Finalize();
	//finalize mpi
	return 0;
}

	/*************************************** Collapse algrithem ***************************************/
int sumUp(int* num,int sum)
{
	int k = 0;
	int subTotal=0;
	
	if(sum==0)
	    subTotal = 0;

	else if(sum==1)
		subTotal = num[0];

	else{
		for(k = 0; k < (sum-1); k++){
			num[k+1] = num[k] + num[k+1];
			if (num[k+1]>9)
				num[k+1] = num[k+1]/10 + num[k+1]%10;
			subTotal = num[k+1];
		}
	}
	return subTotal;
}

int buf_read(char * buf, const char * end, FILE * fp) {
	int count;
	count = fread(buf, 1, end - buf, fp);
	if (count > 0 && buf[count - 1] == '\n') {
		--count;
	}
	return count;
}
