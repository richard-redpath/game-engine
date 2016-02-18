#include <string.h>

#include "2dsprites.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

typedef struct
{
  GLuint glTextureId;
  char filename[240];
} LoadedTexture;

u8 loadedTextureCount = 0;
LoadedTexture loadedTextures[256];

GLuint LoadTexture(char* filename, u32* width, u32* height)
{
  // Start checking textures from the last one loaded. Latest loaded is most likely to be used next?
  u8 checkTexture = loadedTextureCount;
  while(checkTexture--)
    if(strcmp(filename, loadedTextures[checkTexture].filename) == 0)
      return loadedTextures[checkTexture].glTextureId;

  // We have not already loaded this texture, load and bind it now
  int componentsPerPixel;
  unsigned char* imageData = stbi_load(filename, width, height, &componentsPerPixel, 4);
  LoadedTexture* texture = &loadedTextures[loadedTextureCount++];

  GLuint newTexture;
  glGenTextures(1, &newTexture);
  DEBUG_LOG("%d", glGetError());
  
  glBindTexture(GL_TEXTURE_2D, newTexture);
  DEBUG_LOG("%d", glGetError());

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  DEBUG_LOG("%d", glGetError());

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  DEBUG_LOG("%d", glGetError());

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  DEBUG_LOG("%d", glGetError());

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  DEBUG_LOG("%d", glGetError());

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, *width, *height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
  DEBUG_LOG("%d", glGetError());

  stbi_image_free(imageData);

  strcpy(texture->filename, filename);
  texture->glTextureId = newTexture;

  return newTexture;
}

void SetRenderableSpriteForEntityInWorld(World* world, u32 entityId, char* filename, u32 width, u32 height)
{
    u32 a, b;
    GLuint texture = LoadTexture(filename, &a, &b);
    Entity* entity = EntityFromWorld(world, entityId);
    AddComponentsToEntityInWorld(world, entityId, GetComponentFlag(Renderable));
    DEBUG_LOG("Adding texture \"%s\" to entity %d (%p)", filename, entityId, entity);
    DEBUG_LOG("Renderable = %p", entity->renderable);
    PrintFlagValue(Renderable);
    entity->renderable->textureId = texture;
    entity->renderable->width = width;
    entity->renderable->height = height;
}

void FreeTexture(GLuint texture)
{
}
