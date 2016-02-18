#define NO_PRINT

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_opengl.h>

#include "keyHandling.h"
#include "entityComponentSystem.h"
#include "logging.h"
#include "2dsprites.h"

static SDL_Window* window;
static SDL_GLContext glContext;

int main()
{
    // Initialise SDL
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
	DEBUG_ERR("Failed to initialise SDL video");
	exit(1);
    }

    // Begin setup of our GL context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // Request a window
    window = SDL_CreateWindow("My Window", 10, 10, 512, 512, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if(!window)
    {
	DEBUG_ERR("Failed to create SDL_GL window");
	exit(2);
    }

    // And get a handle to its context
    glContext = SDL_GL_CreateContext(window);
    if(!glContext)
    {
	DEBUG_ERR("Failed to get context for window");
	exit(3);
    }

    // Setup opengl
    glViewport(0, 0, 512, 512);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 512, 512, 0, -100, 100);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    PrintFlagValue(Allocated);
    PrintFlagValue(Position);
    PrintFlagValue(Velocity);
    PrintFlagValue(Health);

    World* world15 = CreateWorld(15);
  
    int i;
    DEBUG_LOG("Creating 12 entities");
    for(i = 0; i < 12; ++i)
    {
	u32 newEntity = NewEntityInWorld(world15, GetComponentFlag(Position));
	if(i%3 == 0)
	{
	  AddComponentsToEntityInWorld(world15, newEntity, GetComponentFlag(Position)|GetComponentFlag(Gravity)|GetComponentFlag(Velocity));
	    Entity* entity = EntityFromWorld(world15, newEntity);
	    entity->velocity->vyMax = 10;
	    entity->velocity->vxMax = 5;
	    entity->position->x = i * 35;
	    entity->position->y = 10;

	    SetRenderableSpriteForEntityInWorld(world15, newEntity, "./smilie.png", 50, 50);
	}
	DEBUG_LOG("Entity %d has id %d", i, newEntity);
    }

    u8 run = 1;

    SDL_Event event;
    
    while(run)
    {
      while(SDL_PollEvent(&event))
	{
	  if(event.type == SDL_QUIT)
	    {
	      run = 0;
	    }
	}

      
	usleep(33333);
	DEBUG_LOG("Loading systems");
	LoadSystems("./lib/entitySystems.so");
	DEBUG_LOG("Loaded systems, running them");
	RunSystems(world15);
	DEBUG_LOG("Systems run, terminating");

	DEBUG_LOG("Updating display");
	SDL_GL_SwapWindow(window);
    }
    
    // Cleanup stuff
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
