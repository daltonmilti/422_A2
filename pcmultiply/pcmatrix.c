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

  // Create arrays of threads for producers and consumers
  pthread_t *pr = (pthread_t *) malloc(sizeof(pthread_t) * numw);
  pthread_t *co = (pthread_t *) malloc(sizeof(pthread_t) * numw);

  if (pr == NULL || co == NULL) {
    fprintf(stderr, "Failed to allocate memory for thread arrays\n");
    free(bigmatrix);
    if (pr) free(pr);
    if (co) free(co);
    return 1;
  }

   // Arrays to store statistics from each thread
   ProdConsStats **producer_stats = (ProdConsStats **) malloc(sizeof(ProdConsStats *) * numw);
   ProdConsStats **consumer_stats = (ProdConsStats **) malloc(sizeof(ProdConsStats *) * numw);

   if (producer_stats == NULL || consumer_stats == NULL) {
    fprintf(stderr, "Failed to allocate memory for statistics arrays\n");
    free(bigmatrix);
    free(pr);
    free(co);
    if (producer_stats) free(producer_stats);
    if (consumer_stats) free(consumer_stats);
    return 1;
  }

  // Create producer threads
  for (int i = 0; i < numw; i++) {
    if (pthread_create(&pr[i], NULL, prod_worker, NULL) != 0) {
      fprintf(stderr, "Failed to create producer thread %d\n", i);
      // Clean up already created threads
      for (int j = 0; j < i; j++) {
        pthread_cancel(pr[j]);
      }
      free(bigmatrix);
      free(pr);
      free(co);
      free(producer_stats);
      free(consumer_stats);
      return 1;
    }
  }
  
  // Create consumer threads
  for (int i = 0; i < numw; i++) {
    if (pthread_create(&co[i], NULL, cons_worker, NULL) != 0) {
      fprintf(stderr, "Failed to create consumer thread %d\n", i);
      // Clean up already created threads
      for (int j = 0; j < numw; j++) {
        pthread_cancel(pr[j]);
      }
      for (int j = 0; j < i; j++) {
        pthread_cancel(co[j]);
      }
      free(bigmatrix);
      free(pr);
      free(co);
      free(producer_stats);
      free(consumer_stats);
      return 1;
    }
  }

  // Join producer threads and collect statistics
  for (int i = 0; i < numw; i++) {
    pthread_join(pr[i], (void **)&producer_stats[i]);
  }

  // Join consumer threads and collect statistics
  for (int i = 0; i < numw; i++) {
    pthread_join(co[i], (void **)&consumer_stats[i]);
  }
  


  // These are used to aggregate total numbers for main thread output
  int prs = 0; // total #matrices produced
  int cos = 0; // total #matrices consumed
  int prodtot = 0; // total sum of elements for matrices produced
  int constot = 0; // total sum of elements for matrices consumed
  int consmul = 0; // total # multiplications

  // Combine the stats from all producer threads
  for (int i = 0; i < numw; i++) {
    if (producer_stats[i] != NULL) {
      prs += producer_stats[i]->matrixtotal;
      prodtot += producer_stats[i]->sumtotal;
    }
  }

  // Combine the stats from all consumer threads
  for (int i = 0; i < numw; i++) {
    if (consumer_stats[i] != NULL) {
      cos += consumer_stats[i]->matrixtotal;
      constot += consumer_stats[i]->sumtotal;
      consmul += consumer_stats[i]->multtotal;
    }
  }


  printf("Sum of Matrix elements --> Produced=%d = Consumed=%d\n",prs,cos);
  printf("Matrices produced=%d consumed=%d multiplied=%d\n",prodtot,constot,consmul);

  // Free memory for statistics
  for (int i = 0; i < numw; i++) {
    if (producer_stats[i] != NULL) free(producer_stats[i]);
    if (consumer_stats[i] != NULL) free(consumer_stats[i]);
  }

    // Free memory for arrays
    free(producer_stats);
    free(consumer_stats);
    free(pr);
    free(co);
    free(bigmatrix);

  return 0;
}
