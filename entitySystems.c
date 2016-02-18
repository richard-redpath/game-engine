#define NO_PRINT

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include "logging.h"

#include "entityComponentSystem.h"
#include "entityComponentSystem_dynamic.h"

ImportComponentFlag(Position);
ImportComponentFlag(Allocated);
ImportComponentFlag(Health);
ImportComponentFlag(Velocity);
ImportComponentFlag(Gravity);
ImportComponentFlag(Renderable);

#define PRINT_POSITION_OPERATES_ON (GetComponentFlag(Position))

static inline void ApplyToAllEntitiesInWorld(World* world, void(someFunction)(Entity*,float), ComponentFlags requires)
{
    EntityBatch* batch = NULL;
    Entity entity;
    u32 batchCount = world->batchCount;
    //DEBUG_LOG("Processing %d batches", batchCount);

    requires |= GetComponentFlag(Allocated);

    u32 entityIdx = 0;
    float dt = world->lastTickDt;

    if(batchCount)
    {
	batch = world->batches[0];   
	while(batchCount--)
	{
	    InitEntityInBatch(&entity, batch, requires);
	    batch++;
	    entityIdx = 0;

	    while(entityIdx++ < BATCH_SIZE)
	    {
		if((*entity.components & requires) == requires)
		    (*someFunction)(&entity, dt);
		NextEntity(&entity);
	    }
	}
    }
}

static inline void printEntityPosition(Entity* entity, float dt)
{
    DEBUG_LOG("Entity position = %f %f", entity->position->x, entity->position->y);
}

static inline void printEntityHealth(Entity* entity, float dt)
{
    DEBUG_LOG("Entity health = %d", entity->health->hp);
}

static inline void printEntityVelocity(Entity* entity, float dt)
{
    DEBUG_LOG("Entity velocity = %f %f", entity->velocity->vx, entity->velocity->vy);
}

static inline void applyGravity(Entity* entity, float dt)
{
    entity->velocity->vy += (dt * 9.81f);
    entity->velocity->vx = 0;
}

static inline void clampVelocity(Entity* entity)
{
  return;
    entity->velocity->vx = fmin(entity->velocity->vx, entity->velocity->vxMax);
    entity->velocity->vy = fmin(entity->velocity->vy, entity->velocity->vyMax);
  
    entity->velocity->vx = fmax(entity->velocity->vx, -entity->velocity->vxMax);
    entity->velocity->vy = fmax(entity->velocity->vy, -entity->velocity->vyMax);
}

static inline void moveEntity(Entity* entity, float dt)
{
    clampVelocity(entity);
    entity->position->x += (dt * entity->velocity->vx);
    entity->position->y += (dt * entity->velocity->vy);
}

static inline void render(Entity* entity, float dt)
{
    Renderable* renderable = entity->renderable;
    Position* position = entity->position;

    glBindTexture(GL_TEXTURE_2D, renderable->textureId);

    DEBUG_LOG("Bind - %d", glGetError());

    glPushMatrix();
    glTranslatef(position->x, position->y, -10);

    DEBUG_LOG("Translate - %d", glGetError());
    
    glBegin(GL_QUADS);

    //glColor4f(1.0, 0.0, 0.0, 1.0);
    glTexCoord2f(0, 0);
    DEBUG_LOG("Coord - %d", glGetError());


    glVertex3f(0, 0, 0);

    DEBUG_LOG("Vert - %d", glGetError());

    glTexCoord2f(1, 0);
    glVertex3f(renderable->width, 0, 0);    

    glTexCoord2f(1, 1);
    glVertex3f(renderable->width, renderable->height, 0);

    glTexCoord2f(0, 1);
    glVertex3f(0, renderable->height, 0);

    glEnd();
    glPopMatrix();
}

#define PRINT_POSITION_SYSTEM_COMPONENTS (GetComponentFlag(Position))
void printPositionSystem(World* world)
{
    ApplyToAllEntitiesInWorld(world, &printEntityPosition, PRINT_POSITION_SYSTEM_COMPONENTS);
}

#define PRINT_HEALTH_SYSTEM_COMPONENTS (GetComponentFlag(Health))
void printHealthSystem(World* world)
{
    ApplyToAllEntitiesInWorld(world, &printEntityHealth, PRINT_HEALTH_SYSTEM_COMPONENTS);
}

#define PRINT_VELOCITY_SYSTEM_COMPONENTS (GetComponentFlag(Velocity))
void printVelocitiesSystem(World* world)
{
    ApplyToAllEntitiesInWorld(world, &printEntityVelocity, PRINT_VELOCITY_SYSTEM_COMPONENTS);
}

#define APPLY_MOVE_SYSTEM_COMPONENTS (GetComponentFlag(Velocity)|GetComponentFlag(Position))
void doMovementSystem(World* world)
{
    ApplyToAllEntitiesInWorld(world, &moveEntity, APPLY_MOVE_SYSTEM_COMPONENTS);
}

#define APPLY_GRAVITY_SYSTEM_COMPONENTS (GetComponentFlag(Gravity)|GetComponentFlag(Velocity))
void applyGravitySystem(World* world)
{
    ApplyToAllEntitiesInWorld(world, &applyGravity, APPLY_GRAVITY_SYSTEM_COMPONENTS);
}

#define APPLY_RENDER_SYSTEM_COMPONENTS (GetComponentFlag(Renderable)|GetComponentFlag(Position))
void applyRenderSystem(World* world)
{
    glClearColor(0, 0, 1, 0);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    ApplyToAllEntitiesInWorld(world, &render, APPLY_RENDER_SYSTEM_COMPONENTS);
    glFlush();
}

static inline u64 SystemDependenciesFromIds(u8 idCount, va_list* args)
{
    u64 ret = 0;
    while(idCount--)
    {
	ret |= (1<<va_arg(*args, int));
    }
    return ret;
}

SystemDescriptor* BuildSystemDescriptor(u8 id, ComponentFlags usesComponents, void(systemFunction)(World*), int numDependencies, ...)
{
    va_list args;
    va_start(args, numDependencies);

    SystemDescriptor* ret = malloc(sizeof(SystemDescriptor));
    ret->dependsOnSystems = SystemDependenciesFromIds(numDependencies, &args);
    ret->updateFunction = systemFunction;
    ret->componentsUsed = usesComponents;
    ret->id = id;

    va_end(args);

    return ret;
}

SystemDescriptor** GetSystemDescriptors()
{
    SystemDescriptor** ret = malloc(sizeof(SystemDescriptor*) * 6);
    ret[0] = BuildSystemDescriptor(1, APPLY_GRAVITY_SYSTEM_COMPONENTS, &applyGravitySystem, 0);
    //ret[1] = BuildSystemDescriptor(2, PRINT_VELOCITY_SYSTEM_COMPONENTS, &printVelocitiesSystem, 1, 1);
    ret[1] = BuildSystemDescriptor(5, APPLY_MOVE_SYSTEM_COMPONENTS, &doMovementSystem, 1, 1);
    //ret[2] = BuildSystemDescriptor(4, PRINT_POSITION_SYSTEM_COMPONENTS, &printPositionSystem, 1, 5);
    ret[2] = BuildSystemDescriptor(7, APPLY_RENDER_SYSTEM_COMPONENTS, &applyRenderSystem, 2, 1, 5);
    ret[3] = NULL;
    return ret;
}
