#pragma once
#include "main.h"

class Texture
{
private:
    IDirect3DDevice9 *device;
    IDirect3DTexture9 *texture;
    IDirect3DSurface9 *old_surface;
public:
    Texture(IDirect3DDevice9 *device, unsigned width, unsigned height);
    void set_as_target();
    void unset_as_target();
    void set();
    ~Texture();
};