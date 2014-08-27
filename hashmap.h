#ifndef _HASHMAP_H_
#define _HASHMAP_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdlib.h>
#include <string.h>

    typedef struct HashMap HashMap;
    
    typedef void(*hm_enum_func)(const char *key, const char *value, const void *obj);
    
    HashMap * hm_new(unsigned int capacity);
    
    void hm_delete(HashMap *map);
    
    int hm_get(const HashMap *map, const char *key, char *out_buf, unsigned int n_out_buf);
    
    int hm_exists(const HashMap *map, const char *key);
    
    int hm_put(HashMap *map, const char *key, const char *value);
    
    int hm_get_count(const HashMap *map);
    
    int hm_enum(const HashMap *map, hm_enum_func enum_func, const void *obj);

#ifdef __cplusplus
}
#endif

#endif

