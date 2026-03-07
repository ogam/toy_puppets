#ifndef MEMORY_H
#define MEMORY_H

typedef struct Memory
{
    CF_Arena scratch_arena;
} Memory;

void init_memory();
void* scratch_alloc(size_t size);
void scratch_reset();
void* scratch_copy(void* src, size_t size);

typedef struct Node_Pool
{
    CF_List free_list;
    CF_List active_list;
    //  @note:  this is mostly for debugging purposes
    s32 capacity;
} Node_Pool;

void node_pool_init(Node_Pool* pool);
void node_pool_add(Node_Pool* pool, CF_ListNode* node);
void node_pool_reset(Node_Pool* pool);
CF_ListNode* node_pool_alloc(Node_Pool* pool);
void node_pool_free(Node_Pool* pool, CF_ListNode* node);
CF_ListNode* node_pool_recycle_oldest(Node_Pool* pool);
s32 node_pool_active_count(Node_Pool* pool);

#endif //MEMORY_H
