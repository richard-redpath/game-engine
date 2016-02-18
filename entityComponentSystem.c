#define NO_PRINT

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <sys/types.h>

#include "logging.h"

#include "entityComponentSystem.h"

// Initialise component values
SetValueForComponentFlag(Allocated)
SetValueForComponentFlag(Position)
SetValueForComponentFlag(Velocity)
SetValueForComponentFlag(Health)
SetValueForComponentFlag(Gravity)
SetValueForComponentFlag(Renderable)

// Allocate a new batch of entities and mark all entities as having no components
EntityBatch* CreateEntityBatch()
{
    DEBUG_LOG("Creating new entity batch");
    EntityBatch* ret = (EntityBatch*)malloc(sizeof(EntityBatch));

    u32 idx;
    for(idx = 0; idx < BATCH_SIZE; ++idx)
	ret->entityComponents[idx] = 0;

    ret->entityCount = 0;

    return ret;
}

// Create a world with enough space to hold a given number of entities
World* CreateWorld(u32 entityCount)
{
    // How many batches will we fill?
    u32 batchCount = (entityCount/BATCH_SIZE);

    batchCount += ((entityCount % BATCH_SIZE) != 0); // Do we have any extras

    DEBUG_LOG("Creating world with capacity for %d entities in %d batches", entityCount, batchCount);
  
    // Allocate the memory for our info and enough pointers to hold our batches
    World* ret = (World*)malloc(sizeof(World));
    ret->batches = (EntityBatch**)malloc(batchCount * sizeof(EntityBatch*));
    ret->batchCount = batchCount;
  
    ret->lastTickDt = 0.033f;

    // Create our batches
    while(batchCount--)
	ret->batches[batchCount] = CreateEntityBatch();

    return ret;
}

void InitEntityInBatch(Entity* entity, EntityBatch* batch, ComponentFlags flags)
{
    entity->components = batch->entityComponents;
    entity->position = HasComponent(flags, Position) ? batch->positions : NULL;
    entity->velocity = HasComponent(flags, Velocity) ? batch->velocities : NULL;
    entity->health = HasComponent(flags, Health) ? batch->healths : NULL;
    entity->renderable = HasComponent(flags, Renderable) ? batch->renderables : NULL;
}

void NextEntity(Entity* entity)
{
    entity->components++;
    if(entity->position) entity->position++;
    if(entity->velocity) entity->velocity++;
    if(entity->health) entity->health++;
    if(entity->renderable) entity->renderable++;
}

// If we are only interested in one entity we need its batch and its position in that batch
EntityBatch* BatchContainingEntity(World* world, u32 entityId, u32* entityPosition)
{
    u32 batchId = entityId/BATCH_SIZE;

    *entityPosition = entityId%BATCH_SIZE;
  
    DEBUG_LOG("Requesting batch for entity %d, returning batch %d with offset %d", entityId, batchId, *entityPosition);
  
    return world->batches[batchId];
}

Entity* EntityFromWorld(World* world, u32 entityId)
{
    u32 entityIdx;
    EntityBatch* batch = BatchContainingEntity(world, entityId, &entityIdx);
  
    Entity* ret = malloc(sizeof(Entity));
    ret->components = batch->entityComponents + entityIdx;
    ret->position = batch->positions + entityIdx;
    ret->velocity = batch->velocities + entityIdx;
    ret->health = batch->healths + entityIdx;
    ret->renderable = batch->renderables + entityIdx;
  
    return ret;
}

// Find room for a new entity, recycling where possible
// If no room is found then create a new batch
u32 NewEntityInWorld(World* world, ComponentFlags requiredComponents)
{
    DEBUG_LOG("Creating new entity with components %#06x", requiredComponents);

    // Skip over full batches
    u32 batchIdx = 0;
    while(world->batches[batchIdx]->entityCount == BATCH_SIZE)
    {
	DEBUG_LOG("Batch %d is full, moving on", batchIdx);
      
	// Move to the next batch
	batchIdx++;
      
	// If this leaves us with a valid, allocated batch then try again
	if(batchIdx < world->batchCount)
	    continue;

	// Otherwise we have filled all of our batches and must make a new one
	DEBUG_LOG("All batches are full, creating new batch");

	// How much memory do we need?
	u32 newBatchArraySize = ++world->batchCount * sizeof(EntityBatch*);

	// Get more memory (staying in place if we can)
	world->batches = realloc(world->batches, newBatchArraySize);

	// Allocate the new batch
	world->batches[batchIdx] = CreateEntityBatch();
	DEBUG_LOG("    New batch count = %d", world->batchCount);
	break;
    }

    // We are now guaranteed to be pointing to a batch with at least 1 free space
    EntityBatch* batch = world->batches[batchIdx];
    DEBUG_LOG("Have batch reference %p", batch);
  
    // Skip over allocated entity slots
    u32 entityIdx = 0;
    while(batch->entityComponents[entityIdx] & GetComponentFlag(Allocated))
    {
	DEBUG_LOG("Entity %d of batch is occupied", entityIdx);
	entityIdx++;
    }

    DEBUG_LOG("Entity index = %d", entityIdx);

    // Mark this entity slot as allocated as well as containing the required components
    batch->entityComponents[entityIdx] = (requiredComponents | GetComponentFlag(Allocated));
    batch->entityCount++;

    DEBUG_LOG("Set flags");

    // Increase our returned id by the number of entities we skipped in the batch finding stage
    entityIdx += (batchIdx * BATCH_SIZE);

    DEBUG_LOG("Creating new entity with index %d", entityIdx);
  
    return entityIdx;
}

// Allows us to dynamically add components to entities
void AddComponentsToEntityInWorld(World* world, u32 entityId, ComponentFlags components)
{
    DEBUG_LOG("Adding components %#06x to entity %d", components, entityId);
    u32 idxInBatch;
    EntityBatch* batch = BatchContainingEntity(world, entityId, &idxInBatch);
    batch->entityComponents[idxInBatch] |= components;
}

// Allows us to dynamically remove components from entities
void RemoveComponentsFromEntityInWorld(World* world, u32 entityId, ComponentFlags components)
{
    DEBUG_LOG("Adding components %#06x to entity %d", components, entityId);
    u32 idxInBatch;
    EntityBatch* batch = BatchContainingEntity(world, entityId, &idxInBatch);
    batch->entityComponents[idxInBatch] &= !components;
}


/*************************************************************************
 **                                                                     **
 **                      This way insanity lies                         **
 **                                                                     **
 *************************************************************************/

// Its a swap function...
void SwapDescriptorPointers(SystemDescriptor** p1, SystemDescriptor** p2)
{
    SystemDescriptor* tmp = *p1;
    *p1 = *p2;
    *p2 = tmp;
}

// Change the order of systems so that they always follow what they depend on
u64 ResolveDependencyOrder(SystemDescriptor** resolveFrom)
{
    // Checking lies in the unresolved section, swapTo lies at the start of it
    SystemDescriptor** checking = resolveFrom;
    SystemDescriptor** swapTo = resolveFrom;

    // What systems have we already resolved?
    u64 resolvedFlags = 0;

    // As long as we are looking at a descriptor
    while(*checking)
    {
	// What does this system need?
	u64 requiredFlags = (*checking)->dependsOnSystems;

	// If the system we are checking has had its dependencies resolved
	if((requiredFlags & resolvedFlags) == requiredFlags)
	{
	    // Include this system in the resolved systems flags
	    resolvedFlags |= (1 << (*checking)->id);

	    // Move it to the resolved section
	    SwapDescriptorPointers(swapTo, checking);

	    // Move the start of the unresolved section along
	    swapTo++;

	    // And start again at the start of the unresolved section
	    checking = swapTo;
	}
	else
	{
	    // We can not yet run this system, try the next one
	    checking++;
	}
    }

    // Return the flags of all resolved systems, if these match the provided systems flags
    // then everything has been resolved properly
    return resolvedFlags;
}

// Global defines for dynamic loading
char* systemFilename = NULL;
ino_t systemFileInodeNumber;
time_t systemFileEditTime = 0;
void* systemFileDLHandle = NULL;

// Which systems do we have?
SystemDescriptor systems[64];  
int numSystems = 0;  

// @TODO: Error checking in here
// Copies a file in 1K chunks, no error checking implemented yet
static inline u8 CopyFile(char* source, char* dest)
{
    // Open our files
    FILE* inFile = fopen(source, "rb");
    FILE* outFile = fopen(dest, "wb");

    char buffer[1024]; // Where to store stuff
    u32 read; // How many bytes did we read?

    // As long as there is some file left
    while(!feof(inFile))
    {
	// Read in (upto) 1K
	read = fread(buffer, 1, 1024, inFile);
	fwrite(buffer, 1, read, outFile); // And write it out
    }

    // Close our input
    fclose(inFile);

    // Ensure our output writing has finished and close it
    fflush(outFile);
    fclose(outFile);

    return 1;
}

// Updated the globals associated with dynamic loading
static inline void UpdateSystemFileInfo(char* filename, time_t modified, ino_t inodeNumber)
{
    systemFileEditTime = modified;
    systemFileInodeNumber = inodeNumber;
    if(systemFilename)
	free(systemFilename);
    systemFilename = malloc(sizeof(char)*strlen(filename));
    strcpy(systemFilename, filename);

    systemFileEditTime = modified;
}

typedef struct
{
    u8 changed;
    time_t modified;
    ino_t inodeNumber;
} FileChangedResult;

// Check if the system file has changed, either contents or by name
static inline FileChangedResult FileHasChangedDebug(char* filename)
{
    FileChangedResult ret;
    
    // Get the last modified time of the provided file
    struct stat fileAttrs;
    if(stat(filename, &fileAttrs))
    {
	DEBUG_ERR("Unable to get stats on file, stat() call failed");
	ret.changed = 0;
	return ret;
    }

    time_t modified = fileAttrs.st_mtime;
    ino_t inodeNumber = fileAttrs.st_ino;

    // File passed is newer, or is a different file on disk to what we had before
    if((modified > systemFileEditTime) || (inodeNumber != systemFileInodeNumber))
    {
	ret.changed = 1;
	ret.modified = modified;
	ret.inodeNumber = inodeNumber;
	return ret;
    }
    
    // Name is the same and we have not changed
    ret.changed = 0;
    return ret;
}

#ifdef DEBUG
#define FileHasChanged(name) FileHasChangedDebug(name)
#else
#define FileHasChanged(name) ((FileChangedResult){0, 0, 0})
#endif

// Query our system file for what system functions it contains
typedef SystemDescriptor** (*SystemDescriptorQueryFunction)();

static inline SystemDescriptorQueryFunction LoadNewFile(char* filename)
{
    dlerror(); // Clear any old errors

    DEBUG_LOG("Copying file");

    char tempName[255];
    tmpnam_r(tempName);
    
    // Copy the newly compiled file so we can overwrite it when we compile later
    if(!CopyFile(filename, tempName))
	return NULL;

    DEBUG_LOG("File copied");
  
    // Open the library file and resolve dependencies now
    void* newFile = dlopen(tempName, RTLD_NOW);  
    if(newFile == NULL)  
    {  
	DEBUG_ERR("Failed to open systems file \"%s\":", filename);  
	DEBUG_ERR("\t%s", dlerror());  
	return NULL;  
    }

    // The new file is legal, close the old one
    if(systemFileDLHandle)
    {
	DEBUG_LOG("Closing so file handle");
	if(dlclose(systemFileDLHandle))
	{
	    DEBUG_ERR("Unable to close old system file");
	    DEBUG_ERR("\t%s", dlerror());
	    return NULL;
	}
    }

    // Clear errors
    dlerror();

    DEBUG_LOG("Getting query function");
  
    // Try to get a handle to the function
    SystemDescriptorQueryFunction func = dlsym(newFile, "GetSystemDescriptors");
    DEBUG_LOG("Finished dlsym call");
    if(func == NULL)
    {
	DEBUG_ERR("Failed to get system descriptor query function handle");
	DEBUG_ERR("\t%s", dlerror());
	return NULL;
    }

    systemFileDLHandle = newFile;

    return func;
}

// Convenience methods (only used for printing)
static inline u32 HigherBits(u64 val)
{
    return (u32)((0xffffffff & val)>>32);
}

static inline u32 LowerBits(u64 val)
{
    return (u32)(0xffffffff & val);
}

#define u64PrintfSymbol "%#06x%04x"
#define u64ToPrintf(val) HigherBits(val),LowerBits(val)

// Perform the query and manage the returned system descriptors
static inline void LoadDescriptors(SystemDescriptorQueryFunction queryFunction)
{ 
    // Do the query
    SystemDescriptor** firstDescriptor = (*queryFunction)();
    if(firstDescriptor == NULL)
    {
	DEBUG_ERR("System descriptor query function returned NULL");
	return;
    }

    // As we load we should track which system ids are used
    u64 loadedSystems = 0;

    // How many descriptors do we have?
    u8 descriptorCount = 0;
    SystemDescriptor** iterator = firstDescriptor;
    while(*iterator && (++descriptorCount<64)) 
	loadedSystems |= (1<<((*iterator++)->id));

    // It had better be less than 4
    if(descriptorCount >= 64)
    {
	DEBUG_ERR("Too many system descriptors, 64 allowed at most");
	return;
    }

    DEBUG_LOG("Loaded %d descriptors", descriptorCount);

    int i;
    for(i = 0; i < descriptorCount; ++i)
	DEBUG_LOG("System %d requires %#06x%04x", firstDescriptor[i]->id, HigherBits(firstDescriptor[i]->dependsOnSystems), LowerBits(firstDescriptor[i]->dependsOnSystems));

    // Try to resolve the dependencies
    u64 resolvedSystems = ResolveDependencyOrder(firstDescriptor);
    if(resolvedSystems != loadedSystems)
    {
	DEBUG_ERR("Failed to resolve system dependencies, check you don't have cyles");
	DEBUG_ERR("    Loaded systems     "u64PrintfSymbol, u64ToPrintf(loadedSystems));
	DEBUG_ERR("    Resolved systems   %#06x%04x", HigherBits(resolvedSystems), LowerBits(resolvedSystems));
	return;
    }

    // We have sucessfully managed to load the file, retrieve the definition function
    // and resolve dependencies. Copy the descriptor information and cleanup
    numSystems = descriptorCount;
    while(descriptorCount--)
    {
	systems[descriptorCount] = **(firstDescriptor + descriptorCount);
	DEBUG_LOG("Loaded system id = %d, depends on %#010x", systems[descriptorCount].id, LowerBits(systems[descriptorCount].dependsOnSystems));
    }
}

// One call to rule them all
void LoadSystems(char* filename)  
{
    DEBUG_LOG("Checking file changed");

    // One call to find them
    FileChangedResult res;
    if(!(res = FileHasChanged(filename)).changed)
	return;

    DEBUG_LOG("File has changed, getting systems");
  
    // One call to bring them all
    SystemDescriptorQueryFunction queryFunction = LoadNewFile(filename);
    DEBUG_LOG("Query function is %p", queryFunction);
    if(queryFunction == NULL)
    {
	DEBUG_ERR("Null query function, returning");
	return;
    }

    DEBUG_LOG("Query function found, querying");

    UpdateSystemFileInfo(filename, res.modified, res.inodeNumber);
  
    // And in the globals bind them
    LoadDescriptors(queryFunction);

    DEBUG_LOG("Have query functions");
}

// Runs the systems, I named this one quite well
void RunSystems(World* world)
{
    int i;
    DEBUG_LOG("************************************");
    DEBUG_LOG("        Running %3d systems         ", numSystems);
    DEBUG_LOG("************************************");
    for(i = 0; i < numSystems; ++i)
    {
	systems[i].updateFunction(world);
    }
}
