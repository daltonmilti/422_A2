/*
 *  pcmatrix module
 *  Primary module providing control flow for the pcMatrix program
 *
 *  Producer consumer bounded buffer program to produce random matrices in parallel
 *  and consume them while searching for valid pairs for matrix multiplication.
 *  Matrix multiplication requires the first matrix column count equal the
 *  second matrix row count.
 *
 *  A matrix is consumed from the bounded buffer.  Then matrices are consumed
 *  from the bounded buffer, ONE AT A TIME, until an eligible matrix for multiplication
 *  is found.
 *
 *  Totals are tracked using the ProdConsStats Struct for each thread separately:
 *  - the total number of matrices multiplied (multtotal from each consumer thread)
 *  - the total number of matrices produced (matrixtotal from each producer thread)
 *  - the total number of matrices consumed (matrixtotal from each consumer thread)
 *  - the sum of all elements of all matrices produced and consumed (sumtotal from each producer and consumer thread)
 *  
 *  Then, these values from each thread are aggregated in main thread for output
 *
 *  Correct programs will produce and consume the same number of matrices, and
 *  report the same sum for all matrix elements produced and consumed.
 *
 *  Each thread produces a total sum of the value of
 *  randomly generated elements.  Producer sum and consumer sum must match.
 *
 *  University of Washington, Tacoma
 *  TCSS 422 - Operating Systems
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <time.h>
#include "matrix.h"
#include "counter.h"
#include "prodcons.h"
#include "pcmatrix.h"

int main (int argc, char * argv[])
{
  // Process command line arguments
  int numw = NUMWORK;
  if (argc==1)
  {
    BOUNDED_BUFFER_SIZE=MAX;
    NUMBER_OF_MATRICES=LOOPS;
    MATRIX_MODE=DEFAULT_MATRIX_MODE;
    printf("USING DEFAULTS: worker_threads=%d bounded_buffer_size=%d matricies=%d matrix_mode=%d\n",numw,BOUNDED_BUFFER_SIZE,NUMBER_OF_MATRICES,MATRIX_MODE);
  }
  else
  {
    if (argc==2)
    {
      numw=atoi(argv[1]);
      BOUNDED_BUFFER_SIZE=MAX;
      NUMBER_OF_MATRICES=LOOPS;
      MATRIX_MODE=DEFAULT_MATRIX_MODE;
    }
    if (argc==3)
    {
      numw=atoi(argv[1]);
      BOUNDED_BUFFER_SIZE=atoi(argv[2]);
      NUMBER_OF_MATRICES=LOOPS;
      MATRIX_MODE=DEFAULT_MATRIX_MODE;
    }
    if (argc==4)
    {
      numw=atoi(argv[1]);
      BOUNDED_BUFFER_SIZE=atoi(argv[2]);
      NUMBER_OF_MATRICES=atoi(argv[3]);
      MATRIX_MODE=DEFAULT_MATRIX_MODE;
    }
    if (argc==5)
    {
      numw=atoi(argv[1]);
      BOUNDED_BUFFER_SIZE=atoi(argv[2]);
      NUMBER_OF_MATRICES=atoi(argv[3]);
      MATRIX_MODE=atoi(argv[4]);
    }
    printf("USING: worker_threads=%d bounded_buffer_size=%d matricies=%d matrix_mode=%d\n",numw,BOUNDED_BUFFER_SIZE,NUMBER_OF_MATRICES,MATRIX_MODE);
  }

  time_t t;
  // Seed the random number generator with the system time
  srand((unsigned) time(&t));

  //
  // Demonstration code to show the use of matrix routines
  //
  // DELETE THIS CODE FOR YOUR SUBMISSION
  // ----------------------------------------------------------
  // bigmatrix = (Matrix **) malloc(sizeof(Matrix *) * BOUNDED_BUFFER_SIZE);
  // printf("MATRIX MULTIPLICATION DEMO:\n\n");
  // Matrix *m1, *m2, *m3;
  // for (int i=0;i<NUMBER_OF_MATRICES;i++)
  // {
  //   m1 = GenMatrixRandom();
  //   m2 = GenMatrixRandom();
  //   m3 = MatrixMultiply(m1, m2);
  //   if (m3 != NULL)
  //   {
  //     DisplayMatrix(m1,stdout);
  //     printf("    X\n");
  //     DisplayMatrix(m2,stdout);
  //     printf("    =\n");
  //     DisplayMatrix(m3,stdout);
  //     printf("\n");
  //     FreeMatrix(m3);
  //     FreeMatrix(m2);
  //     FreeMatrix(m1);
  //     m1=NULL;
  //     m2=NULL;
  //     m3=NULL;
  //   }
  // }
  // return 0;
  // ----------------------------------------------------------

    // Allocate memory for the bounded buffer
    bigmatrix = (Matrix **) malloc(sizeof(Matrix *) * BOUNDED_BUFFER_SIZE);
    if (bigmatrix == NULL) {
      fprintf(stderr, "Failed to allocate memory for bounded buffer\n");
      return 1;
    }



  printf("Producing %d matrices in mode %d.\n",NUMBER_OF_MATRICES,MATRIX_MODE);
  printf("Using a shared buffer of size=%d\n", BOUNDED_BUFFER_SIZE);
  printf("With %d producer and consumer thread(s).\n",numw);
  printf("\n");

  // define one producer and one consumer
  pthread_t pr;
  pthread_t co;

  // Variables to store the statistics returned by threads
  ProdConsStats *producer_stats = NULL;
  ProdConsStats *consumer_stats = NULL;

  // Create the producer thread
  if (pthread_create(&pr, NULL, prod_worker, NULL) != 0) {
    fprintf(stderr, "Failed to create producer thread\n");
    free(bigmatrix);
    return 1;
  }
  
  // Create the consumer thread
  if (pthread_create(&co, NULL, cons_worker, NULL) != 0) {
    fprintf(stderr, "Failed to create consumer thread\n");
    pthread_cancel(pr);
    free(bigmatrix);
    return 1;
  }

  // Wait for threads to complete and get their statistics
  pthread_join(pr, (void **)&producer_stats);
  pthread_join(co, (void **)&consumer_stats);
  


  // These are used to aggregate total numbers for main thread output
  int prs = 0; // total #matrices produced
  int cos = 0; // total #matrices consumed
  int prodtot = 0; // total sum of elements for matrices produced
  int constot = 0; // total sum of elements for matrices consumed
  int consmul = 0; // total # multiplications

  // consume ProdConsStats from producer and consumer threads
  if (producer_stats != NULL) {
    prs = producer_stats->matrixtotal;
    prodtot = producer_stats->sumtotal;
  }
  
  if (consumer_stats != NULL) {
    cos = consumer_stats->matrixtotal;
    constot = consumer_stats->sumtotal;
    consmul = consumer_stats->multtotal;
  }

  // consume ProdConsStats from producer and consumer threads [HINT: return from join]
  // add up total matrix stats in prs, cos, prodtot, constot, consmul

  printf("Sum of Matrix elements --> Produced=%d = Consumed=%d\n",prs,cos);
  printf("Matrices produced=%d consumed=%d multiplied=%d\n",prodtot,constot,consmul);

  // Free memory
  free(producer_stats);
  free(consumer_stats);
  free(bigmatrix);

  return 0;
}
