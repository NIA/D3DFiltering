#pragma once
#include "main.h"

class Texture
{
private:
    IDirect3DDevice9 *device;
    IDirect3DTexture9 *texture;
    IDirect3DSurface9 *old_surface;

    unsigned width, height;
    unsigned samplers_count;
public:
    Texture(IDirect3DDevice9 *device, unsigned width, unsigned height, unsigned samplers_count = 1);
    void set_as_target();
    void unset_as_target();
    void set();
    float get_float_width() { return static_cast<float>( width ); }
    float get_float_height() { return static_cast<float>( height ); }
    ~Texture();
};
