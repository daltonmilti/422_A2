/*
 *  prodcons module
 *  Producer Consumer module
 *
 *  Implements routines for the producer consumer module based on
 *  chapter 30, section 2 of Operating Systems: Three Easy Pieces
 *
 *  University of Washington, Tacoma
 *  TCSS 422 - Operating Systems
 */

// Include only libraries for this module
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "counter.h"
#include "matrix.h"
#include "pcmatrix.h"
#include "prodcons.h"
#include <pthread.h>

// Define Locks, Condition variables, and so on here
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

// Indices for the buffer
int in = 0; // Next index to produce into
int out = 0; // Next index to consume from
int count = 0; // Number of matrices currently in buffer

// Global counters for production and consumption
int globalProduced = 0;
int globalConsumed = 0;
pthread_mutex_t global_counter_mutex = PTHREAD_MUTEX_INITIALIZER;


// Bounded buffer put() get()
int put(Matrix * value)
{
	// Lock the buffer for exclusive access
	pthread_mutex_lock(&buffer_mutex);

	// If the buffer is full, wait until a consumer removes an item
	while (count == BOUNDED_BUFFER_SIZE) {
		pthread_cond_wait(&not_full, &buffer_mutex);
	}

	// Insert matrix pointer into buffer at 'in'
	bigmatrix[in] = value;

	// Update 'in' index
	in = (in + 1) % BOUNDED_BUFFER_SIZE; // We modulo 'in' because this is a circular buffer

	// Increment 'count'
	count++;

	// Signal that there is at least one item available for consumers
	pthread_cond_signal(&not_empty);

	// Unlock the buffer
	pthread_mutex_unlock(&buffer_mutex);

	// Success
	return 0;
}

Matrix * get()
{
	// Lock the buffer for exclusive access
	pthread_mutex_lock(&buffer_mutex);

	// If the buffer is empty, wait until a producer adds an item
	while (count == 0) {
		pthread_cond_wait(&not_empty, &buffer_mutex);
	}

	// Get matrix pointer from buffer at 'out'
	Matrix *value = bigmatrix[out];

	// Update 'out' index
	out = (out + 1) % BOUNDED_BUFFER_SIZE;

	// Decrement 'count'
	count--;

	// Signal that there is space available for producers
	pthread_cond_signal(&not_full);

	// Unlock the buffer
	pthread_mutex_unlock(&buffer_mutex);

	// Return the matrix pointer
 	return value;
}

// Matrix PRODUCER worker thread
void *prod_worker(void *arg)
{
	// Allocate and Initialize local statisitcs structure
	ProdConsStats *stats = malloc(sizeof(ProdConsStats));
	stats->sumTotal = 0;
	stats->matrixTotal = 0;
	stats->multtotal = 0;

	// Loop until global production counter reaches NUMBER_OF_MATRICIES
	while (globalProduced <= NUMBER_OF_MATRICIES) {
		// Generate a new matrix
		Matrix *mat = GenMatrixRandom();

		// Update local stats
		stats->sumTotal += SumMatrix(mat);
		stats->matrixTotal++;

        // Insert the generated matrix into the bounded buffer.
        put(mat);
    }

    // Return the statistics pointer so main() can aggregate results.
    return stats;
}

// Matrix CONSUMER worker thread
void *cons_worker(void *arg) {
    // Allocate and initialize the statistics structure for this thread.
    ProdConsStats *stats = malloc(sizeof(ProdConsStats));
    stats->sumtotal = 0;
    stats->matrixtotal = 0;
    stats->multtotal = 0;

    while (1) {
        // Check if all matrices have been consumed.
        pthread_mutex_lock(&global_counter_mutex);
        if (globalConsumed >= NUMBER_OF_MATRICES) {
            pthread_mutex_unlock(&global_counter_mutex);
            break;  // No more matrices to consume.
        }
        pthread_mutex_unlock(&global_counter_mutex);

        // Retrieve the first matrix (M1) from the bounded buffer.
        Matrix *m1 = get();
        stats->matrixtotal++;
        stats->sumtotal += SumMatrix(m1);

        Matrix *m2 = NULL;
        Matrix *result = NULL;

        // Loop to find a valid second matrix (M2) that can be multiplied with M1.
        while (1) {
            m2 = get();
            stats->matrixtotal++;
            stats->sumtotal += SumMatrix(m2);
            if (m1->cols == m2->rows) {
                // Valid pair found; multiply them.
                result = MatrixMultiply(m1, m2);
                break;
            } else {
                // The matrices are incompatible; free M2 and try again.
                FreeMatrix(m2);
            }
        }

        // If multiplication was successful, display and record it.
        if (result != NULL) {
            DisplayMatrix(result, stdout);
            stats->multtotal++;
            FreeMatrix(result);
        }

        // Free the first matrix.
        FreeMatrix(m1);

        // Update the global consumption counter (note: m1 and m2 have both been consumed).
        pthread_mutex_lock(&global_counter_mutex);
        globalConsumed += 2;
        pthread_mutex_unlock(&global_counter_mutex);
    }

    // Return the statistics pointer so main() can aggregate results.
    return stats;
}
