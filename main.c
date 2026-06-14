#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct{
    char* key; //string key
    void* value; // generic in C. points to its location in memory.
} hashmap_entry;

typedef struct{
    hashmap_entry* buckets; // array of entries. slots always exist, key is NULL when empty.
    size_t capacity; // unsigned ints, C convention for arrays
    size_t num_entries;
} hashmap;

#define INITIAL_CAPACITY 16 // macro. preprocessor will run over this and replace INITIAL_CAPACITY with 16 in our code.
#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

static uint64_t hash_key(char* key){
    uint64_t hash = FNV_OFFSET;
    for (char* c = key; *c; c++){  // *c is falsy when it hits the null terminator
        hash ^= (uint64_t)(unsigned char)(*c); // cast to unsigned char first to prevent sign extension
        hash *= FNV_PRIME;
    }
    return hash;
}

hashmap* hm_create(){  // returns a pointer to the newly created hashmap
    hashmap* hm = malloc(sizeof(hashmap));
    hm->capacity = INITIAL_CAPACITY;
    hm->num_entries = 0;

    // initialize all buckets to 0 using calloc
    hm->buckets = calloc(INITIAL_CAPACITY, sizeof(hashmap_entry)); // provides a pointer to a contiguous block of memory, all 0s

    // error checking. calloc returns NULL if memory could not be allocated
    if (hm->buckets == NULL) {
        free(hm); // entries failed, can't create hashmap, free what we already allocated
        return NULL;
    }
    return hm;
}

void hm_destroy(hashmap* hm){
    for (size_t i = 0; i < hm->capacity; i++){
        free(hm->buckets[i].key); // free strdup'd keys
    }
    free(hm->buckets);
    free(hm);
}

void* hm_get(hashmap* hm, char* key){
    uint64_t hash = hash_key(key);
    size_t index = (size_t)(hash & (uint64_t)(hm->capacity - 1));

    // if a NULL key slot is found before the desired key, the key was never inserted
    while (hm->buckets[index].key != NULL){
        if (strcmp(key, hm->buckets[index].key) == 0){
            return hm->buckets[index].value;
        }
        // key wasn't in this slot, linear probe to next
        index++;
        if(index >= hm->capacity){
            index = 0; // wrap around
        }
    }
    return NULL;
}

static char* hm_set_entry(hashmap_entry* buckets, size_t capacity, char* key, void* value, size_t* num_entries) {
    uint64_t hash = hash_key(key);
    size_t index = (size_t)(hash & (uint64_t)(capacity - 1));

    while (buckets[index].key != NULL) {
        if (strcmp(key, buckets[index].key) == 0) {
            buckets[index].value = value; // key exists, update value
            return buckets[index].key;
        }
        index++;
        if (index >= capacity) {
            index = 0;
        }
    }

    // found an empty slot, insert new entry
    if (num_entries != NULL) {
        key = strdup(key); // hashmap owns its own copy of the key
        if (key == NULL) { // error checking strdup, might fail to allocate memory
            return NULL;
        }
        (*num_entries)++; // dereference pointer to increment the actual count
    }
    // NULL is passed for num_entries during resize — keys already copied, count already correct
    buckets[index].key = key;
    buckets[index].value = value;
    return key;
}

static bool hm_expand(hashmap* hm) {
    size_t new_capacity = hm->capacity * 2;
    if (new_capacity < hm->capacity) { // in case size_t loops back around to 0
        return false;
    }
    hashmap_entry* new_buckets = calloc(new_capacity, sizeof(hashmap_entry));
    if (new_buckets == NULL) {
        return false;
    }

    // rehash all existing entries into new buckets (indices change with new capacity)
    for (size_t i = 0; i < hm->capacity; i++) {
        hashmap_entry entry = hm->buckets[i]; // copy by value
        if (entry.key != NULL) {
            hm_set_entry(new_buckets, new_capacity, entry.key, entry.value, NULL);
        }
    }

    free(hm->buckets);
    hm->buckets = new_buckets;
    hm->capacity = new_capacity;
    return true;
}

char* hm_insert(hashmap* hm, char* key, void* value){
    if (value == NULL){
        return NULL; // calloc sets all values to NULL, so we can't allow NULL insertions
    }

    if (hm->num_entries >= (3 * hm->capacity) / 4){
        if (!hm_expand(hm)) { // hm_expand runs and we check the boolean output
            return NULL;
        }
    }

    return hm_set_entry(hm->buckets, hm->capacity, key, value, &hm->num_entries);
}

int main(){
    hashmap* hm = hm_create();

    int a = 1, b = 2, c = 3;
    hm_insert(hm, "foo", &a);
    hm_insert(hm, "bar", &b);
    hm_insert(hm, "baz", &c);

    printf("foo: %d\n", *(int*)hm_get(hm, "foo"));
    printf("bar: %d\n", *(int*)hm_get(hm, "bar"));
    printf("baz: %d\n", *(int*)hm_get(hm, "baz"));
    printf("missing: %p\n", hm_get(hm, "missing"));

    hm_destroy(hm);
    return 0;
}