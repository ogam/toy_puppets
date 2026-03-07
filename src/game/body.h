//  @note:  if making a lot of different body types, then make a tool with alignment
//          constraint adjustments then have this export this out to c and some data
//          format that can be edit later on. all of the bodies below ere done 
//          manually and it was a huge pain

#ifndef BODY_H
#define BODY_H

typedef s32 Body_Type;

//  @todo:  get rid of the prefix here
#define Body_Type(PREFIX, ENUM) \
ENUM(PREFIX, Human) \
ENUM(PREFIX, Tentacle) \
ENUM(PREFIX, Slime) \
ENUM(PREFIX, Hand) \
ENUM(PREFIX, Count)

MAKE_ENUM(Body_Type);

// https://web.archive.org/web/20080410171619/http://www.teknikus.dk/tj/gdc2001.html
typedef struct Particle_Constraint
{
    s32 a;
    s32 b;
    f32 length;
    f32 base_length;
    f32 stiffness;
} Particle_Constraint;

typedef struct Body
{
    dyna CF_V2* prev_particles;
    dyna CF_V2* particles;
    dyna CF_V2* accelerations;
    dyna b8* is_locked;
    dyna Particle_Constraint* constraints;
    
    dyna s32* hurt_particles;
    
    Body_Type type;
    f32 time_step;
    
    f32 height;
    f32 base_height;
    
    // x axis only, 1 for right, -1 for left, 0 facing towards camera
    f32 facing_direction;
    CF_V2 position;
    
    f32 wheel_angle;
} Body;

typedef struct Body_Proportions
{
    f32 scale;
    
    // human
    f32 head_scale;
    f32 neck_thickness;
    f32 torso_chubiness;
    
    f32 upper_arm_thickness;
    f32 lower_arm_thickness;
    f32 hand_thickness;
    
    f32 upper_leg_thickness;
    f32 lower_leg_thickness;
    f32 foot_thickness;
    
    f32 upper_arm_length;
    f32 lower_arm_length;
    f32 upper_leg_length;
    f32 lower_leg_length;
    f32 leg_length;
} Body_Proportions;

Body_Proportions make_default_body_proportions();
Body_Proportions make_random_body_proportions();

Body make_default_body(Body_Type type, s32 particle_count, f32 height);
void destroy_body(Body* body);
void body_scale(Body* body, Body_Proportions proportions);

void body_acceleration_reset(Body* body);
void body_verlet(Body* body);
void body_satisfy_constraints(Body* body, CF_Aabb bounds);

void body_stabilize(Body* body);

CF_V2 body_centeroid(Body* body);
CF_V2 body_prev_centeroid(Body* body);

void body_draw_constraints(Body* body, f32 thickness);
void body_draw(Body* body, Body_Proportions proportions);

s32 body_particles_touching_ground(Body* body);
CF_Aabb body_get_bounds(Body* body);
CF_Aabb body_get_particle_bounds(Body* body, s32* particles, s32 count);

void body_teleport(Body* body, CF_V2 position);

// @human

typedef s32 Body_Human;
enum
{
    Body_Human_Left_Hip,
    Body_Human_Right_Hip,
    Body_Human_Left_Shoulder,
    Body_Human_Right_Shoulder,
    Body_Human_Neck,
    Body_Human_Head,
    Body_Human_Left_Elbow,
    Body_Human_Left_Hand,
    Body_Human_Right_Elbow,
    Body_Human_Right_Hand,
    Body_Human_Left_Knee,
    Body_Human_Left_Foot,
    Body_Human_Right_Knee,
    Body_Human_Right_Foot,
    Body_Human_Count,
};

Body make_human_body(f32 height);
void body_human_stabilize(Body* body);
b32 body_human_is_flipped(Body* body);

void body_human_scale(Body* body, Body_Proportions proportions);
void body_human_draw(Body* body, Body_Proportions proportions);

void body_human_hand_guard(Body* body);
void body_human_walk(Body* body, CF_V2 direction, CF_V2 stride, f32 move_speed, f32 spin_speed);
void body_human_idle(Body* body, CF_V2 stride, f32 spin_speed);

void body_human_kick_co(CF_Coroutine co);

CF_V2 body_human_get_punch_target(Body* body, Body* target_body);
void body_human_punch_co(CF_Coroutine co);

void body_human_fix_head_co(CF_Coroutine co);

// @human

// @tentacle

typedef s32 Body_Tentacle;
enum
{
    Body_Tentacle_Root,
    Body_Tentacle_Count = 10
};

Body make_tentacle_body(f32 height);
void body_tentacle_stabilize(Body* body);

void body_tentacle_draw(Body* body, Body_Proportions proportions);

void body_tentacle_idle(Body* body);
void body_tentacle_whip_co(CF_Coroutine co);
// @tentacle

// @slime

typedef s32 Body_Slime;
enum
{
    Body_Slime_Center,
    Body_Slime_Count = 17
};

Body make_slime_body(f32 height);
void body_slime_stabilize(Body* body);

void body_slime_draw(Body* body, Body_Proportions proportions);

void body_slime_walk(Body* body, CF_V2 direction, f32 move_speed);
void body_slime_hop(Body* body, CF_V2 direction, CF_V2 stride);
void body_slime_lunge_co(CF_Coroutine co);
void body_slime_forward_spike_co(CF_Coroutine co);

// @slime

// @hand

typedef s32 Body_Hand;
enum
{
    Body_Hand_Index_Tip,
    Body_Hand_Index_Joint,
    Body_Hand_Index_Base,
    Body_Hand_Middle_Tip,
    Body_Hand_Middle_Joint,
    Body_Hand_Middle_Base,
    Body_Hand_Pinky_Tip,
    Body_Hand_Pinky_Joint,
    Body_Hand_Pinky_Base,
    Body_Hand_Thumb_Tip,
    Body_Hand_Thumb_Joint,
    Body_Hand_Thumb_Base,
    Body_Hand_Wrist,
    Body_Hand_Count,
};

Body make_hand_body(f32 height);
void body_hand_stabilize(Body* body);

void body_hand_draw(Body* body, Body_Proportions proportions);

void body_hand_move(Body* body, CF_V2 position);
void body_hand_pinch(Body* body);
void body_hand_gesture(Body* body);

// slaps are mainly for arena so if coroutine ends early make sure to call
// body_hand_slap_end(), the begin here is just to stick to pattern
void body_hand_slap_begin(Body* body);
void body_hand_slap_end(Body* body);
void body_hand_slap_co(CF_Coroutine co);

void body_hand_cast(Body* body);

// @hand

// utils
void body_draw_line(CF_V2 p0, CF_V2 p1, f32 p0_thickness, f32 p1_thickness, s32 iterations);
void body_slap(Body* body, CF_V2 force);

CF_Sprite get_body_sprite(Body_Type type);

#endif //BODY_H
