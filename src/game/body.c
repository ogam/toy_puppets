#include "game/body.h"

Body_Proportions make_default_body_proportions()
{
    Body_Proportions proportions = { 0 };
    proportions.scale = 1.0f;
    proportions.head_scale = 1.0f;
    proportions.neck_thickness = 1.0f;
    proportions.torso_chubbiness = 1.0f;
    
    proportions.upper_arm_thickness = 1.0f;
    proportions.lower_arm_thickness = 1.0f;
    proportions.hand_thickness = 1.0f;
    
    proportions.upper_leg_thickness = 1.0f;
    proportions.lower_leg_thickness = 1.0f;
    proportions.foot_thickness = 1.0f;
    
    proportions.upper_arm_length = 1.0f;
    proportions.lower_arm_length = 1.0f;
    proportions.upper_leg_length = 1.0f;
    proportions.lower_leg_length = 1.0f;
    proportions.leg_length = 1.0f;
    return proportions;
}

Body_Proportions make_random_body_proportions()
{
    CF_Rnd* rnd = &s_app->world.rnd;
    
    Body_Proportions proportions = make_default_body_proportions();
    f32* values[] =
    {
        &proportions.scale,
        &proportions.head_scale,
        &proportions.neck_thickness,
        &proportions.torso_chubbiness,
        &proportions.upper_arm_thickness,
        &proportions.lower_arm_thickness,
        &proportions.hand_thickness,
        &proportions.upper_leg_thickness,
        &proportions.lower_leg_thickness,
        &proportions.foot_thickness,
        &proportions.upper_arm_length,
        &proportions.lower_arm_length,
        &proportions.upper_leg_length,
        &proportions.lower_leg_length,
        &proportions.leg_length,
    };
    
    u32 rand_mask = cf_rnd_range_int(rnd, -29401281, 84725);
    f32 min = 0.8f;
    f32 max = 1.15f;
    
    for (s32 index = 0; index < CF_ARRAY_SIZE(values); ++index)
    {
        if (BIT_IS_SET(rand_mask, index))
        {
            *values[index] = cf_rnd_range_float(rnd, min, max);
        }
    }
    
    return proportions;
}

Body make_default_body(Body_Type type, s32 particle_count, f32 height)
{
    Body body = { 0 };
    body.type = type;
    cf_array_fit(body.prev_particles, particle_count);
    cf_array_fit(body.particles, particle_count);
    cf_array_fit(body.accelerations, particle_count);
    cf_array_fit(body.constraints, particle_count * 2);
    cf_array_fit(body.is_locked, particle_count);
    cf_array_fit(body.hurt_particles, particle_count);
    
    body.facing_direction = 1.0f;
    body.height = height;
    body.base_height = height;
    body.time_step = 1.0f / TARGET_FRAMERATE;
    body.wheel_angle = CF_PI * 0.5f;
    
    for (s32 index = 0; index < particle_count; ++index)
    {
        cf_array_push(body.prev_particles, cf_v2(0.0f, (f32)index));
        cf_array_push(body.particles, cf_v2(0.0f, (f32)index));
        cf_array_push(body.accelerations, cf_v2(0, 0));
        cf_array_push(body.is_locked, false);
    }
    
    return body;
}

void destroy_body(Body* body)
{
    cf_array_free(body->prev_particles);
    cf_array_free(body->particles);
    cf_array_free(body->accelerations);
    cf_array_free(body->constraints);
    cf_array_free(body->is_locked);
    cf_array_free(body->hurt_particles);
    
    body->prev_particles = NULL;
    body->particles = NULL;
    body->accelerations = NULL;
    body->constraints = NULL;
    body->is_locked = NULL;
    body->hurt_particles = NULL;
}

void body_scale(Body* body, Body_Proportions proportions)
{
    body->height = body->base_height * proportions.scale;
    for (s32 index = 0; index < cf_array_count(body->constraints); ++index)
    {
        body->constraints[index].length = body->constraints[index].base_length * proportions.scale;
    }
    
    if (body->type == Body_Type_Human)
    {
        body_human_scale(body, proportions);
    }
}

void body_shatter(Body* body)
{
    CF_MEMSET(body->is_locked, 0, sizeof(*body->is_locked) * cf_array_size(body->is_locked));
    
    switch (body->type)
    {
        case Body_Type_Human:
        {
            // dislocate shoulders, hip and base of neck
            for (s32 index = cf_array_count(body->constraints) - 1; index >= 0; --index)
            {
                Particle_Constraint constraint = body->constraints[index];
                if ((constraint.a == Body_Human_Left_Shoulder_Socket && constraint.b == Body_Human_Left_Shoulder) || 
                    (constraint.a == Body_Human_Right_Shoulder_Socket && constraint.b == Body_Human_Right_Shoulder) ||
                    (constraint.a == Body_Human_Left_Hip_Socket && constraint.b == Body_Human_Left_Hip) || 
                    (constraint.a == Body_Human_Right_Hip_Socket && constraint.b == Body_Human_Right_Hip) || 
                    (constraint.a == Body_Human_Neck_Socket && constraint.b == Body_Human_Neck))
                {
                    cf_array_del(body->constraints, index);
                }
                
                // make sure limbs are stiff
                if (constraint.a == Body_Human_Left_Elbow || constraint.b == Body_Human_Left_Elbow ||
                    constraint.a == Body_Human_Right_Elbow || constraint.b == Body_Human_Right_Elbow ||
                    constraint.a == Body_Human_Left_Knee || constraint.b == Body_Human_Left_Knee ||
                    constraint.a == Body_Human_Right_Knee || constraint.b == Body_Human_Right_Knee ||
                    constraint.a == Body_Human_Head || constraint.b == Body_Human_Head)
                {
                    constraint.stiffness = 1.0f;
                }
            }
            
            break;
        }
        case Body_Type_Slime:
        {
            // separate into chunks
            break;
        }
        case Body_Type_Tubeman:
        {
            // dislocate shoulders
            for (s32 index = cf_array_count(body->constraints) - 1; index >= 0; --index)
            {
                Particle_Constraint constraint = body->constraints[index];
                if ((constraint.a == Body_Tubeman_Left_Shoulder && constraint.b == Body_Tubeman_Segment_Left_3) ||
                    (constraint.a == Body_Tubeman_Right_Shoulder && constraint.b == Body_Tubeman_Segment_Right_3))
                {
                    cf_array_del(body->constraints, index);
                }
                
                // make sure limbs are stiff
                if (constraint.a == Body_Tubeman_Left_Elbow || constraint.b == Body_Tubeman_Left_Elbow ||
                    constraint.a == Body_Tubeman_Right_Elbow || constraint.b == Body_Tubeman_Right_Elbow)
                {
                    constraint.stiffness = 1.0f;
                }
            }
            break;
        }
    }
}

void body_apply_friction(Body* body)
{
    f32 ground_y = body->position.y;
    
    for (s32 index = 0; index < cf_array_count(body->particles); ++index)
    {
        CF_V2 current = body->particles[index];
        CF_V2 prev = body->prev_particles[index];
        
        f32 y = current.y;
        f32 dy = y - ground_y;
        if (cf_abs(dy) < 1e-7f || y < ground_y)
        {
            body->accelerations[index].x += (prev.x - current.x) * 500.0f;
        }
    }
}

void body_acceleration_reset(Body* body)
{
    CF_MEMSET(body->accelerations, 0, sizeof(*body->accelerations) * cf_array_count(body->accelerations));
}

void body_verlet(Body* body)
{
    for (s32 index = 0; index < cf_array_count(body->particles); ++index)
    {
        if (body->is_locked[index])
        {
            continue;
        }
        CF_V2 p0 = body->prev_particles[index];
        CF_V2 p1 = body->particles[index];
        CF_V2 prev = p1;
        CF_V2 a = body->accelerations[index];
        f32 time_step = body->time_step;
        
        p1.x += p1.x - p0.x + a.x * time_step * time_step;;
        p1.y += p1.y - p0.y + a.y * time_step * time_step;;
        
        body->prev_particles[index] = prev;
        body->particles[index] = p1;
    }
}

void body_satisfy_constraints(Body* body, CF_Aabb bounds)
{
    s32 max_iterations = 5;
    for (s32 iteration = 0; iteration < max_iterations; ++iteration)
    {
        for (s32 index = 0; index < cf_array_count(body->constraints); ++index)
        {
            Particle_Constraint constraint = body->constraints[index];
            CF_V2 p0 = body->particles[constraint.a];
            CF_V2 p1 = body->particles[constraint.b];
            CF_V2 dp = cf_sub(p1, p0);
            
            f32 length = cf_len(dp);
            f32 diff = (length - constraint.length) / length * 0.5f * constraint.stiffness;
            
            CF_V2 delta = cf_mul_v2_f(dp, diff);
            
            p0 = cf_add(p0, delta);
            p1 = cf_sub(p1, delta);
            
            p0 = cf_clamp_aabb_v2(bounds, p0);
            p1 = cf_clamp_aabb_v2(bounds, p1);
            
            if (!body->is_locked[constraint.a])
            {
                body->particles[constraint.a] = p0;
            }
            if (!body->is_locked[constraint.b])
            {
                body->particles[constraint.b] = p1;
            }
        }
    }
}

void body_stabilize(Body* body)
{
    switch (body->type)
    {
        case Body_Type_Human:
        {
            body_human_stabilize(body);
            break;
        }
        case Body_Type_Tentacle:
        {
            body_tentacle_stabilize(body);
            break;
        }
        case Body_Type_Slime:
        {
            body_slime_stabilize(body);
            break;
        }
        case Body_Type_Hand:
        {
            body_hand_stabilize(body);
            break;
        }
        case Body_Type_Tubeman:
        {
            body_tubeman_stabilize(body);
            break;
        }
    }
}

CF_V2 body_centeroid(Body* body)
{
    CF_V2 centeroid = cf_v2(0, 0);
    if (cf_array_count(body->particles))
    {
        for (s32 index = 0; index < cf_array_count(body->particles); ++index)
        {
            centeroid = cf_add(centeroid, body->particles[index]);
        }
        centeroid = cf_div(centeroid, (f32)cf_array_count(body->particles));
    }
    
    return centeroid;
}

CF_V2 body_prev_centeroid(Body* body)
{
    CF_V2 centeroid = cf_v2(0, 0);
    if (cf_array_count(body->prev_particles))
    {
        for (s32 index = 0; index < cf_array_count(body->prev_particles); ++index)
        {
            centeroid = cf_add(centeroid, body->prev_particles[index]);
        }
        centeroid = cf_div(centeroid, (f32)cf_array_count(body->prev_particles));
    }
    
    return centeroid;
}

void body_draw_constraints(Body* body, f32 thickness)
{
    for (s32 index = 0; index < cf_array_count(body->constraints); ++index)
    {
        Particle_Constraint constraint = body->constraints[index];
        cf_draw_line(body->particles[constraint.a], body->particles[constraint.b], thickness);
    }
}

void body_draw(Body* body, Body_Proportions proportions)
{
    switch (body->type)
    {
        case Body_Type_Human:
        {
            body_human_draw(body, proportions);
            break;
        }
        case Body_Type_Tentacle:
        {
            body_tentacle_draw(body, proportions);
            break;
        }
        case Body_Type_Slime:
        {
            body_slime_draw(body, proportions);
            break;
        }
        case Body_Type_Hand:
        {
            body_hand_draw(body, proportions);
            break;
        }
        case Body_Type_Tubeman:
        {
            body_tubeman_draw(body, proportions);
            break;
        }
    }
}

s32 body_particles_touching_ground(Body* body)
{
    s32 count = 0;
    for (s32 index = 0; index < cf_array_count(body->particles); ++index)
    {
        f32 y = body->particles[index].y;
        f32 dy = y - body->position.y;
        if (cf_abs(dy) < 1e-7f || y < body->position.y)
        {
            ++count;
        }
    }
    
    return count;
}

CF_Aabb body_get_bounds(Body* body)
{
    CF_V2 min = cf_v2(F32_MAX, F32_MAX);
    CF_V2 max = cf_v2(-F32_MAX, -F32_MAX);
    CF_Aabb bounds = (CF_Aabb){ 0 };
    
    if (cf_array_count(body->particles))
    {
        if (body->type == Body_Type_Human)
        {
            s32 ignore_list[] = 
            {
                Body_Human_Left_Shoulder,
                Body_Human_Left_Elbow,
                Body_Human_Left_Hand,
                Body_Human_Right_Shoulder,
                Body_Human_Right_Elbow,
                Body_Human_Right_Hand,
            };
            
            for (s32 index = 0; index < cf_array_count(body->particles); ++index)
            {
                b32 skip = false;
                for (s32 ignore_index = 0; ignore_index < CF_ARRAY_SIZE(ignore_list); ++ignore_index)
                {
                    if (index == ignore_list[ignore_index])
                    {
                        skip = true;
                        break;
                    }
                }
                
                if (skip)
                {
                    continue;
                }
                
                CF_V2 position = body->particles[index];
                min = cf_min(min, position);
                max = cf_max(max, position);
            }
        }
        else if (body->type == Body_Type_Tubeman)
        {
            s32 ignore_list[] = 
            {
                Body_Tubeman_Left_Elbow,
                Body_Tubeman_Left_Hand,
                Body_Tubeman_Right_Elbow,
                Body_Tubeman_Right_Hand,
                Body_Tubeman_Left_Shoulder,
                Body_Tubeman_Right_Shoulder,
                Body_Tubeman_Hair_0_0,
                Body_Tubeman_Hair_0_1,
                Body_Tubeman_Hair_0_2,
                Body_Tubeman_Hair_1_0,
                Body_Tubeman_Hair_1_1,
                Body_Tubeman_Hair_1_2,
                Body_Tubeman_Hair_2_0,
                Body_Tubeman_Hair_2_1,
                Body_Tubeman_Hair_2_2,
            };
            
            for (s32 index = 0; index < cf_array_count(body->particles); ++index)
            {
                b32 skip = false;
                for (s32 ignore_index = 0; ignore_index < CF_ARRAY_SIZE(ignore_list); ++ignore_index)
                {
                    if (index == ignore_list[ignore_index])
                    {
                        skip = true;
                        break;
                    }
                }
                
                if (skip)
                {
                    continue;
                }
                
                CF_V2 position = body->particles[index];
                min = cf_min(min, position);
                max = cf_max(max, position);
            }
        }
        else
        {
            for (s32 index = 0; index < cf_array_count(body->particles); ++index)
            {
                CF_V2 position = body->particles[index];
                min = cf_min(min, position);
                max = cf_max(max, position);
            }
        }
        
        bounds = cf_make_aabb(min, max);
    }
    
    return bounds;
}

CF_Aabb body_get_particle_bounds(Body* body, s32* particles, s32 count)
{
    CF_V2 min = cf_v2(F32_MAX, F32_MAX);
    CF_V2 max = cf_v2(-F32_MAX, -F32_MAX);
    CF_Aabb bounds = (CF_Aabb){ 0 };
    
    if (count > 0)
    {
        for (s32 index = 0; index < count; ++index)
        {
            s32 particle_index = particles[index];
            min = cf_min(min, body->particles[particle_index]);
            max = cf_max(max, body->particles[particle_index]);
        }
        bounds = cf_make_aabb(min, max);
    }
    
    return bounds;
}

void body_apply_force(Body* body, CF_V2 force)
{
    for (s32 index = 0; index < cf_array_count(body->accelerations); ++index)
    {
        body->accelerations[index] = cf_add(body->accelerations[index], force);
    }
}

void body_teleport(Body* body, CF_V2 position)
{
    CF_V2 dp = cf_sub(position, body->position);
    body->position = position;
    
    for (s32 index = 0; index < cf_array_count(body->particles); ++index)
    {
        body->particles[index] = cf_add(body->particles[index], dp);
    }
    
    CF_MEMCPY(body->prev_particles, body->particles, sizeof(*body->particles) * cf_array_count(body->particles));
}

// @human

Body make_human_body(f32 height)
{
    Body body = make_default_body(Body_Type_Human, Body_Human_Count, height);
    
    f32 head_height = height * 1.00f;
    f32 torso_height = height * 0.90f;
    f32 torso_width = height * 0.1f;
    f32 hip_height = height * 0.5f;
    f32 hip_width = torso_width;
    f32 shoulder_width = torso_width * 0.5f;
    f32 upper_arm_length = height * 0.45f * 0.5f;
    f32 lower_arm_length = height * 0.45f * 0.5f;
    f32 upper_leg_length = height * 0.25f;
    f32 lower_leg_length = height * 0.25f;
    
    f32 left_elbow_x = -shoulder_width - upper_arm_length;
    f32 left_hand_x = -shoulder_width - upper_arm_length - lower_arm_length;
    
    f32 right_elbow_x = shoulder_width + upper_arm_length;
    f32 right_hand_x = shoulder_width + upper_arm_length + lower_arm_length;
    
    // setup particle positions
    {
        body.particles[Body_Human_Left_Hip] = cf_v2(-shoulder_width, hip_height);
        body.particles[Body_Human_Right_Hip] = cf_v2(shoulder_width, hip_height);
        body.particles[Body_Human_Left_Shoulder] = cf_v2(-shoulder_width, torso_height);
        body.particles[Body_Human_Right_Shoulder] = cf_v2(shoulder_width, torso_height);
        body.particles[Body_Human_Neck] = cf_v2(0.0f, torso_height);
        body.particles[Body_Human_Head] = cf_v2(0.0f, head_height);
        body.particles[Body_Human_Left_Elbow] = cf_v2(left_elbow_x, torso_height);
        body.particles[Body_Human_Right_Elbow] = cf_v2(right_elbow_x, torso_height);
        body.particles[Body_Human_Left_Hand] = cf_v2(left_hand_x, torso_height);
        body.particles[Body_Human_Right_Hand] = cf_v2(right_hand_x, torso_height);
        body.particles[Body_Human_Left_Knee] = cf_v2(-shoulder_width, upper_leg_length);
        body.particles[Body_Human_Right_Knee] = cf_v2(shoulder_width, upper_leg_length);
        body.particles[Body_Human_Left_Foot] = cf_v2(-shoulder_width, 0.0f);
        body.particles[Body_Human_Right_Foot] = cf_v2(shoulder_width, 0.0f);
        
        body.particles[Body_Human_Left_Hip_Socket] = body.particles[Body_Human_Left_Hip];
        body.particles[Body_Human_Right_Hip_Socket] = body.particles[Body_Human_Right_Hip];
        body.particles[Body_Human_Left_Shoulder_Socket] = body.particles[Body_Human_Left_Shoulder];
        body.particles[Body_Human_Right_Shoulder_Socket] = body.particles[Body_Human_Right_Shoulder];
        body.particles[Body_Human_Neck_Socket] = body.particles[Body_Human_Neck];
        
        CF_MEMCPY(body.prev_particles, body.particles, sizeof(*body.particles) * cf_array_count(body.particles));
    }
    
    Particle_Constraint constraint = { 0 };
    constraint.stiffness = 1.0f;
    
    // torso
    {
        constraint.a = Body_Human_Left_Hip_Socket;
        constraint.b = Body_Human_Right_Hip_Socket;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Human_Left_Hip_Socket;
        constraint.b = Body_Human_Left_Shoulder_Socket;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Human_Right_Hip_Socket;
        constraint.b = Body_Human_Right_Shoulder_Socket;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Human_Left_Shoulder_Socket;
        constraint.b = Body_Human_Right_Shoulder_Socket;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Human_Left_Shoulder_Socket;
        constraint.b = Body_Human_Right_Hip_Socket;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Human_Left_Hip_Socket;
        constraint.b = Body_Human_Right_Shoulder_Socket;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
    }
    
    // head
    {
        constraint.a = Body_Human_Left_Shoulder;
        constraint.b = Body_Human_Neck;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Human_Neck;
        constraint.b = Body_Human_Right_Shoulder;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Human_Neck;
        constraint.b = Body_Human_Head;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Human_Left_Shoulder;
        constraint.b = Body_Human_Head;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Human_Right_Shoulder;
        constraint.b = Body_Human_Head;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
    }
    
    // left arm
    {
        constraint.a = Body_Human_Left_Shoulder;
        constraint.b = Body_Human_Left_Elbow;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Human_Left_Elbow;
        constraint.b = Body_Human_Left_Hand;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
    }
    
    // right arm
    {
        constraint.a = Body_Human_Right_Shoulder;
        constraint.b = Body_Human_Right_Elbow;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Human_Right_Elbow;
        constraint.b = Body_Human_Right_Hand;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
    }
    
    // left leg
    {
        constraint.a = Body_Human_Left_Hip;
        constraint.b = Body_Human_Left_Knee;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Human_Left_Knee;
        constraint.b = Body_Human_Left_Foot;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Human_Left_Hip;
        constraint.b = Body_Human_Left_Foot;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
    }
    
    // right leg
    {
        constraint.a = Body_Human_Right_Hip;
        constraint.b = Body_Human_Right_Knee;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Human_Right_Knee;
        constraint.b = Body_Human_Right_Foot;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Human_Right_Hip;
        constraint.b = Body_Human_Right_Foot;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
    }
    
    // sockets
    {
        constraint.a = Body_Human_Left_Hip_Socket;
        constraint.b = Body_Human_Left_Hip;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Human_Right_Hip_Socket;
        constraint.b = Body_Human_Right_Hip;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Human_Left_Shoulder_Socket;
        constraint.b = Body_Human_Left_Shoulder;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Human_Right_Shoulder_Socket;
        constraint.b = Body_Human_Right_Shoulder;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Human_Neck_Socket;
        constraint.b = Body_Human_Neck;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
    }
    
    return body;
}

void body_human_stabilize(Body* body)
{
    CF_V2 midpoint = cf_lerp(body->particles[Body_Human_Left_Foot], 
                             body->particles[Body_Human_Right_Foot], 0.5f);
    
    CF_V2 offset = cf_v2((midpoint.x - body_centeroid(body).x) * 1000.0f, 10000.0f);
    
    // needed to keep bodies upright, otherwise they flail around
    for (s32 index = 0; index < cf_array_count(body->accelerations); ++index)
    {
        if (index == Body_Human_Left_Elbow || index == Body_Human_Left_Hand || 
            index == Body_Human_Right_Elbow || index == Body_Human_Right_Hand)
        {
            continue;
        }
        body->accelerations[index] = cf_add(body->accelerations[index], offset);
    }
}

b32 body_human_is_flipped(Body* body)
{
    CF_V2 right = cf_v2(1.0f, 0.0f);
    CF_V2 dp = cf_sub(body->particles[Body_Human_Right_Hip], body->particles[Body_Human_Left_Hip]);
    return cf_dot(dp, right) < 0.0f;
}

void body_human_scale(Body* body, Body_Proportions proportions)
{
    for (s32 index = 0; index < cf_array_count(body->constraints); ++index)
    {
        Particle_Constraint* constraint = body->constraints + index;
        
        // upper arms
        if ((constraint->a == Body_Human_Left_Shoulder || constraint->a == Body_Human_Left_Elbow) && 
            (constraint->b == Body_Human_Left_Shoulder || constraint->b == Body_Human_Left_Elbow))
        {
            constraint->length *= proportions.upper_arm_length;
        }
        else if ((constraint->a == Body_Human_Right_Shoulder || constraint->a == Body_Human_Right_Elbow) && 
                 (constraint->b == Body_Human_Right_Shoulder || constraint->b == Body_Human_Right_Elbow))
        {
            constraint->length *= proportions.upper_arm_length;
        }
        // lower arms
        else if ((constraint->a == Body_Human_Left_Elbow || constraint->a == Body_Human_Left_Hand) && 
                 (constraint->b == Body_Human_Left_Elbow || constraint->b == Body_Human_Left_Hand))
        {
            constraint->length *= proportions.lower_arm_length;
        }
        else if ((constraint->a == Body_Human_Right_Elbow || constraint->a == Body_Human_Right_Hand) && 
                 (constraint->b == Body_Human_Right_Elbow || constraint->b == Body_Human_Right_Hand))
        {
            constraint->length *= proportions.lower_arm_length;
        }
        // upper legs
        else if ((constraint->a == Body_Human_Left_Hip || constraint->a == Body_Human_Left_Knee) && 
                 (constraint->b == Body_Human_Left_Hip || constraint->b == Body_Human_Left_Knee))
        {
            constraint->length *= proportions.upper_leg_length;
        }
        else if ((constraint->a == Body_Human_Right_Hip || constraint->a == Body_Human_Right_Knee) && 
                 (constraint->b == Body_Human_Right_Hip || constraint->b == Body_Human_Right_Knee))
        {
            constraint->length *= proportions.upper_leg_length;
        }
        // lower legs
        else if ((constraint->a == Body_Human_Left_Knee || constraint->a == Body_Human_Left_Foot) && 
                 (constraint->b == Body_Human_Left_Knee || constraint->b == Body_Human_Left_Foot))
        {
            constraint->length *= proportions.lower_leg_length;
        }
        else if ((constraint->a == Body_Human_Right_Knee || constraint->a == Body_Human_Right_Foot) && 
                 (constraint->b == Body_Human_Right_Knee || constraint->b == Body_Human_Right_Foot))
        {
            constraint->length *= proportions.lower_leg_length;
        }
        // leg length
        else if ((constraint->a == Body_Human_Left_Hip || constraint->a == Body_Human_Left_Foot) && 
                 (constraint->b == Body_Human_Left_Hip || constraint->b == Body_Human_Left_Foot))
        {
            constraint->length *= proportions.leg_length;
        }
        else if ((constraint->a == Body_Human_Right_Hip || constraint->a == Body_Human_Right_Foot) && 
                 (constraint->b == Body_Human_Right_Hip || constraint->b == Body_Human_Right_Foot))
        {
            constraint->length *= proportions.leg_length;
        }
    }
}

void body_human_draw(Body* body, Body_Proportions proportions)
{
    f32 head_radius = body->height * 0.05f * proportions.head_scale;
    f32 neck_thickness = body->height * 0.02f * proportions.neck_thickness;
    f32 upper_arm_thickness = body->height * 0.04f * proportions.upper_arm_thickness;
    f32 lower_arm_thickness = body->height * 0.03f * proportions.lower_arm_thickness;
    f32 hand_thickness = body->height * 0.025f * proportions.hand_thickness;
    f32 upper_leg_thickness = body->height * 0.05f * proportions.upper_leg_thickness;;
    f32 lower_leg_thickness = body->height * 0.04f * proportions.lower_leg_thickness;;
    f32 foot_thickness = body->height * 0.02f * proportions.foot_thickness;;
    
    f32 torso_chubbiness = body->height * 0.02f * proportions.torso_chubbiness;
    
    CF_Poly torso = { 0 };
    torso.verts[0] = body->particles[Body_Human_Left_Hip_Socket];
    torso.verts[1] = body->particles[Body_Human_Right_Hip_Socket];
    torso.verts[2] = body->particles[Body_Human_Right_Shoulder_Socket];
    torso.verts[3] = body->particles[Body_Human_Left_Shoulder_Socket];
    torso.count = 4;
    cf_make_poly(&torso);
    
    s32 iterations = 16;
    
    CF_Color start_color = cf_draw_peek_color();
    CF_Color end_color = cf_color_black();
    end_color.a = start_color.a * 0.5f;
    
    CF_Color upper_color = cf_color_lerp(start_color, end_color, 0.025f);
    CF_Color lower_color = cf_color_lerp(start_color, end_color, 0.05f);
    
    cf_draw_push_color(lower_color);
    body_draw_line(body->particles[Body_Human_Left_Knee], body->particles[Body_Human_Left_Foot], lower_leg_thickness, foot_thickness, iterations);
    body_draw_line(body->particles[Body_Human_Right_Knee], body->particles[Body_Human_Right_Foot], lower_leg_thickness, foot_thickness, iterations);
    cf_draw_pop_color();
    
    cf_draw_push_color(upper_color);
    body_draw_line(body->particles[Body_Human_Left_Hip], body->particles[Body_Human_Left_Knee], upper_leg_thickness, lower_leg_thickness, iterations);
    body_draw_line(body->particles[Body_Human_Right_Hip], body->particles[Body_Human_Right_Knee], upper_leg_thickness, lower_leg_thickness, iterations);
    cf_draw_pop_color();
    
    cf_draw_push_color(upper_color);
    cf_draw_line(body->particles[Body_Human_Neck], body->particles[Body_Human_Head], neck_thickness);
    cf_draw_pop_color();
    
    body_draw_polygon_fill(torso, torso_chubbiness);
    
    cf_draw_circle_fill2(body->particles[Body_Human_Head], head_radius);
    
    cf_draw_push_color(lower_color);
    body_draw_line(body->particles[Body_Human_Left_Elbow], body->particles[Body_Human_Left_Hand], lower_arm_thickness, hand_thickness, iterations);
    body_draw_line(body->particles[Body_Human_Right_Elbow], body->particles[Body_Human_Right_Hand], lower_arm_thickness, hand_thickness, iterations);
    cf_draw_pop_color();
    
    cf_draw_push_color(upper_color);
    body_draw_line(body->particles[Body_Human_Left_Shoulder], body->particles[Body_Human_Left_Elbow], upper_arm_thickness, lower_arm_thickness, iterations);
    body_draw_line(body->particles[Body_Human_Right_Shoulder], body->particles[Body_Human_Right_Elbow], upper_arm_thickness, lower_arm_thickness, iterations);
    cf_draw_pop_color();
    
}

void body_human_hand_guard(Body* body)
{
    CF_V2 head = body->particles[Body_Human_Head];
    CF_V2 left_hand = cf_v2(head.x + body->facing_direction * body->height * 0.15f, head.y);
    CF_V2 right_hand = cf_v2(head.x + body->facing_direction * body->height * 0.15f, head.y);
    
    if (body->facing_direction < 0)
    {
        right_hand.x -= body->height * 0.1f;
    }
    else
    {
        left_hand.x += body->height * 0.1f;
    }
    
    CF_V2 down_direction = cf_sub(body->particles[Body_Human_Neck], body->particles[Body_Human_Head]);
    f32 y_elbow_offset = body->height * 0.1f;
    CF_V2 down_offset = cf_mul_v2_f(cf_safe_norm(down_direction), y_elbow_offset);
    
    body->particles[Body_Human_Left_Hand] = cf_lerp(body->particles[Body_Human_Left_Hand], left_hand, body->time_step);
    body->particles[Body_Human_Right_Hand] = cf_lerp(body->particles[Body_Human_Right_Hand], right_hand, body->time_step);
    
    if (body->particles[Body_Human_Left_Elbow].y > body->particles[Body_Human_Left_Hand].y)
    {
        body->particles[Body_Human_Left_Elbow] = cf_add(body->particles[Body_Human_Left_Elbow], down_offset);
    }
    
    if (body->particles[Body_Human_Right_Elbow].y > body->particles[Body_Human_Right_Hand].y)
    {
        body->particles[Body_Human_Right_Elbow] = cf_add(body->particles[Body_Human_Right_Elbow], down_offset);
    }
}

void body_human_walk(Body* body, CF_V2 direction, CF_V2 stride, f32 move_speed, f32 spin_speed)
{
    f32 dt = s_app->world.dt;
    
    CF_V2 wheel_direction = cf_v2(0, 0);
    if (direction.y > 0)
    {
        wheel_direction.y += 1.0f;
    }
    if (direction.y < 0)
    {
        wheel_direction.y += -1.0f;
    }
    if (direction.x > 0)
    {
        wheel_direction.x += -1.0f;
    }
    if (direction.x < 0)
    {
        wheel_direction.x += 1.0f;
    }
    
    body->wheel_angle += wheel_direction.x * CF_TAU * spin_speed;
    body->position = cf_add(body->position, cf_mul_v2_f(direction, move_speed * dt));
    
    if (body->wheel_angle < 0)
    {
        body->wheel_angle += CF_TAU;
    }
    if (body->wheel_angle > CF_TAU)
    {
        body->wheel_angle -= CF_TAU;
    }
    
    if (direction.x != 0.0f)
    {
        body->facing_direction = direction.x;
    }
    
    f32 left_foot_angle = body->wheel_angle + CF_PI * 0.5f;
    f32 right_foot_angle = body->wheel_angle - CF_PI * 0.5f;
    
    CF_SinCos left_sc = cf_sincos(left_foot_angle);
    CF_SinCos right_sc = cf_sincos(right_foot_angle);
    
    CF_V2 left_foot = cf_v2(stride.x * left_sc.c, stride.y * left_sc.s);
    CF_V2 right_foot = cf_v2(stride.x * right_sc.c, stride.y * right_sc.s);
    
    left_foot = cf_add(left_foot, body->position);
    right_foot = cf_add(right_foot, body->position);
    
    if (direction.y == 0.0f)
    {
        left_foot.y = cf_max(left_foot.y, body->position.y);
        right_foot.y = cf_max(right_foot.y, body->position.y);
    }
    
    body->particles[Body_Human_Left_Foot] = left_foot;
    body->particles[Body_Human_Right_Foot] = right_foot;
}

void body_human_idle(Body* body, CF_V2 stride, f32 spin_speed)
{
    f32 dt = s_app->world.dt;
    
    f32 target_angle = CF_PI * 0.5f;
    if (body_human_is_flipped(body))
    {
        target_angle = CF_PI * 1.5f;
    }
    
    body->wheel_angle = cf_lerp(body->wheel_angle, target_angle, dt);
    
    if (body->wheel_angle < 0)
    {
        body->wheel_angle += CF_TAU;
    }
    if (body->wheel_angle > CF_TAU)
    {
        body->wheel_angle -= CF_TAU;
    }
    
    f32 left_foot_angle = body->wheel_angle + CF_PI * 0.5f;
    f32 right_foot_angle = body->wheel_angle - CF_PI * 0.5f;
    
    CF_SinCos left_sc = cf_sincos(left_foot_angle);
    CF_SinCos right_sc = cf_sincos(right_foot_angle);
    
    CF_V2 left_foot = cf_v2(stride.x * left_sc.c, stride.y * left_sc.s);
    CF_V2 right_foot = cf_v2(stride.x * right_sc.c, stride.y * right_sc.s);
    
    left_foot = cf_add(left_foot, body->position);
    right_foot = cf_add(right_foot, body->position);
    
    body->particles[Body_Human_Left_Foot] = left_foot;
    body->particles[Body_Human_Right_Foot] = right_foot;
}

void body_human_kick_co(CF_Coroutine co)
{
    Body* body = (Body*)cf_coroutine_get_udata(co);
    World* world = &s_app->world;
    
    s32 off_index = Body_Human_Left_Foot;
    s32 off_hip_index = Body_Human_Left_Hip;
    s32 off_knee_index = Body_Human_Left_Knee;
    
    s32 hip_index = Body_Human_Right_Hip;
    s32 knee_index = Body_Human_Right_Knee;
    s32 foot_index = Body_Human_Right_Foot;
    
    if (body->facing_direction < 0)
    {
        hip_index = Body_Human_Left_Hip;
        knee_index = Body_Human_Left_Knee;
        foot_index = Body_Human_Left_Foot;
        off_index = Body_Human_Right_Foot;
        off_hip_index = Body_Human_Right_Hip;
        off_knee_index = Body_Human_Right_Knee;
    }
    
    if (body_human_is_flipped(body))
    {
        TOY_SWAP(foot_index, off_index);
        TOY_SWAP(hip_index, off_hip_index);
        TOY_SWAP(knee_index, off_knee_index);
    }
    
    CF_V2 old_foot_position = body->particles[foot_index];
    
    body->is_locked[Body_Human_Left_Hip] = true;
    body->is_locked[Body_Human_Right_Hip] = true;
    body->is_locked[Body_Human_Left_Shoulder] = true;
    body->is_locked[Body_Human_Right_Shoulder] = true;
    body->is_locked[off_index] = true;
    
    CF_V2 c0 = cf_v2(body->position.x, old_foot_position.y);
    CF_V2 c1 = c0;
    CF_V2 c2 = c0;
    
    c0 = cf_add(c0, cf_v2(body->facing_direction * body->height * -0.2f, body->height * 0.25f));
    c1 = cf_add(c1, cf_v2(body->facing_direction * body->height * 2.0f, body->height * 0.5f));
    
    cf_array_clear(body->hurt_particles);
    cf_array_push(body->hurt_particles, hip_index);
    cf_array_push(body->hurt_particles, knee_index);
    cf_array_push(body->hurt_particles, foot_index);
    
    f32 duration = 1.0f;
    f32 delay = 0.0f;
    while (delay < duration)
    {
        f32 t = cf_clamp01(delay / duration);
        t = cf_circle_in_out(t);
        
        CF_V2 foot_target = cf_bezier(c0, c1, c2, t);
        
        body->particles[foot_index] = cf_lerp(body->particles[foot_index], foot_target, t);
        delay += world->dt;
        cf_coroutine_yield(co);
    }
    
    cf_array_clear(body->hurt_particles);
    
    body->is_locked[Body_Human_Left_Hip] = false;
    body->is_locked[Body_Human_Right_Hip] = false;
    body->is_locked[Body_Human_Left_Shoulder] = false;
    body->is_locked[Body_Human_Right_Shoulder] = false;
    body->is_locked[off_index] = false;
}

CF_V2 body_human_get_punch_target(Body* body, Body* target_body)
{
    s32 hand_index = Body_Human_Left_Hand;
    if (body->facing_direction < 0)
    {
        hand_index = Body_Human_Right_Hand;
    }
    
    CF_V2 p0 = cf_v2(body->particles[hand_index].x, body->particles[Body_Human_Left_Shoulder].y);
    CF_V2 p1 = cf_v2(p0.x + body->facing_direction * body->height * 2.0f, p0.y);
    
    if (target_body && body->height > target_body->height)
    {
        CF_V2 target_centroid = body_centeroid(target_body);
        CF_V2 hit_direction = cf_sub(target_centroid, p0);
        hit_direction = cf_safe_norm(hit_direction);
        p1 = cf_add(p0, cf_mul_v2_f(hit_direction, body->height * 2.0f));
    }
    
    return p1;
}

void body_human_punch_co(CF_Coroutine co)
{
    Body* body = (Body*)cf_coroutine_get_udata(co);
    World* world = &s_app->world;
    CF_V2 hit_target = cf_v2(0.0f, 0.0f);
    
    if (cf_coroutine_bytes_pushed(co) >= sizeof(Body))
    {
        Body target_body;
        if (cf_coroutine_pop(co, &target_body, sizeof(Body)).code == CF_RESULT_SUCCESS)
        {
            hit_target = body_human_get_punch_target(body, &target_body);
        }
        else
        {
            hit_target = body_human_get_punch_target(body, NULL);
        }
    }
    else
    {
        hit_target = body_human_get_punch_target(body, NULL);
    }
    
    s32 off_index = Body_Human_Right_Elbow;
    s32 shoulder_index = Body_Human_Left_Shoulder;
    s32 elbow_index = Body_Human_Left_Elbow;
    s32 hand_index = Body_Human_Left_Hand;
    
    if (body->facing_direction < 0)
    {
        hand_index = Body_Human_Right_Hand;
        shoulder_index = Body_Human_Right_Shoulder;
        elbow_index = Body_Human_Right_Elbow;
        off_index = Body_Human_Left_Elbow;
    }
    
    CF_V2 old_hand_position = body->particles[hand_index];
    
    body->is_locked[Body_Human_Left_Hip] = true;
    body->is_locked[Body_Human_Right_Hip] = true;
    body->is_locked[Body_Human_Left_Foot] = true;
    body->is_locked[Body_Human_Right_Foot] = true;
    body->is_locked[off_index] = true;
    
    CF_V2 c0 = cf_v2(old_hand_position.x, body->particles[Body_Human_Left_Shoulder].y);
    CF_V2 c1 = hit_target;
    CF_V2 c2 = c0;
    
    c0 = cf_add(c0, cf_v2(body->facing_direction * body->height * -1.0f, body->height * -0.1f));
    
    cf_array_clear(body->hurt_particles);
    cf_array_push(body->hurt_particles, shoulder_index);
    cf_array_push(body->hurt_particles, elbow_index);
    cf_array_push(body->hurt_particles, hand_index);
    
    f32 duration = 1.0f;
    f32 delay = 0.0f;
    while (delay < duration)
    {
        f32 t = cf_clamp01(delay / duration);
        t = cf_circle_in_out(t);
        
        CF_V2 hand_target = cf_bezier(c0, c1, c2, t);
        
        body->particles[hand_index] = cf_lerp(body->particles[hand_index], hand_target, t);
        delay += world->dt;
        cf_coroutine_yield(co);
    }
    
    cf_array_clear(body->hurt_particles);
    
    body->is_locked[Body_Human_Left_Hip] = false;
    body->is_locked[Body_Human_Right_Hip] = false;
    body->is_locked[Body_Human_Left_Foot] = false;
    body->is_locked[Body_Human_Right_Foot] = false;
    body->is_locked[off_index] = false;
}

void body_human_fix_head_co(CF_Coroutine co)
{
    Body* body = (Body*)cf_coroutine_get_udata(co);
    
    World* world = &s_app->world;
    
    body->is_locked[Body_Human_Neck] = true;
    
    CF_V2* left_hand = body->particles + Body_Human_Left_Hand;
    CF_V2* right_hand = body->particles + Body_Human_Right_Hand;
    CF_V2* head = body->particles + Body_Human_Head;
    
    CF_V2 direction = cf_sub(body->particles[Body_Human_Left_Shoulder], body->particles[Body_Human_Left_Hip]);
    direction = cf_safe_norm(direction);
    
    CF_V2 c0 = *head;
    CF_V2 c1 = cf_add(c0, cf_mul_v2_f(direction, body->height * 0.15f));
    
    f32 duration = 1.0f;
    f32 delay = 0.0f;
    f32 spin_speed = 0.001f;
    
    while (delay < duration)
    {
        *left_hand = cf_lerp(*left_hand, c0, world->dt);
        *right_hand = cf_lerp(*right_hand, c0, world->dt);
        
        if (cf_distance(*left_hand, c0) < 5.0f && 
            cf_distance(*right_hand, c0) < 5.0f)
        {
            break;
        }
        
        CF_V2 stride = cf_v2(body->height * 0.1f, body->height * 0.1f);
        body_human_idle(body, stride, spin_speed);
        
        delay += world->dt;
        cf_coroutine_yield(co);
    }
    
    body->is_locked[Body_Human_Head] = true;
    
    duration = 0.5f;
    delay = 0.0f;
    while (delay < duration)
    {
        f32 t = cf_clamp01(delay / duration);
        
        CF_V2 target = cf_lerp(c0, c1, t);
        *left_hand = target;
        *right_hand = target;
        *head = target;
        
        CF_V2 stride = cf_v2(body->height * 0.1f, body->height * 0.1f);
        body_human_idle(body, stride, spin_speed);
        
        delay += world->dt;
        cf_coroutine_yield(co);
    }
    
    body->is_locked[Body_Human_Neck] = false;
    body->is_locked[Body_Human_Head] = false;
}

// @human

// @tenacle

Body make_tentacle_body(f32 height)
{
    Body body = make_default_body(Body_Type_Tentacle, Body_Tentacle_Count, height);
    
    s32 link_count = Body_Tentacle_Count - 1;
    for (s32 index = 0; index < Body_Tentacle_Count; ++index)
    {
        body.particles[index] = cf_v2(0.0f, height * (f32)index / link_count);
    }
    
    CF_MEMCPY(body.prev_particles, body.particles, sizeof(*body.particles) * cf_array_count(body.particles));
    
    for (s32 index = 0; index < link_count; ++index)
    {
        Particle_Constraint constraint = { 0 };
        constraint.a = index;
        constraint.b = index + 1;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        constraint.stiffness = cf_max(1.0f - (index + 1.0f) / link_count, 0.5f);
        cf_array_push(body.constraints, constraint);
    }
    
    return body;
}

void body_tentacle_stabilize(Body* body)
{
    CF_V2 offset = cf_v2((body->position.x - body_centeroid(body).x) * 1000.0f, 0.0f);
    
    for (s32 index = 0; index < cf_array_count(body->particles); ++index)
    {
        if (body->particles[index].y < body->position.y)
        {
            body->accelerations[index].y += 250.0f;
        }
    }
    
    // needed to keep bodies upright, otherwise they flail around
    for (s32 index = 0; index < cf_array_count(body->accelerations); ++index)
    {
        body->accelerations[index] = cf_add(body->accelerations[index], offset);
    }
}

void body_tentacle_draw(Body* body, Body_Proportions proportions)
{
    UNUSED(proportions);
    
    CF_Color start_color = cf_draw_peek_color();
    CF_Color end_color = cf_color_black();
    end_color.a = start_color.a;
    end_color = cf_color_lerp(start_color, end_color, 0.25f);
    
    for (s32 index = 2; index < Body_Tentacle_Count - 1; ++index)
    {
        f32 t = (f32)index / Body_Tentacle_Count;
        CF_Color color = cf_color_lerp(start_color, end_color, 1.0f - t);
        cf_draw_push_color(color);
        cf_draw_bezier_line(body->particles[Body_Tentacle_Root], body->particles[Body_Tentacle_Root + 1], body->particles[index], 32, 1.0f);
        cf_draw_pop_color();
    }
    
    cf_draw_bezier_line2(body->particles[Body_Tentacle_Root], body->particles[Body_Tentacle_Root + 1], body->particles[Body_Tentacle_Root + 2], body->particles[Body_Tentacle_Count - 1], 32, 1.0f);
}

void body_tentacle_idle(Body* body)
{
    s32 joint_index = Body_Tentacle_Root + 1;
    s32 tip_index = Body_Tentacle_Count - 1;
    CF_V2 tip_dp = cf_sub(body->particles[joint_index], body->prev_particles[joint_index]);
    f32 a_x = tip_dp.x > 0.0f ? -100.0f : 100.0f;
    
    body->accelerations[tip_index].x += a_x;
}

void body_tentacle_whip_co(CF_Coroutine co)
{
    Body* body = (Body*)cf_coroutine_get_udata(co);
    World* world = &s_app->world;
    
    CF_V2 c0 = cf_v2(body->position.x, body->position.y + body->height);
    CF_V2 c1 = c0;
    CF_V2 c2 = c0;
    
    c0.x -= body->facing_direction * body->height * 2.0f;
    c1.x += body->facing_direction * body->height * 4.0f;
    c1.y -= body->height * 0.5f;
    
    CF_V2* tip = &cf_array_last(body->particles);
    
    cf_array_clear(body->hurt_particles);
    
    f32 duration = 1.0f;
    f32 delay = 0.0f;
    while (delay < duration)
    {
        f32 t = cf_clamp01(delay / duration);
        
        if (t > 0.25f && cf_array_count(body->hurt_particles) == 0)
        {
            for (s32 index = Body_Tentacle_Root + 1; index < Body_Tentacle_Count; ++index)
            {
                cf_array_push(body->hurt_particles, index);
            }
        }
        
        t = cf_circle_in_out(t);
        
        CF_V2 target = cf_bezier(c0, c1, c2, t);
        
        *tip = cf_lerp(*tip, target, t);
        delay += CF_DELTA_TIME;
        cf_coroutine_yield(co);
    }
    
    cf_array_clear(body->hurt_particles);
}

// @tentacle

// @slime

void body_slime_reset_constraints(Body* body)
{
    for (s32 index = 1; index < cf_array_count(body->constraints); ++index)
    {
        if (body->constraints[index].a == 0)
        {
            body->constraints[index].stiffness = 0.01f;
        }
        else
        {
            body->constraints[index].stiffness = 0.3f;
        }
    }
}

Body make_slime_body(f32 height)
{
    Body body = make_default_body(Body_Type_Slime, Body_Slime_Count, height);
    
    f32 radius = height * 0.5f;
    CF_V2 center = cf_v2(0.0f, radius);
    
    body.particles[0] = center;
    
    for (s32 index = 1; index < cf_array_count(body.particles); ++index)
    {
        f32 angle = (f32)index / (cf_array_count(body.particles) - 1) * CF_TAU;
        CF_SinCos sc = cf_sincos(angle);
        CF_V2 position = cf_v2(radius * sc.c, radius * sc.s);
        position = cf_add(position, center);
        
        body.particles[index] = position;
    }
    
    CF_MEMCPY(body.prev_particles, body.particles, sizeof(*body.particles) * cf_array_count(body.particles));
    
    for (s32 index = 1; index < cf_array_count(body.particles); ++index)
    {
        Particle_Constraint constraint = { 0 };
        constraint.stiffness = 0.3f;
        constraint.a = index;
        constraint.b = index == cf_array_count(body.particles) - 1 ? 1 : index + 1;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
    }
    
    for (s32 index = 1; index < cf_array_count(body.particles); ++index)
    {
        Particle_Constraint constraint = { 0 };
        constraint.stiffness = 0.01f;
        constraint.a = 0;
        constraint.b = index;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
    }
    
    return body;
}

void body_slime_stabilize(Body* body)
{
    f32 radius = body->height * 0.5f;
    CF_V2 centeroid = body_centeroid(body);
    
    // pop body up from being in floor
    
    {
        f32 target_center_y = body->position.y + body->height * 0.25f;
        if (body->particles[Body_Slime_Center].y < target_center_y)
        {
            body->particles[Body_Slime_Center].y = target_center_y;
            body->prev_particles[Body_Slime_Center].y = body->particles[Body_Slime_Center].y;
        }
    }
    
    f32 distance = cf_distance(body->particles[Body_Slime_Center], centeroid);
    
    // try to unfold
    if (distance > body->height * 0.1f)
    {
        CF_V2 center = body->particles[Body_Slime_Center];
        
        // body is no longer in the correct circular shape
        for (s32 index = 1; index < cf_array_count(body->particles); ++index)
        {
            f32 angle = (f32)index / (cf_array_count(body->particles) - 1) * CF_TAU;
            CF_SinCos sc = cf_sincos(angle);
            CF_V2 position = cf_v2(radius * sc.c, radius * sc.s);
            position = cf_add(position, center);
            
            CF_V2 dp = cf_sub(position, body->particles[index]);
            body->particles[index] = cf_add(body->particles[index], cf_mul_v2_f(dp, body->time_step));
        }
    }
    
    // friction
    {
        CF_V2 delta = cf_sub(body->prev_particles[Body_Slime_Center], body->particles[Body_Slime_Center]);
        if (cf_abs(delta.x) > 1e-7f)
        {
            s32 ground_count = body_particles_touching_ground(body);
            body->accelerations[Body_Slime_Center].x += delta.x * 500.0f * ground_count;
        }
    }
    
    body->accelerations[Body_Slime_Center].y += -1000.0f;
    // it's a bit weird align the position.x here to center but slime can squash and squish and fly
    // in weird ways so this needs to be handled somewhere
    // it can't be done in the walk since walk for slimes are hops instead of walking across the floor
    body->position.x = body->particles[Body_Slime_Center].x;
}

void body_slime_draw(Body* body, Body_Proportions proportions)
{
    UNUSED(proportions);
    
    f32 inner_radius = body->height * 0.25f;
    f32 particle_radius = body->height * 0.2f;
    
    CF_Aabb scissor = cf_expand_aabb(body_get_bounds(body), cf_v2(50.0f, 0.0f));
    scissor.max.y += 50.0f;
    
    for (s32 index = 1; index < Body_Slime_Count; ++index)
    {
        s32 i0 = index % Body_Slime_Count;
        s32 i2 = (index + 1) % Body_Slime_Count;
        
        if (i0 == Body_Slime_Center)
        {
            ++i0;
        }
        
        if (i2 == Body_Slime_Center)
        {
            ++i2;
        }
        
        CF_V2 p0 = body->particles[index];
        CF_V2 p1 = body->particles[Body_Slime_Center];
        CF_V2 p2 = body->particles[i2];
        
        cf_draw_tri_fill(p0, p1, p2, 10.0f);
    }
}

void body_slime_walk(Body* body, CF_V2 direction, f32 move_speed)
{
    f32 dt = s_app->world.dt;
    
    body->position.y += direction.y * move_speed * dt;
    CF_V2 offset = cf_mul_v2_f(direction,  move_speed * dt);
    
    {
        body->particles[Body_Slime_Center].x += offset.x;
        body->particles[Body_Slime_Center].y = body->position.y + body->height * 0.25f;
    }
}

// doesn't look good, but concept works
void body_slime_lunge_co(CF_Coroutine co)
{
    Body* body = (Body*)cf_coroutine_get_udata(co);
    
    World* world = &s_app->world;
    
    CF_V2 c0 = body->particles[Body_Slime_Center];
    CF_V2 c1 = c0;
    CF_V2 c2 = c0;
    
    c1.x += body->facing_direction * body->height * 1.5f;
    c1.y += body->height * 1.0f;
    c2.y = cf_max(c2.y, body->position.y + body->height * 0.5f);
    
    for (s32 index = 0; index < cf_array_count(body->constraints); ++index)
    {
        body->constraints[index].stiffness = 1.0f;
    }
    
    CF_V2 min = cf_mul_v2_f(s_app->screen_size, -0.25f);
    CF_V2 max = cf_mul_v2_f(s_app->screen_size,  0.25f);
    CF_Aabb bounds = cf_make_aabb(min, max);
    
    f32 duration = 1.0f;
    f32 delay = 0.0f;
    while (delay < duration)
    {
        for (s32 index = 0; index < cf_array_count(body->is_locked); ++index)
        {
            body->is_locked[index] = false;
        }
        
        f32 t = cf_clamp01(delay / duration);
        t = cf_quad_in(t);
        
        CF_V2 target = cf_bezier(c0, c1, c2, t);
        
        body->particles[Body_Slime_Center] = cf_lerp(body->particles[Body_Slime_Center], target, t);
        
        body_verlet(body);
        body_satisfy_constraints(body, bounds);
        
        delay += world->dt;
        
        // lock while attacking to avoid body_satisfy_constraints() from going crazy
        for (s32 index = 0; index < cf_array_count(body->is_locked); ++index)
        {
            body->is_locked[index] = true;
        }
        
        cf_coroutine_yield(co);
    }
    
    for (s32 index = 0; index < cf_array_count(body->is_locked); ++index)
    {
        body->is_locked[index] = false;
    }
    
    // set prev to current to so body_verlet() doesn't have a large delta causing each particle to get launched
    for (s32 index = 0; index < cf_array_count(body->particles); ++index)
    {
        body->prev_particles[index] = body->particles[index];
    }
    
    body_slime_reset_constraints(body);
}

void body_slime_forward_spike_co(CF_Coroutine co)
{
    Body* body = (Body*)cf_coroutine_get_udata(co);
    World* world = &s_app->world;
    
    CF_V2 attack_offset = cf_v2(body->facing_direction * body->height * 2.0f, body->height);
    b32 is_center_locked = body->is_locked[Body_Slime_Center];
    
    s32 particle_index = 0;
    {
        CF_V2 centroid = body_centeroid(body);
        f32 furthest_distance = 0.0f;
        for (s32 index = 0; index < cf_array_count(body->particles); ++index)
        {
            CF_V2 dp = cf_sub(body->particles[index], centroid);
            f32 dot = cf_dot(dp, attack_offset);
            if (dot > furthest_distance)
            {
                furthest_distance = dot;
                particle_index = index;
            }
        }
    }
    
    CF_V2 c0 = body->particles[particle_index];
    CF_V2 c1 = cf_add(c0, attack_offset);
    CF_V2 c2 = cf_sub(c0, cf_mul_v2_f(cf_safe_norm(attack_offset), body->height * 0.5f));
    
    CF_MEMSET(body->is_locked, 1, sizeof(*body->is_locked) * cf_array_count(body->is_locked));
    body->is_locked[particle_index] = false;
    
    cf_array_clear(body->hurt_particles);
    cf_array_push(body->hurt_particles, particle_index);
    cf_array_push(body->hurt_particles, Body_Slime_Center);
    
    f32 duration = 0.5f;
    f32 delay = 0.0f;
    while (delay < duration)
    {
        f32 t = cf_clamp01(delay / duration);
        t = cf_quint_in_out(t);
        CF_V2 p = cf_bezier(c0, c1, c2, t);
        
        body->particles[particle_index] = p;
        
        delay += world->dt;
        cf_coroutine_yield(co);
    }
    
    cf_array_clear(body->hurt_particles);
    
    CF_MEMSET(body->is_locked, 0, sizeof(*body->is_locked) * cf_array_count(body->is_locked));
    
    body->is_locked[Body_Slime_Center] = true;
    for (s32 index = 0; index < cf_array_count(body->particles); ++index)
    {
        f32 dy = body->particles[index].y - body->position.y;
        if (dy < 1e-7f)
        {
            body->is_locked[index] = true;
        }
    }
    
    // post delay
    duration = 1.0f;
    delay = 0.0f;
    while (delay < duration)
    {
        delay += world->dt;
        cf_coroutine_yield(co);
    }
    
    CF_MEMSET(body->is_locked, 0, sizeof(*body->is_locked) * cf_array_count(body->is_locked));
    
    body->is_locked[Body_Slime_Center] = is_center_locked;
}

// @slime

// @hand

Body make_hand_body(f32 height)
{
    Body body = make_default_body(Body_Type_Hand, Body_Hand_Count, height);
    
    body.facing_direction = 1.0f;
    body.height = height;
    body.base_height = height;
    body.time_step = 1.0f / TARGET_FRAMERATE;
    body.wheel_angle = CF_PI * 0.5f;
    
    // index
    {
        body.particles[Body_Hand_Index_Tip] = cf_v2(0, 0);
        body.particles[Body_Hand_Index_Joint] = cf_v2(height * 0.1f, 0.0f);
        body.particles[Body_Hand_Index_Base] = cf_v2(height * 0.25f, 0.0f);;
        
        body.particles[Body_Hand_Middle_Tip] = cf_v2(height * 0.35f, height * 0.25f);
        body.particles[Body_Hand_Middle_Joint] = cf_v2(height * 0.35f, height * 0.1f);
        body.particles[Body_Hand_Middle_Base] = cf_v2(height * 0.35f, 0.0f);
        
        body.particles[Body_Hand_Pinky_Tip] = cf_v2(height * 0.45f, height * 0.2f);
        body.particles[Body_Hand_Pinky_Joint] = cf_v2(height * 0.45f, height * 0.1f);
        body.particles[Body_Hand_Pinky_Base] = cf_v2(height * 0.45f, 0.0f);
        
        body.particles[Body_Hand_Thumb_Tip] = cf_v2(height * 0.15f, height * -0.25f);
        body.particles[Body_Hand_Thumb_Joint] = cf_v2(height * 0.3f, height * -0.25f);
        body.particles[Body_Hand_Thumb_Base] = cf_v2(height * 0.45f, height * -0.25f);
        
        body.particles[Body_Hand_Wrist] = cf_v2(height * 0.6f, height * -0.25f);
    }
    
    CF_MEMCPY(body.prev_particles, body.particles, sizeof(*body.particles) * cf_array_count(body.particles));
    
    Particle_Constraint constraint = { 0 };
    constraint.stiffness = 1.0f;
    
    // index finger
    {
        constraint.a = Body_Hand_Index_Tip;
        constraint.b = Body_Hand_Index_Joint;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Hand_Index_Joint;
        constraint.b = Body_Hand_Index_Base;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
    }
    
    // middle finger
    {
        constraint.a = Body_Hand_Middle_Tip;
        constraint.b = Body_Hand_Middle_Joint;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Hand_Middle_Joint;
        constraint.b = Body_Hand_Middle_Base;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
    }
    
    // pinky finger
    {
        constraint.a = Body_Hand_Pinky_Tip;
        constraint.b = Body_Hand_Pinky_Joint;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Hand_Pinky_Joint;
        constraint.b = Body_Hand_Pinky_Base;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
    }
    
    // thumb finger
    {
        constraint.a = Body_Hand_Thumb_Tip;
        constraint.b = Body_Hand_Thumb_Joint;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Hand_Thumb_Joint;
        constraint.b = Body_Hand_Thumb_Base;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
    }
    
    // palm
    {
        constraint.a = Body_Hand_Index_Base;
        constraint.b = Body_Hand_Middle_Base;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Hand_Index_Base;
        constraint.b = Body_Hand_Pinky_Base;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Hand_Middle_Base;
        constraint.b = Body_Hand_Pinky_Base;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Hand_Index_Base;
        constraint.b = Body_Hand_Thumb_Base;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Hand_Thumb_Base;
        constraint.b = Body_Hand_Wrist;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Hand_Pinky_Base;
        constraint.b = Body_Hand_Wrist;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Hand_Index_Base;
        constraint.b = Body_Hand_Wrist;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Hand_Thumb_Base;
        constraint.b = Body_Hand_Pinky_Base;
        constraint.length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.base_length = constraint.length;
        cf_array_push(body.constraints, constraint);
    }
    
    return body;
}

void body_hand_stabilize(Body* body)
{
    
}

void body_hand_draw(Body* body, Body_Proportions proportions)
{
    UNUSED(proportions);
    
    f32 tip_thickness = body->height * 0.05f;
    f32 joint_thickness = body->height * 0.075f;
    f32 base_thickness = body->height * 0.1f;
    
    CF_Poly palm = { 0 };
    palm.count = 5;
    palm.verts[0] = body->particles[Body_Hand_Index_Base];
    palm.verts[1] = body->particles[Body_Hand_Middle_Base];
    palm.verts[2] = body->particles[Body_Hand_Pinky_Base];
    palm.verts[3] = body->particles[Body_Hand_Wrist];
    palm.verts[4] = body->particles[Body_Hand_Thumb_Base];
    
    cf_make_poly(&palm);
    
    f32 palm_chubbiness = body->height * 0.05f;
    
    CF_Color upper_color = cf_draw_peek_color();
    CF_Color lower_color = upper_color;
    upper_color = cf_color_lerp(upper_color, cf_color_black(), 0.025f);
    lower_color = cf_color_lerp(lower_color, cf_color_black(), 0.05f);
    
    cf_draw_push_color(lower_color);
    body_draw_line(body->particles[Body_Hand_Pinky_Joint], body->particles[Body_Hand_Pinky_Tip], joint_thickness, tip_thickness, 4);
    
    body_draw_line(body->particles[Body_Hand_Middle_Joint], body->particles[Body_Hand_Middle_Tip], joint_thickness, tip_thickness, 4);
    
    body_draw_line(body->particles[Body_Hand_Index_Joint], body->particles[Body_Hand_Index_Tip], joint_thickness, tip_thickness, 4);
    
    body_draw_line(body->particles[Body_Hand_Thumb_Joint], body->particles[Body_Hand_Thumb_Tip], joint_thickness, tip_thickness, 4);
    cf_draw_pop_color();
    
    
    cf_draw_push_color(upper_color);
    body_draw_line(body->particles[Body_Hand_Pinky_Base], body->particles[Body_Hand_Pinky_Joint], base_thickness, joint_thickness, 4);
    
    cf_draw_push_color(upper_color);
    body_draw_line(body->particles[Body_Hand_Middle_Base], body->particles[Body_Hand_Middle_Joint], base_thickness, joint_thickness, 4);
    
    cf_draw_push_color(upper_color);
    body_draw_line(body->particles[Body_Hand_Index_Base], body->particles[Body_Hand_Index_Joint], base_thickness, joint_thickness, 4);
    
    cf_draw_push_color(upper_color);
    body_draw_line(body->particles[Body_Hand_Thumb_Base], body->particles[Body_Hand_Thumb_Joint], base_thickness, joint_thickness, 4);
    
    cf_draw_pop_color();
    
    body_draw_polygon_fill(palm, palm_chubbiness);
}

void body_hand_move(Body* body, CF_V2 position)
{
    CF_V2 p0 = position;
    CF_V2 p1 = cf_v2(p0.x + body->height * 0.15f, p0.y + body->height * -0.25f);
    CF_V2 p2 = cf_v2(p0.x + body->height * 0.2f, p0.y + body->height * -0.2f);
    CF_V2 p3 = cf_v2(p0.x, p0.y + body->height * -0.4f);
    
    CF_V2 b0 = cf_v2(position.x + body->height * 0.45f, position.y + body->height * -0.2f);
    CF_V2 b1 = cf_v2(position.x + body->height * 0.45f, position.y + body->height * -0.45f);
    CF_V2 wrist = cf_v2(position.x + body->height * 0.6f, position.y + body->height * -0.45f);
    
    body->particles[Body_Hand_Index_Tip] = p0;
    
    body->particles[Body_Hand_Pinky_Base] = b0;
    body->particles[Body_Hand_Thumb_Base] = b1;
    body->particles[Body_Hand_Wrist] = wrist;
    
    body->particles[Body_Hand_Middle_Tip] = p1;
    body->particles[Body_Hand_Pinky_Tip] = p2;
    body->particles[Body_Hand_Thumb_Tip] = p3;
}

void body_hand_pinch(Body* body)
{
    CF_V2 position = body->particles[Body_Hand_Index_Tip];
    CF_V2 b0 = body->particles[Body_Hand_Index_Base];
    b0.y = position.y;
    CF_V2 b1 = cf_v2(b0.x, b0.y + body->height * -0.25f);;
    CF_V2 b2 = cf_v2(b0.x + body->height * 0.25f, b0.y + body->height * -0.25f);;
    
    body->particles[Body_Hand_Thumb_Tip] = position;
    body->particles[Body_Hand_Index_Base] = b0;
    body->particles[Body_Hand_Thumb_Base] = b1;
    body->particles[Body_Hand_Wrist] = b2;
}

void body_hand_gesture(Body* body)
{
    body->particles[Body_Hand_Thumb_Tip] = body->particles[Body_Hand_Middle_Tip];
}

void body_hand_slap_begin(Body* body)
{
    
    body->is_locked[Body_Hand_Index_Base] = true;
    body->is_locked[Body_Hand_Middle_Base] = true;
    body->is_locked[Body_Hand_Pinky_Base] = true;
    body->is_locked[Body_Hand_Thumb_Base] = true;
    body->is_locked[Body_Hand_Wrist] = true;
}

void body_hand_slap_end(Body* body)
{
    
    body->is_locked[Body_Hand_Index_Base] = false;;
    body->is_locked[Body_Hand_Middle_Base] = false;;
    body->is_locked[Body_Hand_Pinky_Base] = false;;
    body->is_locked[Body_Hand_Thumb_Base] = false;;
    body->is_locked[Body_Hand_Wrist] = false;;
}

void body_hand_slap_co(CF_Coroutine co)
{
    Body* body = (Body*)cf_coroutine_get_udata(co);
    World* world = &s_app->world;
    
    // index
    CF_V2 i0 = s_app->input.world_position;
    CF_V2 i1 = i0;
    CF_V2 i2 = i0;
    i0.x += body->height * -0.25f;
    i0.y += body->height * -0.25f;
    i2.x += body->height * 0.5f;
    
    // middle
    CF_V2 m0 = i0;
    CF_V2 m1 = i1;
    CF_V2 m2 = i2;
    m0.y += body->height * -0.1f;
    m1.y += body->height * -0.1f;
    m2.y += body->height * -0.1f;
    
    // pinky
    CF_V2 p0 = i0;
    CF_V2 p1 = i1;
    CF_V2 p2 = i2;
    p0.y += body->height * -0.15f;
    p1.y += body->height * -0.15f;
    p2.y += body->height * -0.15f;
    
    // index base
    CF_V2 ib_0 = body->particles[Body_Hand_Index_Base];
    CF_V2 ib_1 = ib_0;
    CF_V2 ib_2 = ib_0;
    ib_1.x += body->height * 0.2f;
    ib_1.y += body->height * 0.05f;
    ib_1.x += body->height * 0.4f;
    ib_1.y += body->height * 0.05f;
    
    // middle base
    CF_V2 mb_0 = ib_0;
    CF_V2 mb_1 = ib_1;
    CF_V2 mb_2 = ib_2;
    mb_0.y += body->height * -0.1f;
    mb_1.y += body->height * -0.1f;
    mb_2.y += body->height * -0.1f;
    
    // pinky base
    CF_V2 pb_0 = ib_0;
    CF_V2 pb_1 = ib_1;
    CF_V2 pb_2 = ib_2;
    pb_0.y += body->height * -0.2f;
    pb_1.y += body->height * -0.2f;
    pb_2.y += body->height * -0.2f;
    
    body_hand_slap_begin(body);
    
    // pre slap
    f32 duration = 0.2f;
    f32 delay = 0.0f;
    while (delay < duration)
    {
        f32 t = cf_clamp01(delay / duration);
        
        body->particles[Body_Hand_Index_Tip] = cf_lerp(body->particles[Body_Hand_Index_Tip], i0, t);
        body->particles[Body_Hand_Middle_Tip] = cf_lerp(body->particles[Body_Hand_Middle_Tip], m0, t);
        body->particles[Body_Hand_Pinky_Tip] = cf_lerp(body->particles[Body_Hand_Pinky_Tip], p0, t);
        
        body->particles[Body_Hand_Thumb_Tip] = cf_lerp(body->particles[Body_Hand_Thumb_Tip], body->particles[Body_Hand_Index_Base], t);
        
        body->particles[Body_Hand_Index_Base] = cf_lerp(body->particles[Body_Hand_Index_Base], ib_0, t);
        body->particles[Body_Hand_Middle_Base] = cf_lerp(body->particles[Body_Hand_Middle_Base], mb_0, t);
        body->particles[Body_Hand_Pinky_Base] = cf_lerp(body->particles[Body_Hand_Pinky_Base], pb_0, t);
        
        body->particles[Body_Hand_Wrist] = cf_lerp(body->particles[Body_Hand_Wrist], body->particles[Body_Hand_Thumb_Base], t);
        
        delay += world->dt;
        cf_coroutine_yield(co);
    }
    
    // slap wing
    duration = 0.2f;
    delay = 0.0f;
    while (delay < duration)
    {
        f32 t = cf_clamp01(delay / duration);
        t = cf_smoothstep(t);
        
        CF_V2 i_t = cf_bezier(i0, i1, i2, t);
        CF_V2 m_t = cf_bezier(m0, m1, m2, t);
        CF_V2 p_t = cf_bezier(p0, p1, p2, t);
        
        CF_V2 ib_t = cf_bezier(ib_0, ib_1, ib_2, t);
        CF_V2 mb_t = cf_bezier(mb_0, mb_1, mb_2, t);
        CF_V2 pb_t = cf_bezier(pb_0, pb_1, pb_2, t);
        
        body->particles[Body_Hand_Index_Tip] = i_t;
        body->particles[Body_Hand_Middle_Tip] = m_t;
        body->particles[Body_Hand_Pinky_Tip] = p_t;
        body->particles[Body_Hand_Thumb_Tip] = body->particles[Body_Hand_Index_Base];
        
        body->particles[Body_Hand_Index_Base] = ib_t;
        body->particles[Body_Hand_Middle_Base] = mb_t;
        body->particles[Body_Hand_Pinky_Base] = pb_t;
        
        delay += world->dt;
        cf_coroutine_yield(co);
    }
    
    // slap hits
    {
        CF_Aabb slap_bounds = body_get_bounds(body);
        fixed ecs_entity_t* queries = spatial_map_query(&world->spatial_map, BIT(TOY_SLAP_INDEX), slap_bounds);
        for (s32 index = 0; index < cf_array_count(queries); ++index)
        {
            if (ECS_HAS(queries[index], C_Slappable))
            {
                C_Puppet* puppet = ECS_GET(queries[index], C_Puppet);
                CF_V2 force = cf_v2(-puppet->body.facing_direction * 15000.0f, 15000.0f);
                body_slap(&puppet->body, force);
                
                CF_Aabb slap_region = body_get_bounds(&puppet->body);
                slap_region = cf_clamp_aabb(slap_region, slap_bounds);
                make_event_hand_slap_hit(queries[index], slap_region);
            }
        }
    }
    
    // post slap
    duration = 0.2f;
    delay = 0.0f;
    while (delay < duration)
    {
        f32 t = cf_clamp01(delay / duration);
        
        body->particles[Body_Hand_Index_Tip] = cf_lerp(body->particles[Body_Hand_Index_Tip], i0, t);
        body->particles[Body_Hand_Middle_Tip] = cf_lerp(body->particles[Body_Hand_Middle_Tip], m0, t);
        body->particles[Body_Hand_Pinky_Tip] = cf_lerp(body->particles[Body_Hand_Pinky_Tip], p0, t);
        
        body->particles[Body_Hand_Thumb_Tip] = cf_lerp(body->particles[Body_Hand_Index_Base], p0, t);
        
        body->particles[Body_Hand_Index_Base] = cf_lerp(body->particles[Body_Hand_Index_Base], ib_0, t);
        body->particles[Body_Hand_Middle_Base] = cf_lerp(body->particles[Body_Hand_Middle_Base], mb_0, t);
        body->particles[Body_Hand_Pinky_Base] = cf_lerp(body->particles[Body_Hand_Pinky_Base], pb_0, t);
        
        delay += world->dt;
        cf_coroutine_yield(co);
    }
    
    body_hand_slap_end(body);
}

void body_hand_cast(Body* body)
{
    CF_V2 position = body->particles[Body_Hand_Index_Tip];
    
    CF_V2 index_base = cf_v2(body->height * -0.05f, body->height * -0.3f);
    CF_V2 middle_tip = cf_v2(body->height * 0.15f, 0.0f);
    CF_V2 middle_base = cf_v2(body->height * 0.15f, body->height * -0.3f);
    CF_V2 pinky_tip = cf_v2(body->height * 0.25f, body->height * 0.0f);
    CF_V2 pinky_base = cf_v2(body->height * 0.25f, body->height * -0.3f);
    CF_V2 thumb_tip = cf_v2(body->height * -0.15f, body->height * -0.25f);
    CF_V2 thumb_base = cf_v2(body->height * -0.05f, body->height * -0.5f);
    CF_V2 wrist = cf_v2(body->height * 0.5f, body->height * -0.5f);
    
    body->particles[Body_Hand_Index_Base] = cf_add(position, index_base);
    
    body->particles[Body_Hand_Middle_Tip] = cf_add(position, middle_tip);
    body->particles[Body_Hand_Middle_Base] = cf_add(position, middle_base);
    
    body->particles[Body_Hand_Pinky_Tip] = cf_add(position, pinky_tip);
    body->particles[Body_Hand_Pinky_Base] = cf_add(position, pinky_base);
    
    body->particles[Body_Hand_Thumb_Tip] = cf_add(position, thumb_tip);
    body->particles[Body_Hand_Thumb_Base] = cf_add(position, thumb_base);
    
    body->particles[Body_Hand_Wrist] = cf_add(position, wrist);
}

// @hand

// @tubeman

Body make_tubeman_body(f32 height)
{
    Body body = make_default_body(Body_Type_Tubeman, Body_Tubeman_Count, height);
    
    body.facing_direction = 1.0f;
    body.height = height;
    body.base_height = height;
    body.time_step = 1.0f / TARGET_FRAMERATE;
    body.wheel_angle = CF_PI * 0.5f;
    
    f32 width = height * 0.15f;
    f32 half_width = width * 0.5f;
    
    f32 segment_height = height / 5;
    f32 arm_length = height * 0.75f;
    
    body.particles[Body_Tubeman_Segment_Left_0] = cf_v2(-half_width, 0 * segment_height);
    body.particles[Body_Tubeman_Segment_Right_0] = cf_v2(half_width, 0 * segment_height);
    body.particles[Body_Tubeman_Segment_Left_1] = cf_v2(-half_width, 1 * segment_height);
    body.particles[Body_Tubeman_Segment_Right_1] = cf_v2(half_width, 1 * segment_height);
    body.particles[Body_Tubeman_Segment_Left_2] = cf_v2(-half_width, 2 * segment_height);
    body.particles[Body_Tubeman_Segment_Right_2] = cf_v2(half_width, 2 * segment_height);
    body.particles[Body_Tubeman_Segment_Left_3] = cf_v2(-half_width, 3 * segment_height);
    body.particles[Body_Tubeman_Segment_Right_3] = cf_v2(half_width, 3 * segment_height);
    body.particles[Body_Tubeman_Segment_Left_4] = cf_v2(-half_width, 4 * segment_height);
    body.particles[Body_Tubeman_Segment_Right_4] = cf_v2(half_width, 4 * segment_height);
    body.particles[Body_Tubeman_Segment_Left_5] = cf_v2(-half_width, 5 * segment_height);
    body.particles[Body_Tubeman_Segment_Right_5] = cf_v2(half_width, 5 * segment_height);
    
    body.particles[Body_Tubeman_Left_Shoulder] = body.particles[Body_Tubeman_Segment_Left_3];
    body.particles[Body_Tubeman_Right_Shoulder] = body.particles[Body_Tubeman_Segment_Right_3];
    
    body.particles[Body_Tubeman_Left_Elbow] = body.particles[Body_Tubeman_Left_Shoulder];
    body.particles[Body_Tubeman_Left_Hand] = body.particles[Body_Tubeman_Left_Shoulder];
    body.particles[Body_Tubeman_Left_Elbow].x -= arm_length * 0.5f;
    body.particles[Body_Tubeman_Left_Hand].x -= arm_length;
    
    body.particles[Body_Tubeman_Right_Elbow] = body.particles[Body_Tubeman_Right_Shoulder];
    body.particles[Body_Tubeman_Right_Hand] = body.particles[Body_Tubeman_Right_Shoulder];
    body.particles[Body_Tubeman_Right_Elbow].x += arm_length * 0.5f;
    body.particles[Body_Tubeman_Right_Hand].x += arm_length;
    
    f32 hair_cursor_x = -half_width + width / 3;
    f32 hair_segment_height = height * 0.05f;
    
    body.particles[Body_Tubeman_Hair_0_0] = cf_v2(hair_cursor_x, 5 * segment_height);
    body.particles[Body_Tubeman_Hair_0_1] = cf_v2(hair_cursor_x, 5 * segment_height);
    body.particles[Body_Tubeman_Hair_0_2] = cf_v2(hair_cursor_x, 5 * segment_height);
    body.particles[Body_Tubeman_Hair_0_1].y += hair_segment_height;
    body.particles[Body_Tubeman_Hair_0_2].y += hair_segment_height * 2;
    hair_cursor_x += width / 6;
    
    body.particles[Body_Tubeman_Hair_1_0] = cf_v2(hair_cursor_x, 5 * segment_height);
    body.particles[Body_Tubeman_Hair_1_1] = cf_v2(hair_cursor_x, 5 * segment_height);
    body.particles[Body_Tubeman_Hair_1_2] = cf_v2(hair_cursor_x, 5 * segment_height);
    body.particles[Body_Tubeman_Hair_1_1].y += hair_segment_height;
    body.particles[Body_Tubeman_Hair_1_2].y += hair_segment_height * 2;
    hair_cursor_x += width / 6;
    
    body.particles[Body_Tubeman_Hair_2_0] = cf_v2(hair_cursor_x, 5 * segment_height);
    body.particles[Body_Tubeman_Hair_2_1] = cf_v2(hair_cursor_x, 5 * segment_height);
    body.particles[Body_Tubeman_Hair_2_2] = cf_v2(hair_cursor_x, 5 * segment_height);
    body.particles[Body_Tubeman_Hair_2_1].y += hair_segment_height;
    body.particles[Body_Tubeman_Hair_2_2].y += hair_segment_height * 2;
    
    CF_MEMCPY(body.prev_particles, body.particles, sizeof(*body.particles) * cf_array_count(body.particles));
    
    Particle_Constraint constraint = { 0 };
    constraint.stiffness = 0.1f;
    // segments
    {
        constraint.a = Body_Tubeman_Segment_Left_0;
        constraint.b = Body_Tubeman_Segment_Right_0;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Segment_Left_0;
        constraint.b = Body_Tubeman_Segment_Left_1;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Segment_Right_0;
        constraint.b = Body_Tubeman_Segment_Right_1;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
    }
    {
        constraint.a = Body_Tubeman_Segment_Left_1;
        constraint.b = Body_Tubeman_Segment_Right_1;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Segment_Left_1;
        constraint.b = Body_Tubeman_Segment_Left_2;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Segment_Right_1;
        constraint.b = Body_Tubeman_Segment_Right_2;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
    }
    {
        constraint.a = Body_Tubeman_Segment_Left_2;
        constraint.b = Body_Tubeman_Segment_Right_2;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Segment_Left_2;
        constraint.b = Body_Tubeman_Segment_Left_3;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Segment_Right_2;
        constraint.b = Body_Tubeman_Segment_Right_3;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
    }
    {
        constraint.a = Body_Tubeman_Segment_Left_3;
        constraint.b = Body_Tubeman_Segment_Right_3;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Segment_Left_3;
        constraint.b = Body_Tubeman_Segment_Left_4;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Segment_Right_3;
        constraint.b = Body_Tubeman_Segment_Right_4;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
    }
    {
        constraint.a = Body_Tubeman_Segment_Left_4;
        constraint.b = Body_Tubeman_Segment_Right_4;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Segment_Left_4;
        constraint.b = Body_Tubeman_Segment_Left_5;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Segment_Right_4;
        constraint.b = Body_Tubeman_Segment_Right_5;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
    }
    {
        constraint.a = Body_Tubeman_Segment_Left_5;
        constraint.b = Body_Tubeman_Segment_Right_5;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
    }
    
    // spine
    {
        constraint.a = Body_Tubeman_Segment_Left_0;
        constraint.b = Body_Tubeman_Segment_Right_1;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Segment_Right_0;
        constraint.b = Body_Tubeman_Segment_Left_1;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Segment_Left_1;
        constraint.b = Body_Tubeman_Segment_Right_2;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Segment_Right_1;
        constraint.b = Body_Tubeman_Segment_Left_2;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Segment_Left_2;
        constraint.b = Body_Tubeman_Segment_Right_3;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Segment_Right_2;
        constraint.b = Body_Tubeman_Segment_Left_3;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Segment_Left_3;
        constraint.b = Body_Tubeman_Segment_Right_4;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Segment_Right_3;
        constraint.b = Body_Tubeman_Segment_Left_4;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Segment_Left_4;
        constraint.b = Body_Tubeman_Segment_Right_5;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Segment_Right_4;
        constraint.b = Body_Tubeman_Segment_Left_5;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
    }
    
    constraint.stiffness = 1.0f;
    // hair left
    {
        constraint.a = Body_Tubeman_Hair_0_0;
        constraint.b = Body_Tubeman_Hair_0_1;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Hair_0_1;
        constraint.b = Body_Tubeman_Hair_0_2;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
    }
    // hair middle
    {
        constraint.a = Body_Tubeman_Hair_1_0;
        constraint.b = Body_Tubeman_Hair_1_1;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Hair_1_1;
        constraint.b = Body_Tubeman_Hair_1_2;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
    }
    // hair right
    {
        constraint.a = Body_Tubeman_Hair_2_0;
        constraint.b = Body_Tubeman_Hair_2_1;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Hair_2_1;
        constraint.b = Body_Tubeman_Hair_2_2;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
    }
    // scalp
    {
        constraint.a = Body_Tubeman_Segment_Left_5;
        constraint.b = Body_Tubeman_Hair_0_0;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Hair_0_0;
        constraint.b = Body_Tubeman_Hair_1_0;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Hair_1_0;
        constraint.b = Body_Tubeman_Hair_2_0;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Hair_2_0;
        constraint.b = Body_Tubeman_Segment_Right_5;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
    }
    
    // shoulder sockets
    {
        constraint.a = Body_Tubeman_Left_Shoulder;
        constraint.b = Body_Tubeman_Segment_Left_3;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Right_Shoulder;
        constraint.b = Body_Tubeman_Segment_Right_3;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
    }
    
    // left arm
    {
        constraint.a = Body_Tubeman_Left_Shoulder;
        constraint.b = Body_Tubeman_Left_Elbow;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Left_Elbow;
        constraint.b = Body_Tubeman_Left_Hand;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
    }
    // right arm
    {
        constraint.a = Body_Tubeman_Right_Shoulder;
        constraint.b = Body_Tubeman_Right_Elbow;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
        
        constraint.a = Body_Tubeman_Right_Elbow;
        constraint.b = Body_Tubeman_Right_Hand;
        constraint.base_length = cf_distance(body.particles[constraint.a], body.particles[constraint.b]);
        constraint.length = constraint.base_length;
        cf_array_push(body.constraints, constraint);
    }
    
    return body;
}

void body_tubeman_stabilize(Body* body)
{
    f32 gravity = -400.0f;
    
    for (s32 index = 0; index < Body_Tubeman_Left_Elbow / 2; ++index)
    {
        s32 left_index = index;
        s32 right_index = index + 1;
        CF_V2 left = body->particles[left_index];
        CF_V2 right = body->particles[right_index];
        
        CF_V2 dp = cf_sub(left, right);
        
        body->accelerations[left_index] = cf_add(body->accelerations[left_index], dp);
        body->accelerations[right_index] = cf_sub(body->accelerations[right_index], dp);
    }
    
    for (s32 index = 0; index < Body_Tubeman_Left_Elbow; ++index)
    {
        body->accelerations[index].y += gravity;
    }
}

void body_tubeman_draw(Body* body, Body_Proportions proportions)
{
    f32 chubbiness = 5.0f * proportions.torso_chubbiness;
    
    f32 upper_arm_thickness = body->height * 0.10f * proportions.upper_arm_thickness;
    f32 lower_arm_thickness = body->height * 0.06f * proportions.lower_arm_thickness;
    f32 hand_thickness = body->height * 0.045f * proportions.hand_thickness;
    f32 hair_thickness = body->height * 0.01f;
    
    s32 iterations = 16;
    
    CF_Color start_color = cf_draw_peek_color();
    CF_Color end_color = cf_color_black();
    end_color.a = start_color.a * 0.5f;
    
    CF_Color upper_color = cf_color_lerp(start_color, end_color, 0.025f);
    CF_Color lower_color = cf_color_lerp(start_color, end_color, 0.05f);
    
    CF_Poly chunk = { 0 };
    
    cf_draw_line(body->particles[Body_Tubeman_Hair_0_0], body->particles[Body_Tubeman_Hair_0_1], hair_thickness);
    cf_draw_line(body->particles[Body_Tubeman_Hair_0_1], body->particles[Body_Tubeman_Hair_0_2], hair_thickness);
    
    cf_draw_line(body->particles[Body_Tubeman_Hair_1_0], body->particles[Body_Tubeman_Hair_1_1], hair_thickness);
    cf_draw_line(body->particles[Body_Tubeman_Hair_1_1], body->particles[Body_Tubeman_Hair_1_2], hair_thickness);
    
    cf_draw_line(body->particles[Body_Tubeman_Hair_2_0], body->particles[Body_Tubeman_Hair_2_1], hair_thickness);
    cf_draw_line(body->particles[Body_Tubeman_Hair_2_1], body->particles[Body_Tubeman_Hair_2_2], hair_thickness);
    
    // first chunk
    chunk.count = 4;
    chunk.verts[0] = body->particles[Body_Tubeman_Segment_Left_0];
    chunk.verts[1] = body->particles[Body_Tubeman_Segment_Right_0];
    chunk.verts[2] = body->particles[Body_Tubeman_Segment_Right_1];
    chunk.verts[3] = body->particles[Body_Tubeman_Segment_Left_1];
    cf_make_poly(&chunk);
    body_draw_polygon_fill(chunk, chubbiness);
    
    // second chunk
    chunk.count = 4;
    chunk.verts[0] = body->particles[Body_Tubeman_Segment_Left_1];
    chunk.verts[1] = body->particles[Body_Tubeman_Segment_Right_1];
    chunk.verts[2] = body->particles[Body_Tubeman_Segment_Right_2];
    chunk.verts[3] = body->particles[Body_Tubeman_Segment_Left_2];
    cf_make_poly(&chunk);
    body_draw_polygon_fill(chunk, chubbiness);
    
    // third chunk
    chunk.count = 4;
    chunk.verts[0] = body->particles[Body_Tubeman_Segment_Left_2];
    chunk.verts[1] = body->particles[Body_Tubeman_Segment_Right_2];
    chunk.verts[2] = body->particles[Body_Tubeman_Segment_Right_3];
    chunk.verts[3] = body->particles[Body_Tubeman_Segment_Left_3];
    cf_make_poly(&chunk);
    body_draw_polygon_fill(chunk, chubbiness);
    
    // fourth chunk
    chunk.count = 4;
    chunk.verts[0] = body->particles[Body_Tubeman_Segment_Left_3];
    chunk.verts[1] = body->particles[Body_Tubeman_Segment_Right_3];
    chunk.verts[2] = body->particles[Body_Tubeman_Segment_Right_4];
    chunk.verts[3] = body->particles[Body_Tubeman_Segment_Left_4];
    cf_make_poly(&chunk);
    body_draw_polygon_fill(chunk, chubbiness);
    
    // fifth chunk
    chunk.count = 4;
    chunk.verts[0] = body->particles[Body_Tubeman_Segment_Left_4];
    chunk.verts[1] = body->particles[Body_Tubeman_Segment_Right_4];
    chunk.verts[2] = body->particles[Body_Tubeman_Segment_Right_5];
    chunk.verts[3] = body->particles[Body_Tubeman_Segment_Left_5];
    cf_make_poly(&chunk);
    body_draw_polygon_fill(chunk, chubbiness);
    
    cf_draw_push_color(lower_color);
    body_draw_line(body->particles[Body_Tubeman_Left_Elbow], body->particles[Body_Tubeman_Left_Hand], lower_arm_thickness, hand_thickness, iterations);
    body_draw_line(body->particles[Body_Tubeman_Right_Elbow], body->particles[Body_Tubeman_Right_Hand], lower_arm_thickness, hand_thickness, iterations);
    cf_draw_pop_color();
    
    cf_draw_push_color(upper_color);
    body_draw_line(body->particles[Body_Tubeman_Left_Shoulder], body->particles[Body_Tubeman_Left_Elbow], upper_arm_thickness, lower_arm_thickness, iterations);
    body_draw_line(body->particles[Body_Tubeman_Right_Shoulder], body->particles[Body_Tubeman_Right_Elbow], upper_arm_thickness, lower_arm_thickness, iterations);
    cf_draw_pop_color();
}

void body_tubeman_sway(Body* body, CF_V2 force)
{
    for (s32 index = 0; index < Body_Tubeman_Left_Elbow; ++index)
    {
        body->accelerations[index] = cf_add(body->accelerations[index], force);
    }
    
    s32 hair_indices[] =
    {
        Body_Tubeman_Hair_0_2,
        Body_Tubeman_Hair_1_2,
        Body_Tubeman_Hair_2_2,
    };
    
    for (s32 index = 0; index < CF_ARRAY_SIZE(hair_indices); ++index)
    {
        body->accelerations[index] = cf_add(body->accelerations[index], force);
    }
}

void body_tubeman_punch_co(CF_Coroutine co)
{
    Body* body = (Body*)cf_coroutine_get_udata(co);
    World* world = &s_app->world;
    
    s32 hand_index = Body_Tubeman_Left_Hand;
    s32 elbow_index = Body_Tubeman_Left_Elbow;
    s32 shoulder_index = Body_Tubeman_Left_Shoulder;
    if (body->facing_direction < 0)
    {
        hand_index = Body_Tubeman_Right_Hand;
        elbow_index = Body_Tubeman_Right_Hand;
        shoulder_index = Body_Tubeman_Right_Shoulder;
    }
    
    CF_V2 centeroid = body_centeroid(body);
    CF_V2 old_hand_position = body->particles[hand_index];
    CF_V2 hit_target = cf_v2(centeroid.x + body->facing_direction * body->height * 3, old_hand_position.y);
    
    if (cf_coroutine_bytes_pushed(co) >= sizeof(Body))
    {
        Body target_body;
        if (cf_coroutine_pop(co, &target_body, sizeof(Body)).code == CF_RESULT_SUCCESS)
        {
            CF_V2 p0 = cf_v2(body->particles[hand_index].x, body->particles[shoulder_index].y);
            CF_V2 p1 = cf_v2(p0.x + body->facing_direction * body->height * 2.0f, p0.y);
            
            CF_V2 target_centroid = body_centeroid(&target_body);
            CF_V2 hit_direction = cf_sub(target_centroid, p0);
            hit_direction = cf_safe_norm(hit_direction);
            p1 = cf_add(p0, cf_mul_v2_f(hit_direction, body->height * 2.0f));
            
            hit_target = body_human_get_punch_target(body, &target_body);
        }
    }
    else
    {
        hit_target = body_human_get_punch_target(body, NULL);
    }
    
    CF_V2 c0 = cf_v2(centeroid.x - body->facing_direction * body->height, old_hand_position.y);
    CF_V2 c1 = hit_target;
    CF_V2 c2 = old_hand_position;
    
    cf_array_clear(body->hurt_particles);
    cf_array_push(body->hurt_particles, hand_index);
    cf_array_push(body->hurt_particles, elbow_index);
    cf_array_push(body->hurt_particles, shoulder_index);
    
    f32 duration = 1.0f;
    f32 delay = 0.0f;
    while (delay < duration)
    {
        f32 t = cf_clamp01(delay / duration);
        t = cf_circle_in_out(t);
        
        CF_V2 hand_target = cf_bezier(c0, c1, c2, t);
        
        body->particles[hand_index] = cf_lerp(body->particles[hand_index], hand_target, t);
        delay += world->dt;
        cf_coroutine_yield(co);
    }
    
    cf_array_clear(body->hurt_particles);
}

void body_tubeman_pull_co(CF_Coroutine co)
{
    Body* body = (Body*)cf_coroutine_get_udata(co);
    World* world = &s_app->world;
    
    ecs_entity_t victim = (ecs_entity_t){ -1 };
    
    if (cf_coroutine_bytes_pushed(co) >= sizeof(victim))
    {
        if (cf_coroutine_pop(co, &victim, sizeof(victim)).code != CF_RESULT_SUCCESS)
        {
            return;
        }
    }
    
    s32 hand_index = Body_Tubeman_Left_Hand;
    s32 elbow_index = Body_Tubeman_Left_Elbow;
    s32 shoulder_index = Body_Tubeman_Left_Shoulder;
    if (body->facing_direction < 0)
    {
        hand_index = Body_Tubeman_Right_Hand;
        elbow_index = Body_Tubeman_Right_Hand;
        shoulder_index = Body_Tubeman_Right_Shoulder;
    }
    
    f32 forearm_constraint_stiffness = 1.0f;
    s32 forearm_constraint_index = 0;
    for (s32 index = 0; index < cf_array_count(body->constraints); ++index)
    {
        Particle_Constraint constraint = body->constraints[index];
        if ((constraint.a == hand_index || constraint.a == hand_index) && 
            (constraint.a == elbow_index || constraint.a == elbow_index))
        {
            forearm_constraint_index = index;
            forearm_constraint_stiffness = constraint.stiffness;
            break;
        }
    }
    
    // spinning hand charge
    f32 charge_radius = body->height * 0.25f;
    CF_V2 charge_position = body->particles[shoulder_index];
    charge_position.x -= body->facing_direction * body->height * 0.5f;
    
    f32 duration = 2.0f;
    f32 delay = 0.0f;
    while (delay < duration && is_entity_alive(victim))
    {
        f32 angle = delay * CF_TAU * 5.0f;
        CF_V2 offset = cf_v2(charge_radius * cf_cos(angle), charge_radius * cf_sin(angle));
        
        body->particles[hand_index] = cf_add(charge_position, offset);
        
        delay += world->dt;
        cf_coroutine_yield(co);
    }
    
    // need to check if victim is valid in every call incase it gets destroyed at any point
    if (!is_entity_alive(victim))
    {
        return;
    }
    
    C_Puppet* victim_puppet = ECS_GET(victim, C_Puppet);
    CF_V2* target_position = &victim_puppet->body.position;
    
    CF_V2 direction = cf_sub(*target_position, body->particles[shoulder_index]);
    direction = cf_safe_norm(direction);
    CF_V2 c0 = body->particles[hand_index];
    CF_V2 c1 = cf_add(body->particles[shoulder_index], cf_mul_v2_f(direction, body->height));
    
    // throw arm forward
    duration = 1.0f;
    delay = 0.0f;
    while (delay < duration && is_entity_alive(victim))
    {
        f32 t = cf_quint_in_out(cf_clamp01(delay / duration));
        body->particles[hand_index] = cf_lerp(c0, *target_position, t);
        
        // allow forearm stretch
        f32 distance = cf_distance(body->particles[hand_index], body->particles[elbow_index]);
        if (distance > body->constraints[forearm_constraint_index].length)
        {
            body->constraints[forearm_constraint_index].stiffness = 0.0f;
        }
        
        delay += world->dt;
        cf_coroutine_yield(co);
    }
    
    c0 = body->particles[hand_index];
    c1 = body->position;
    
    // pull back hand
    duration = 1.0f;
    delay = 0.0f;
    while (delay < duration && is_entity_alive(victim))
    {
        f32 t = cf_quint_in(cf_clamp01(delay / duration));
        body->particles[hand_index] = cf_lerp(c0, c1, t);
        
        // disable forearm stretch to have some momentum body swing
        f32 distance = cf_distance(body->particles[hand_index], body->particles[elbow_index]);
        if (distance <= body->constraints[forearm_constraint_index].length)
        {
            body->constraints[forearm_constraint_index].stiffness = forearm_constraint_stiffness;
        }
        
        body_teleport(&victim_puppet->body, body->particles[hand_index]);
        
        delay += world->dt;
        cf_coroutine_yield(co);
    }
    
    // disable forearm stretch
    body->constraints[forearm_constraint_index].stiffness = forearm_constraint_stiffness;
}

// @tubeman

// utils

// body can be squished so try to at least still draw something
void body_draw_polygon_fill(CF_Poly poly, f32 chubbiness)
{
    if (poly.count > 2)
    {
        cf_draw_polygon_fill(poly.verts, poly.count, chubbiness);
    }
    else if (poly.count == 2)
    {
        cf_draw_line(poly.verts[0], poly.verts[1], chubbiness);
    }
    else
    {
        cf_draw_circle_fill2(poly.verts[0], chubbiness);
    }
}

void body_draw_line(CF_V2 p0, CF_V2 p1, f32 p0_thickness, f32 p1_thickness, s32 iterations)
{
    CF_V2 direction = cf_sub(p1, p0);
    f32 length = cf_len(direction);
    f32 segment_length = length / iterations;
    CF_V2 segment = cf_mul_v2_f(cf_safe_norm(direction), segment_length);
    
    CF_V2 start = p0;
    CF_V2 end = p0;
    for (s32 index = 0; index < iterations; ++index)
    {
        f32 t = (f32)index / iterations;
        //t = cf_quint_in_out(t);
        t = cf_back_in_out(t);
        
        f32 thickness = cf_lerp(p0_thickness, p1_thickness, t);
        end = cf_add(start, segment);
        
        cf_draw_line(start, end, thickness);
        
        start = end;
    }
}

void body_slap(Body* body, CF_V2 force)
{
    switch (body->type)
    {
        case Body_Type_Human:
        {
            force = cf_mul_v2_f(force, 10.0f);
            body->accelerations[Body_Human_Head] = cf_add(body->accelerations[Body_Human_Head], force);
            body->accelerations[Body_Human_Neck] = cf_add(body->accelerations[Body_Human_Neck], force);
            body->accelerations[Body_Human_Left_Shoulder] = cf_add(body->accelerations[Body_Human_Left_Shoulder], force);
            body->accelerations[Body_Human_Right_Shoulder] = cf_add(body->accelerations[Body_Human_Right_Shoulder], force);
            //body->accelerations[Body_Human_Left_Hip] = cf_add(body->accelerations[Body_Human_Left_Hip], force);
            //body->accelerations[Body_Human_Right_Hip] = cf_add(body->accelerations[Body_Human_Right_Hip], force);
            break;
        }
        case Body_Type_Tentacle:
        {
            body->accelerations[Body_Tentacle_Count - 1] = cf_add(body->accelerations[Body_Tentacle_Count - 1], force);
            break;
        }
        case Body_Type_Slime:
        {
            for (s32 index = Body_Slime_Center + 1; index < Body_Slime_Count; ++index)
            {
                body->accelerations[index] = cf_add(body->accelerations[index], force);
            }
            break;
        }
    }
}

CF_Sprite get_body_sprite(Body_Type type)
{
    CF_Sprite sprite = assets_get_sprite(ASSETS_DEFAULT);
    
    switch (type)
    {
        case Body_Type_Tentacle:
        {
            sprite = assets_get_sprite("sprites/award.ase");
            sprite.blend_index = ASSETS_BLEND_LAYER_DEFAULT;
            break;
        }
        case Body_Type_Slime:
        {
            sprite = assets_get_sprite("sprites/exploding.ase");
            sprite.blend_index = ASSETS_BLEND_LAYER_DEFAULT;
            break;
        }
        case Body_Type_Hand:
        {
            sprite = assets_get_sprite("sprites/hand.ase");
            sprite.blend_index = ASSETS_BLEND_LAYER_DEFAULT;
            break;
        }
        case Body_Type_Tubeman:
        case Body_Type_Human:
        default:
        {
            sprite = assets_get_sprite("sprites/character.ase");
            sprite.blend_index = ASSETS_BLEND_LAYER_DEFAULT;
            break;
        }
    }
    
    return sprite;
}