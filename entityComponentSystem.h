#ifndef __ENTITY_COMPONENT_SYSTEM_H__
#define __ENTITY_COMPONENT_SYSTEM_H__ 1

#include "types.h"
#include "logging.h"

#include <GL/gl.h>

#define ComponentFlags u32

#define ComponentFlagName(name) name ## ComponentFlag
#define DeclareComponentFlag(name) extern u32 ComponentFlagName(name)
#define SetValueForComponentFlag(name) u32 ComponentFlagName(name) = (1<<__COUNTER__);
#define ImportComponentFlag(name) u32 ComponentFlagName(name);
#define GetComponentFlag(name) (ComponentFlagName(name))
#define PrintFlagValue(name) DEBUG_LOG("Component flag value [%12s] = %#06x", #name, GetComponentFlag(name))

DeclareComponentFlag(Allocated);

DeclareComponentFlag(Position);
DeclareComponentFlag(Velocity);
DeclareComponentFlag(Health);
DeclareComponentFlag(Gravity);
DeclareComponentFlag(Renderable);

typedef struct
{
  float width;
  float height;
  GLuint textureId;
} Renderable;

typedef struct
{
    float x;
    float y;
} Position;

typedef struct
{
    float vx;
    float vy;
    float vxMax;
    float vyMax;
} Velocity;

typedef struct
{
    int hp;
} Health;

typedef struct
{
    ComponentFlags* components;
    Position* position;
    Velocity* velocity;
    Health* health;
    Renderable* renderable;
} Entity;

#define BATCH_SIZE (10)
typedef struct
{
    u32 entityCount;
    ComponentFlags      entityComponents[BATCH_SIZE];
    Position            positions[BATCH_SIZE];
    Velocity            velocities[BATCH_SIZE];
    Health              healths[BATCH_SIZE];
    Renderable renderables[BATCH_SIZE];
} EntityBatch;

typedef struct
{
    u32 batchCount;
    u32 entityCount;
    float lastTickDt;
    EntityBatch** batches;
} World;

#define HasComponent(flags, component) ((GetComponentFlag(component) & flags) != 0)
#define IsAllocated(entity) (GetComponentFlag(Allocated) & (*entity->components))

void InitEntityInBatch(Entity* entity, EntityBatch* batch, ComponentFlags flags);

void NextEntity(Entity* entity);

typedef void (*UpdateSystemFunction)(World*);

World* CreateWorld(u32 entityCount);
EntityBatch* BatchContainingEntity(World* world, u32 entityId, u32* entityPosition);
Entity* EntityFromWorld(World* world, u32 entityId);

u32 NewEntityInWorld(World* world, ComponentFlags requiredComponents);
void AddComponentsToEntityInWorld(World* world, u32 entityId, ComponentFlags components);
void RemoveComponentsFromEntityInWorld(World* world, u32 entityId, ComponentFlags components);

typedef struct
{
    u64 dependsOnSystems;
    UpdateSystemFunction updateFunction;
    ComponentFlags componentsUsed;
    u8 id;
} SystemDescriptor;

void LoadSystems(char* fileName);
void RunSystems(World* world);

#endif
