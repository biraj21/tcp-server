#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h> // for memcpy()

#include "./vector.h"

struct Vector {
  void *data;
  size_t length;
  size_t capacity;
  size_t elem_size;
};

static void safe_free(void **);

/**
 * initializes a new vector. returns NULL if memory allocation fails.
 *
 * @param elem_size the size of each element in the vector in bytes
 * @param capacity the initial capacity of the vector. if 0, it will be set to
 * `VECTOR_INIT_CAPACITY`
 */
struct Vector *vector_init(size_t elem_size, size_t capacity) {
  assert(elem_size > 0);

  struct Vector *vector = malloc(sizeof(struct Vector));
  if (vector == NULL) {
    return NULL;
  }

  if (capacity == 0) {
    capacity = VECTOR_INIT_CAPACITY;
  }

  vector->data = malloc(elem_size * capacity);
  if (vector->data == NULL) {
    free(vector);
    return NULL;
  }

  vector->length = 0;
  vector->capacity = capacity;
  vector->elem_size = elem_size;

  return vector;
}

/**
 * frees the vector and its data
 *
 * @param vector the vector to free
 */
void vector_free(struct Vector *vector) {
  assert(vector != NULL);

  safe_free(&vector->data);
  safe_free((void **)&vector);
}

/**
 * pushes an element to the end of the vector. returns false on failure.
 *
 * @param vector the vector to push the element to
 * @param element the element to push
 */
bool vector_push(struct Vector *vector, void *element) {
  assert(vector != NULL);

  // if the vector is full, double its capacity
  if (vector->length == vector->capacity) {
    size_t new_capacity = vector->capacity * 2;
    vector->data = realloc(vector->data, vector->elem_size * new_capacity);
    if (vector->data == NULL) {
      return false;
    }

    vector->capacity *= 2;
  }

  // copy the element to the end of the vector
  memcpy(&((char *)vector->data)[vector->length * vector->elem_size], element,
         vector->elem_size);

  vector->length++;

  return true;
}

void *vector_pop(struct Vector *vector) {
  assert(vector != NULL);
  assert(vector->length > 0);

  // just reduce the length. we don't actually remove the element
  vector->length--;

  return &((char *)vector->data)[vector->length * vector->elem_size];
}

/**
 * gets an element from the vector at the specified index
 *
 * @param vector the vector to get the element from
 */
void *vector_get(struct Vector *vector, size_t index) {
  assert(vector != NULL);
  assert(index < vector->length);

  return &((char *)vector->data)[index * vector->elem_size];
}

/**
 * sets an element in the vector at the specified index
 *
 * @param vector the vector to set the element in
 * @param index the index to set the element at
 * @param element the element to set
 */
void vector_set(struct Vector *vector, size_t index, void *element) {
  assert(vector != NULL);
  assert(index < vector->length);

  memcpy(&((char *)vector->data)[index * vector->elem_size], element,
         vector->elem_size);
}

/**
 * clears the vector's data. it DOES NOT free the data. it just sets the length
 * to 0.
 *
 * @param vector the vector to clear
 */
void vector_clear(struct Vector *vector) {
  assert(vector != NULL);

  vector->length = 0;
}

/**
 * get the entire data array of the vector
 *
 * @param vector the vector to get the data from
 */
void *vector_data(struct Vector *vector) {
  assert(vector != NULL);

  return vector->data;
}

/**
 * gets the length of the vector
 *
 * @param vector the vector to get the length of
 */
size_t vector_length(struct Vector *vector) {
  assert(vector != NULL);

  return vector->length;
}

/**
 * gets the capacity of the vector
 *
 * @param vector the vector to get the capacity of
 */
size_t vector_capacity(struct Vector *vector) {
  assert(vector != NULL);

  return vector->capacity;
}

/**
 * gets the element size of elements in the vector
 *
 * @param vector the vector to get the size of each element of
 */
size_t vector_elem_size(struct Vector *vector) {
  assert(vector != NULL);

  return vector->elem_size;
}

/**
 * frees the `malloc`'d memory. it takes a pointer to a pointer so that it can
 * set the pointer to NULL after freeing it.
 */
static void safe_free(void **ptr) {
  assert(ptr != NULL);

  if (*ptr == NULL) {
    return;
  }

  free(*ptr);
  *ptr = NULL;
}