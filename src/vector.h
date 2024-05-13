#ifndef VECTOR_H
#define VECTOR_H

#include <stdbool.h>
#include <stddef.h>

struct Vector;

#define VECTOR_INIT_CAPACITY 8

struct Vector *vector_init(size_t elem_size, size_t capacity);
void vector_free(struct Vector *vector);

bool vector_push(struct Vector *vector, void *element);
void *vector_pop(struct Vector *vector);

void *vector_get(struct Vector *vector, size_t index);
void vector_set(struct Vector *vector, size_t index, void *element);

void vector_clear(struct Vector *vector);

// ----- getters ----- //

void *vector_data(struct Vector *vector);
size_t vector_length(struct Vector *vector);
size_t vector_capacity(struct Vector *vector);
size_t vector_elem_size(struct Vector *vector);

#endif