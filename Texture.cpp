#include "Texture.h"

Texture::Texture(IDirect3DDevice9 *device, unsigned width, unsigned height)
: device(device), texture(NULL), width(width), height(height), old_surface(NULL)
{
    check_texture( D3DXCreateTexture(device, width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R8G8B8, D3DPOOL_DEFAULT, &texture) );
}

void Texture::set(unsigned samplers_count /*= 1*/)
{
    for( unsigned i = 0; i < samplers_count; ++i )
    {
        check_texture( device->SetTexture(i, texture) );
    }
}

void Texture::set_as_target()
{
    IDirect3DSurface9 *surface;
    check_texture( texture->GetSurfaceLevel(0, &surface) );
    check_texture( device->GetRenderTarget(0, &old_surface) ); // Store old
    check_texture( device->SetRenderTarget(0, surface) );
    release_interface(surface);
}

void Texture::unset_as_target()
{
    check_texture( device->SetRenderTarget(0, old_surface) ); // Restore old
    release_interface(old_surface);
}

Texture::~Texture()
{
    release_interface(texture);
    release_interface(old_surface);
}
