#include "common/memory.h"

void init_memory()
{
    s_app->memory.scratch_arena = cf_make_arena(16, MB(32));
}

void* scratch_alloc(size_t size)
{
    return cf_arena_alloc(&s_app->memory.scratch_arena, (s32)size);;
}

void scratch_reset()
{
    cf_arena_reset(&s_app->memory.scratch_arena);
}

void* scratch_copy(void* src, size_t size)
{
    void* dst = scratch_alloc(size);
    CF_MEMCPY(dst, src, size);
    return dst;
}

void node_pool_init(Node_Pool* pool)
{
    pool->free_list = (CF_List){ 0 };
    pool->active_list = (CF_List){ 0 };
    cf_list_init(&pool->free_list);
    cf_list_init(&pool->active_list);
    pool->capacity = 0;
}

void node_pool_add(Node_Pool* pool, CF_ListNode* node)
{
    cf_list_push_back(&pool->free_list, node);
    pool->capacity++;
}

void node_pool_reset(Node_Pool* pool)
{
    FOREACH_LIST_UNSTABLE(node, &pool->active_list)
    {
        CF_ListNode* old_node = node;
        node = node->next;
        
        cf_list_remove(old_node);
        cf_list_push_front(&pool->free_list, old_node);
    }
}

CF_ListNode* node_pool_alloc(Node_Pool* pool)
{
    CF_ListNode* node = cf_list_pop_front(&pool->free_list);
    if (node != cf_list_end(&pool->free_list))
    {
        cf_list_push_back(&pool->active_list, node);
        return node;
    }
    return NULL;
}

void node_pool_free(Node_Pool* pool, CF_ListNode* node)
{
    cf_list_remove(node);
    cf_list_push_front(&pool->free_list, node);
}

CF_ListNode* node_pool_recycle_oldest(Node_Pool* pool)
{
    CF_ListNode* node = cf_list_pop_front(&pool->active_list);
    if (node)
    {
        cf_list_push_back(&pool->active_list, node);
        return node;
    }
    return NULL;
}

s32 node_pool_active_count(Node_Pool* pool)
{
    s32 count = 0;
    
    FOREACH_LIST(node, &pool->active_list)
    {
        count++;
    }
    
    return count;
}