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

// ---------------------------------------------------------------------
// Shared Bounded Buffer and Synchronization Variables
// ---------------------------------------------------------------------

pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

// Circular buffer indices and count
int in = 0;    // Next index to produce into
int out = 0;   // Next index to consume from
int count = 0; // Number of matrices currently in buffer

// Global counters for production and consumption
int globalProduced = 0;
int globalConsumed = 0;
pthread_mutex_t global_counter_mutex = PTHREAD_MUTEX_INITIALIZER;

// ---------------------------------------------------------------------
// put() - Insert a matrix into the bounded buffer
// ---------------------------------------------------------------------
int put(Matrix *value)
{
    // Lock the buffer for exclusive access
    pthread_mutex_lock(&buffer_mutex);

    // If the buffer is full, wait
    while (count == BOUNDED_BUFFER_SIZE) {
        pthread_cond_wait(&not_full, &buffer_mutex);
    }

    // Insert matrix pointer at index 'in'
    bigmatrix[in] = value;
    in = (in + 1) % BOUNDED_BUFFER_SIZE; // Circular buffer wrap-around

    // Increment count and signal that the buffer is not empty
    count++;
    pthread_cond_signal(&not_empty);

    // Unlock and return
    pthread_mutex_unlock(&buffer_mutex);
    return 0;
}

// ---------------------------------------------------------------------
// get() - Remove a matrix from the bounded buffer
// ---------------------------------------------------------------------
Matrix *get()
{
    // Lock the buffer for exclusive access
    pthread_mutex_lock(&buffer_mutex);

    // If the buffer is empty, wait
    while (count == 0) {
        pthread_cond_wait(&not_empty, &buffer_mutex);
    }

    // Retrieve the matrix from index 'out'
    Matrix *value = bigmatrix[out];
    out = (out + 1) % BOUNDED_BUFFER_SIZE; // Circular buffer wrap-around

    // Decrement count and signal that the buffer is not full
    count--;
    pthread_cond_signal(&not_full);

    // Unlock and return
    pthread_mutex_unlock(&buffer_mutex);
    return value;
}

// ---------------------------------------------------------------------
// Producer Thread Function
// ---------------------------------------------------------------------
void *prod_worker(void *arg)
{
    // Allocate and initialize local statistics
    ProdConsStats *stats = malloc(sizeof(ProdConsStats));
    stats->sumtotal = 0;
    stats->matrixtotal = 0;
    stats->multtotal = 0;

    while (1) {
        // Lock the global counter to safely check/update production
        pthread_mutex_lock(&global_counter_mutex);
        if (globalProduced >= NUMBER_OF_MATRICES) {
            // We have produced enough matrices, stop
            pthread_mutex_unlock(&global_counter_mutex);
            break;
        }
        // Increment the globalProduced count
        globalProduced++;
        pthread_mutex_unlock(&global_counter_mutex);

        // Generate a new matrix
        Matrix *mat = GenMatrixRandom();
        // Update local stats
        stats->sumtotal += SumMatrix(mat);
        stats->matrixtotal++;

        // Put the matrix into the bounded buffer
        put(mat);
    }

    // Return stats so main can aggregate
    return stats;
}

// ---------------------------------------------------------------------
// Consumer Thread Function
// ---------------------------------------------------------------------
void *cons_worker(void *arg)
{
    // Allocate and initialize local statistics
    ProdConsStats *stats = malloc(sizeof(ProdConsStats));
    stats->sumtotal = 0;
    stats->matrixtotal = 0;
    stats->multtotal = 0;

    while (1) {
        // Check if we've consumed all matrices
        pthread_mutex_lock(&global_counter_mutex);
        if (globalConsumed >= NUMBER_OF_MATRICES) {
            pthread_mutex_unlock(&global_counter_mutex);
            break;
        }
        pthread_mutex_unlock(&global_counter_mutex);

        // Retrieve the first matrix (M1)
        Matrix *m1 = get();
        stats->matrixtotal++;
        stats->sumtotal += SumMatrix(m1);

        Matrix *m2 = NULL;
        Matrix *result = NULL;

        // Find a valid second matrix (M2)
        while (1) {
            m2 = get();
            stats->matrixtotal++;
            stats->sumtotal += SumMatrix(m2);

            if (m1->cols == m2->rows) {
                // Valid pair found; multiply them
                result = MatrixMultiply(m1, m2);
                break;
            } else {
                // Incompatible matrix, free M2 and try again
                FreeMatrix(m2);
                m2 = NULL;
            }
        }

        // If multiplication was successful, display the result
        if (result != NULL) {
            DisplayMatrix(result, stdout);
            stats->multtotal++;
            FreeMatrix(result);
        }

        // Free M2 if it wasn't already freed
        if (m2 != NULL) {
            FreeMatrix(m2);
        }

        // Free the first matrix
        FreeMatrix(m1);

        // Update the global consumption counter
        pthread_mutex_lock(&global_counter_mutex);
        // We consumed 2 matrices: m1 and m2
        globalConsumed += 2;
        pthread_mutex_unlock(&global_counter_mutex);
    }

    // Return stats so main can aggregate
    return stats;
}
