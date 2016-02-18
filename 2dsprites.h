#ifndef __2DSPRITES_H__
#define __2DSPRITES_H__

#include <GL/gl.h>
#include "entityComponentSystem.h"
#include "types.h"

GLuint LoadTexture(char* filename, u32* width, u32* height);
void SetRenderableSpriteForEntityInWorld(World* world, u32 entityId, char* filename, u32 width, u32 height);
void FreeTexture(GLuint texture);

#endif
