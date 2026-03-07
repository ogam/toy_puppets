#ifndef SPATIAL_MAP_H
#define SPATIAL_MAP_H

typedef struct Spatial_Grid_Indices
{
    s32 min_x;
    s32 min_y;
    s32 max_x;
    s32 max_y;
} Spatial_Grid_Indices;

typedef struct Spatial_Grid_Partition
{
    CF_Aabb bounds;
    dyna struct ecs_entity_t* entities;
} Spatial_Grid_Partition;

typedef struct Spatial_Grid
{
    CF_Aabb bounds;
    s32 partition_w;
    s32 partition_h;
    dyna Spatial_Grid_Partition* partitions;
} Spatial_Grid;

typedef struct Spatial_Map
{
    CF_Aabb bounds;
    s32 partition_w;
    s32 partition_h;
    CF_MAP(Spatial_Grid) grids;
} Spatial_Map;

CF_V2 spatial_grid_section_extents(Spatial_Grid grid);
Spatial_Grid_Indices spatial_grid_get_indices(Spatial_Grid grid, CF_Aabb aabb);
Spatial_Grid spatial_grid_init(CF_Aabb bounds, s32 partition_w, s32 partition_h);
void spatial_map_init(Spatial_Map* spatial_map, CF_Aabb bounds, s32 partition_w, s32 partition_h);
void spatial_map_clear(Spatial_Map* spatial_map);
void spatial_map_push(Spatial_Map* spatial_map, u64 team, struct ecs_entity_t entity, CF_Aabb aabb);
fixed struct ecs_entity_t* spatial_map_query(Spatial_Map* spatial_map, u64 mask, CF_Aabb bounds);

#endif //SPATIAL_MAP_H
