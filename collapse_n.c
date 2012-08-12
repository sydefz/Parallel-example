#include <mpi.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#define BUF_SIZE 64

int sumUp(int* num,int sum)
{
	int k = 0;
	int subTotal = 0;
	
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


int main(int argc, char **argv) 
{
	int rank, numprocs, i, k;
	int sumTotal = 0;
	int number = 0;
	int flag = 0;
	int read = 0;
	int sum = 0;

	

	char * bufNumber= (char *)calloc(BUF_SIZE,sizeof(char));
	char * bufRev = (char *)calloc(BUF_SIZE,sizeof(char));
	int* signal = (int *)malloc(2*sizeof(int));
	int* num = (int *)calloc(BUF_SIZE,sizeof(int));
	MPI_Init( &argc, &argv );
	MPI_Comm_rank( MPI_COMM_WORLD, &rank );
	MPI_Comm_size( MPI_COMM_WORLD, &numprocs );

	if (rank == 0) { 
		MPI_Request* requests = (MPI_Request*)malloc((numprocs-1)*sizeof(MPI_Request));
		MPI_Status* statuses = (MPI_Status*)malloc(sizeof(MPI_Status)*(numprocs-1));
		int* indices = (int *)malloc((numprocs-1)*sizeof(int));
		int* buf = (int *)malloc((numprocs-1)*sizeof(int)*2);
		int j, ndone, count, testNum;
		int w_src;
		int samllSum=0;
		srand((unsigned)time(NULL));
		if (numprocs == 1){
			flag =  1+rand() % BUF_SIZE;
			read = buf_read(bufNumber, bufNumber + flag);
			while(read){
				if(read!=flag){
					bufNumber[read]='\0';
				}
				for (k=0; k<flag;k++)
				{
					if (bufNumber[k]=='\0')
					{
						number=k;
						num[k]=0;
						break;
					}
					else
					{
						num[k]=bufNumber[k]-48;
						number=flag;
					}
				}
				sum=0;
				sum=sumUp(num,number);
				samllSum = samllSum + sum;
				if(samllSum>9)
				{
					samllSum = samllSum/10 + samllSum%10; 
				}
				read=0;
				read = buf_read(bufNumber, bufNumber + flag);
			}
			printf("Only one process, the result is %d\n",samllSum);
		}
		else{
			/* prepare to receive (non-blocking) messages from all workers */
			count = numprocs-1;
			for (i=0; i<numprocs-1; i++)
			{
				MPI_Irecv(buf+2*i, 2, MPI_INT, i+1, 2, MPI_COMM_WORLD, &requests[i]);
			}
			/* receive and check conditions */
			while(count > 0) {
				MPI_Waitsome(numprocs-1, requests, &ndone, indices, statuses);

				count = count-ndone;
				//printf("ndone = %d\n", ndone);
				for (i=0; i<ndone; i++) {
					j = indices[i];
					w_src = statuses[i].MPI_SOURCE;
					sumTotal=sumTotal+buf[2*j+1];
					sumTotal=sumTotal/10+sumTotal%10;
					sumTotal=sumTotal/10+sumTotal%10;
					read = buf_read(bufNumber, bufNumber + buf[2*j]);
					//printf("read is %d\n",read);
					//printf("process is %d, flag is %d, result is %d\n",w_src,buf[2*j],buf[2*j+1]);
					if (read){
						count++;
						if(read==buf[2*j]){						
							MPI_Send(bufNumber, buf[2*j], MPI_CHAR, w_src , 0, MPI_COMM_WORLD);
							read=0;
						}
						else
						{
							bufNumber[read]='\0';
							MPI_Send(bufNumber, buf[2*j], MPI_CHAR, w_src , 0, MPI_COMM_WORLD);
							read=0;
						}
						/* read a block, but if the last read was a short item count, don't try to read again */
					}
					else{
						/* send a zero length array to terminate the worker */
						bufNumber[read]='\0';
						MPI_Send(bufNumber, buf[2*j], MPI_CHAR, w_src , 0, MPI_COMM_WORLD);
					}
					MPI_Irecv(buf+2*j, 2, MPI_INT, w_src, 2, MPI_COMM_WORLD, &requests[j]);
				}
				//printf("count = %d\n", count);
			}
			printf("By multiple processors, the result is %d\n",sumTotal);
		}
	}
	else {
		MPI_Status stat;
		sum=0;	
		do{
			flag =  1+rand() % BUF_SIZE;
			//printf("rank is %d\n",rank);
			//printf("flag is %d\n",flag);
			//printf("sum is %d\n",sum);
			signal[0] = flag;
			signal[1] = sum;
			MPI_Send(signal, 2, MPI_INT, 0, 2, MPI_COMM_WORLD);
			//printf("This line~!Y\n");
			MPI_Recv(bufRev, signal[0], MPI_CHAR, 0, 0, MPI_COMM_WORLD, &stat);
			for (k=0; k<signal[0];k++)
			{
				if (bufRev[k]=='\0')
				{
					//printf("This line~!X\n");
					number=k;
					num[k]=0;
					break;
				}
				else
				{
					num[k]=bufRev[k]-48;
					number=signal[0];
				}
			}
			/*for(k=0;k<signal[0];k++)
			{
			printf("%d",num[k]);
			}
			printf("tested\n");*/
			sum=0;
			sum=sumUp(num,number);

		}while(number != 0);
	}
	MPI_Finalize();
	return 0;
}


int buf_read(char * buf, const char * end) {
	int count;
	count = fread(buf, 1, end - buf, stdin);
	if (count > 0 && buf[count - 1] == '\n') {
		--count;
	}
	return count;
}