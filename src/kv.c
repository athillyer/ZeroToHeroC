#include <kv.h>

// internal private
size_t hash(char *key, int capacity)
{
    size_t hash = 0x13371337deadbeef;

    // not cyrptographically sophisticated
    while (*key)
    {
        hash ^= *key;     // xor
        hash = hash << 8; // side step
        hash += *key;

        key++; // increment step
    }

    return hash % capacity;
}

// db - pointer to db
// key - pointer to key value
// value - pointer to the value at that key
// returns index if no error, -1 if failure, -2 if not found/capacity met
int kv_put(kv_t *db, char *key, char *value)
{
    if (!db || !key || !value)
        return -1;

    size_t idx = hash(key, db->capacity);

    for (int i = 0; i < db->capacity - 1; i++)
    {
        // key already set, updating
        // land in slot that is empty -> null, or empty -> TOMBSTONE
        size_t real_idx = (idx + i) % db->capacity;
        kv_entry_t *entry = &db->entries[real_idx];

        if (entry->key && entry->key !=TOMBSTONE && !strcmp(entry->key, key))
        {
            char *newval = strdup(value); // because we don't know the lifetime.
            if (!newval)        
                return -1; // failed alloc

            free(entry->value);
            entry->value = newval;
            return real_idx;
        }

        // null or tombstone
        if (!entry->key || entry->key == TOMBSTONE)
        {
            char *newval = strdup(value);
            char *newkey = strdup(key);
            if (!newval || !newkey)
            {
                free(newkey);
                free(newval);
                return -1;
            }

            entry->key = newkey;
            entry->value = newval;
            db->count++;
            return real_idx;
        }

    }

    return -2; // occupied/no capacity
}

char *kv_get(kv_t *db, char *key)
{
    if(!db || !key) return NULL;

    int idx = hash(key, db->capacity);

    for(int i = 0; i < db->capacity -1; i++)
    {
        size_t real_idx = (idx + i) % db->capacity;
        kv_entry_t *entry = &db->entries[real_idx];
        
        if(entry->key == NULL)
        {
            return NULL;
        }

        //equal is 0, which would be the only "false" result from strcmp
        if(entry->key && entry->key !=TOMBSTONE && !strcmp(entry->key, key))
        {
            return entry->value;
        }
    }

    return NULL;
}

//-1 error, //-2 no entry, //0 success
int kv_delete(kv_t *db, char *key)
{
    if(!db || !key) return -1;
    int idx = hash(key, db->capacity);

    for(int i = 0; i < db->capacity -1; i++)
    {
        size_t real_idx = (idx + i) %db->capacity;
        kv_entry_t *entry = &db->entries[real_idx];

        if(entry->key == NULL) return -2;

        if(entry->key && entry->key != TOMBSTONE && !strcmp(entry->key, key))
        {
            free(entry->key);
            free(entry->value);
            entry->key = TOMBSTONE;
            entry->value = NULL;
            return 0;
        }
    }

    return -2;
}

kv_t *kv_init(size_t capacity)
{
    if (capacity == 0)
        return NULL;

    kv_t *table = malloc(sizeof(kv_t));
    if (table == NULL)
    {
        return NULL;
    }

    table->capacity = capacity;
    table->count = 0;

    table->entries = calloc(sizeof(kv_entry_t), capacity);
    if (table->entries == NULL)
    {
        return NULL;
    }

    return table;
}

void kv_free(kv_t *db)
{
    for(int i = 0; i < db->capacity -1; i++)
    {
        kv_entry_t *entry = &db->entries[i];
        if(entry->key != TOMBSTONE && entry->key != NULL)
        {
            free(entry->key);
            free(entry->value);
        }
    }

    free(db->entries);
    free(db);
}