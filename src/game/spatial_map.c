#include "game/spatial_map.h"


CF_V2 spatial_grid_section_extents(Spatial_Grid grid)
{
    CF_V2 extents = cf_extents(grid.bounds);
    CF_V2 section_extents = cf_div(extents, cf_v2((f32)grid.partition_w, (f32)grid.partition_h));
    return section_extents;
}

Spatial_Grid_Indices spatial_grid_get_indices(Spatial_Grid grid, CF_Aabb aabb)
{
    CF_V2 section_extents = spatial_grid_section_extents(grid);
    
    CF_Aabb local_aabb = move_aabb(aabb, cf_neg(grid.bounds.min));
    
    CF_V2 local_min = cf_div(local_aabb.min, section_extents);
    CF_V2 local_max = cf_div(local_aabb.max, section_extents);
    
    s32 min_x = (s32)local_min.x;
    s32 min_y = (s32)local_min.y;
    s32 max_x = (s32)(local_max.x + 0.5f);
    s32 max_y = (s32)(local_max.y + 0.5f);
    
    Spatial_Grid_Indices indices = { 0 };
    indices.min_x = cf_clamp(min_x, 0, grid.partition_w - 1);
    indices.min_y = cf_clamp(min_y, 0, grid.partition_h - 1);
    indices.max_x = cf_clamp(max_x, 0, grid.partition_w - 1);
    indices.max_y = cf_clamp(max_y, 0, grid.partition_h - 1);
    
    return indices;
}

Spatial_Grid spatial_grid_init(CF_Aabb bounds, s32 partition_w, s32 partition_h)
{
    s32 count = partition_w * partition_h;
    CF_ASSERT(count);
    
    Spatial_Grid grid = { 0 };
    grid.bounds = bounds;
    grid.partition_w = partition_w;
    grid.partition_h = partition_h;
    
    CF_V2 section_extents = spatial_grid_section_extents(grid);
    CF_ASSERT(section_extents.x > 0 && section_extents.y > 0);
    
    s32 depth = 32;
    
    for (s32 y = 0; y < partition_h; ++y)
    {
        for (s32 x = 0; x < partition_w; ++x)
        {
            CF_V2 min = cf_v2(x * section_extents.x, y * section_extents.y);
            CF_V2 max = cf_v2((x + 1) * section_extents.x, (y + 1) * section_extents.y);
            min = cf_add(min, bounds.min);
            max = cf_add(max, bounds.min);
            CF_Aabb section = cf_make_aabb(min, max);
            
            Spatial_Grid_Partition partition = { 0 };
            partition.bounds = section;
            cf_array_fit(partition.entities, depth);
            
            cf_array_push(grid.partitions, partition);
        }
    }
    
    return grid;
}

void spatial_map_init(Spatial_Map* spatial_map, CF_Aabb bounds, s32 partition_w, s32 partition_h)
{
    spatial_map->bounds = bounds;
    spatial_map->partition_w = partition_w;
    spatial_map->partition_h = partition_h;
    CF_ASSERT(partition_w > 0 && partition_h > 0);
}

void spatial_map_clear(Spatial_Map* spatial_map)
{
    for (s32 index = 0; index < cf_map_size(spatial_map->grids); ++index)
    {
        Spatial_Grid* grid = spatial_map->grids + index;
        for (s32 partition_index = 0; partition_index < cf_array_count(grid->partitions); ++partition_index)
        {
            cf_array_clear(grid->partitions[partition_index].entities);
        }
    }
}

void spatial_map_push(Spatial_Map* spatial_map, u64 team, ecs_entity_t entity, CF_Aabb aabb)
{
    if (!cf_map_has(spatial_map->grids, team))
    {
        Spatial_Grid grid = spatial_grid_init(spatial_map->bounds, spatial_map->partition_w, spatial_map->partition_h);
        
        cf_map_set(spatial_map->grids, team, grid);
    }
    
    Spatial_Grid* grid = cf_map_get_ptr(spatial_map->grids, team);
    
    Spatial_Grid_Indices indices = spatial_grid_get_indices(*grid, aabb);
    
    for (s32 y = indices.min_y; y <= indices.max_y; ++y)
    {
        for (s32 x = indices.min_x; x <= indices.max_x; ++x)
        {
            s32 partition_index = x + y * grid->partition_w;
            cf_array_push(grid->partitions[partition_index].entities, entity);
        }
    }
}

fixed ecs_entity_t* spatial_map_query(Spatial_Map* spatial_map, u64 mask, CF_Aabb bounds)
{
    fixed ecs_entity_t* result = NULL;
    
    if (cf_area_aabb(bounds) == 0.0f)
    {
        return NULL;
    }
    
    s32 count = 0;
    u64* teams = cf_map_keys(spatial_map->grids);
    for (s32 index = 0; index < cf_map_size(spatial_map->grids); ++index)
    {
        if (BIT_IS_SET(mask, teams[index]))
        {
            Spatial_Grid* grid = spatial_map->grids + index;
            for (s32 partition_index = 0; partition_index < cf_array_count(grid->partitions); ++partition_index)
            {
                count += cf_array_count(grid->partitions[partition_index].entities);
            }
        }
    }
    
    MAKE_SCRATCH_ARRAY(result, count);
    
    for (s32 team_index = 0; team_index < cf_map_size(spatial_map->grids); ++team_index)
    {
        if (BIT_IS_SET(mask, teams[team_index]))
        {
            Spatial_Grid* grid = spatial_map->grids + team_index;
            Spatial_Grid_Indices indices = spatial_grid_get_indices(*grid, bounds);
            
            for (s32 y = indices.min_y; y <= indices.max_y; ++y)
            {
                for (s32 x = indices.min_x; x <= indices.max_x; ++x)
                {
                    s32 partition_index = x + y * grid->partition_w;
                    
                    dyna ecs_entity_t* entities = grid->partitions[partition_index].entities;
                    
                    for (s32 entity_index = 0; entity_index < cf_array_count(entities); ++entity_index)
                    {
                        array_add_unique(result, entities[entity_index]);
                    }
                }
            }
        }
    }
    
    for (s32 index = cf_array_count(result) - 1; index >= 0; --index)
    {
        ecs_entity_t entity = result[index];
        C_Puppet* puppet = ECS_GET(entity, C_Puppet);
        CF_Aabb body_bounds = body_get_bounds(&puppet->body);
        if (!cf_overlaps(bounds, body_bounds))
        {
            cf_array_del(result, index);
        }
    }
    
    return result;
}
