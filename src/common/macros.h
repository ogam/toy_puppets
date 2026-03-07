#ifndef MACROS_H
#define MACROS_H

#ifndef TARGET_FRAMERATE
#define TARGET_FRAMERATE 60
#endif

// 384 * 5 = 1920
// 216 * 5 = 1080

#ifndef BACKGROUND_WIDTH
#define BACKGROUND_WIDTH 384
#endif

#ifndef BACKGROUND_HEIGHT
#define BACKGROUND_HEIGHT 216
#endif

#ifndef GAME_WIDTH
#define GAME_WIDTH 1920
#endif

#ifndef GAME_HEIGHT
#define GAME_HEIGHT 1080
#endif

#ifndef UNUSED
#define UNUSED(VAR) (void)VAR
#endif

#ifndef fixed
#define fixed
#endif

#ifndef KB
#define KB(X) (1024 * X)
#endif

#ifndef MB
#define MB(X) (KB(1024) * X)
#endif

#ifndef BIT
#define BIT(INDEX) (1LLU << INDEX)
#endif

#ifndef BIT_IS_SET_EX
#define BIT_IS_SET_EX(VALUE, MASK) ( ((VALUE) & (MASK)) != 0 )
#endif

#ifndef BIT_IS_SET
#define BIT_IS_SET(VALUE, INDEX) BIT_IS_SET_EX(VALUE, BIT(INDEX))
#endif

#ifndef BIT_SET_EX
#define BIT_SET_EX(VALUE, MASK) (VALUE |= MASK)
#endif

#ifndef BIT_SET
#define BIT_SET(VALUE, INDEX) (VALUE |= BIT(INDEX))
#endif

#ifndef BIT_UNSET_EX
#define BIT_UNSET_EX(VALUE, MASK) (VALUE &= ~MASK)
#endif

#ifndef BIT_UNSET
#define BIT_UNSET(VALUE, INDEX) (VALUE &= ~BIT(INDEX))
#endif

#ifndef BIT_TOGGLE_EX
#define BIT_TOGGLE_EX(VALUE, MASK) (VALUE ^= MASK)
#endif

#ifndef BIT_TOGGLE
#define BIT_TOGGLE(VALUE, INDEX) (VALUE ^= BIT(INDEX))
#endif

#ifndef BIT_ASSIGN
#define BIT_ASSIGN(VALUE, INDEX) (VALUE = BIT(INDEX))
#endif

#ifndef MAKE_SCRATCH_ARRAY
#define MAKE_SCRATCH_ARRAY(ARR, CAPACITY) \
{ \
size_t __buffer_size = sizeof(CK_ArrayHeader) + sizeof(*ARR) * CAPACITY; \
void* __buffer = scratch_alloc(__buffer_size); \
cf_array_static(ARR, __buffer, (s32)__buffer_size); \
}
#endif

#ifndef MAKE_ARENA_ARRAY
#define MAKE_ARENA_ARRAY(ARENA, ARR, CAPACITY) \
{ \
size_t __buffer_size = sizeof(CK_ArrayHeader) + sizeof(*ARR) * CAPACITY; \
void* __buffer = cf_arena_alloc(ARENA, (s32)__buffer_size); \
cf_array_static(ARR, __buffer, (s32)__buffer_size); \
}
#endif

#ifndef FOREACH_LIST
#define FOREACH_LIST(NODE, LIST)                    \
for (CF_ListNode *NODE = cf_list_begin(LIST);   \
NODE && NODE != cf_list_end(LIST);         \
NODE = NODE->next)
#endif

#ifndef FOREACH_LIST_REVERSE
#define FOREACH_LIST_REVERSE(NODE, LIST)                    \
for (CF_ListNode *NODE = cf_list_end(LIST)->prev;   \
NODE && NODE != cf_list_end(LIST);         \
NODE = NODE->prev)
#endif


// use this one if list can be mutated during iteration like adding to end or removing a node, you must manually advance the node
// with `NODE = NODE->next`
#ifndef FOREACH_LIST_UNSTABLE
#define FOREACH_LIST_UNSTABLE(NODE, LIST)           \
for (CF_ListNode *NODE = cf_list_begin(LIST);   \
NODE && NODE != cf_list_end(LIST);         \
)
#endif

#undef ENUM_MEMBER
#define ENUM_MEMBER(PREFINAME, NAME) PREFINAME##_##NAME,

#undef ENUM_STRING
#define ENUM_STRING(PREFINAME, NAME) #NAME,

#undef ENUM_MEMBER_CONCAT
#define ENUM_MEMBER_CONCAT(PREFIX, MEMBER) PREFIX##_MEMBER

#undef MAKE_ENUM
#define MAKE_ENUM(ENUM_NAME) \
enum { ENUM_NAME(ENUM_NAME, ENUM_MEMBER) }; \
const char* ENUM_NAME##_names[] = { ENUM_NAME(ENUM_NAME, ENUM_STRING) };

#ifndef COROUTINE_IS_DONE
#define COROUTINE_IS_DONE(CO) (CO.id == 0 || cf_coroutine_state(CO) == CF_COROUTINE_STATE_DEAD)
#endif

#ifndef COROUTINE_IS_RUNNING
#define COROUTINE_IS_RUNNING(CO) (CO.id != 0 && cf_coroutine_state(CO) != CF_COROUTINE_STATE_DEAD)
#endif

// creature bodies

#ifndef HUMAN_HEIGHT
#define HUMAN_HEIGHT 256.0f
#endif

#ifndef TENTACLE_HEIGHT
#define TENTACLE_HEIGHT 248.0f
#endif

#ifndef SLIME_HEIGHT
#define SLIME_HEIGHT 128.0f
#endif

#ifndef HAND_HEIGHT
#define HAND_HEIGHT 128.0f
#endif

#ifndef TOY_SWAP
#define TOY_SWAP(T, A, B) { T temp = A; A = B; B = A; }
#endif

#ifndef TOY_PLAYER_INDEX
#define TOY_PLAYER_INDEX 1ULL
#endif

#ifndef TOY_INVENTORY_INFINITE_QTY
#define TOY_INVENTORY_INFINITE_QTY 999
#endif

#ifndef TOY_INVENTORY_MAX_QTY
#define TOY_INVENTORY_MAX_QTY 9
#endif

#endif //MACROS_H
