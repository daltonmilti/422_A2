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

	// Update 'in' index (circular buffer)
	in = (in + 1) % BOUNDED_BUFFER_SIZE;

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

    while (count == 0) {
        // Check if production has finished, preventing an infinite wait
        pthread_mutex_lock(&global_counter_mutex);
        int finished = (globalProduced >= NUMBER_OF_MATRICES);
        pthread_mutex_unlock(&global_counter_mutex);

        if (finished) {
            pthread_mutex_unlock(&buffer_mutex);
            return NULL; // Return NULL to signal that no more matrices will be produced
        }

        pthread_cond_wait(&not_empty, &buffer_mutex);
    }

    // Get matrix pointer from buffer at 'out'
    Matrix *value = bigmatrix[out];

    // Update 'out' index (circular buffer)
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
	// Allocate and initialize local statistics structure
	ProdConsStats *stats = malloc(sizeof(ProdConsStats));
	stats->sumtotal = 0;
	stats->matrixtotal = 0;
	stats->multtotal = 0;

	// Loop until global production counter reaches NUMBER_OF_MATRICES
	while (1) {
		// Lock global counter to safely check production limit
		pthread_mutex_lock(&global_counter_mutex);
		if (globalProduced >= NUMBER_OF_MATRICES) {
			pthread_mutex_unlock(&global_counter_mutex);
			break;
		}
		// Increment globalProduced count
		globalProduced++;
		pthread_mutex_unlock(&global_counter_mutex);

		// Generate a new matrix
		Matrix *mat = GenMatrixRandom();

		// Update local stats
		stats->sumtotal += SumMatrix(mat);
		stats->matrixtotal++;

		// Insert the new matrix into the bounded buffer
		put(mat);
	}

	// Return the statistics pointer so main() can aggregate results
	return stats;
}

// Matrix CONSUMER worker thread
void *cons_worker(void *arg)
{
    // Allocate and initialize local statistics structure
    ProdConsStats *stats = malloc(sizeof(ProdConsStats));
    stats->sumtotal = 0;
    stats->matrixtotal = 0;
    stats->multtotal = 0;

    while (1) {
        // Check if all matrices have been consumed
        pthread_mutex_lock(&global_counter_mutex);
        if (globalConsumed >= NUMBER_OF_MATRICES) {
            pthread_mutex_unlock(&global_counter_mutex);
            break;
        }
        pthread_mutex_unlock(&global_counter_mutex);

        // Retrieve the first matrix (M1) from the bounded buffer
        Matrix *m1 = get();
        if (m1 == NULL) { // Stop if no more matrices
            break;
        }

        stats->matrixtotal++;
        stats->sumtotal += SumMatrix(m1);

        Matrix *m2 = NULL;
        Matrix *result = NULL;

        // Try retrieving a valid second matrix (M2), avoid infinite loop
        int attempts = 0;
        while (attempts < NUMBER_OF_MATRICES) {
            m2 = get();
            if (m2 == NULL) {
                break; // Stop if no more matrices
            }

            stats->matrixtotal++;
            stats->sumtotal += SumMatrix(m2);

            if (m1->cols == m2->rows) {
                // Display matrices before multiplying
                printf("\nMATRIX MULTIPLICATION:\n");
                DisplayMatrix(m1, stdout);
                printf("    X\n");
                DisplayMatrix(m2, stdout);
                printf("    =\n");

                // Perform multiplication
                result = MatrixMultiply(m1, m2);
                break;
            } else {
                FreeMatrix(m2); // Free and retry
            }

            attempts++;
        }

        // If multiplication was successful, display the result
        if (result != NULL) {
            DisplayMatrix(result, stdout);
            stats->multtotal++;
            FreeMatrix(result);
        }

        FreeMatrix(m1);
        if (m2 != NULL) FreeMatrix(m2); // Avoid freeing NULL

        // Update the global consumption counter (m1 and m2 have been consumed)
        pthread_mutex_lock(&global_counter_mutex);
        globalConsumed += 2;
        pthread_mutex_unlock(&global_counter_mutex);
    }

    return stats;
}


