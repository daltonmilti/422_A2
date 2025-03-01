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
  return NULL;
}

// Matrix CONSUMER worker thread
void *cons_worker(void *arg)
{
  return NULL;
}
