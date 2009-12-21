#include "Texture.h"

Texture::Texture(IDirect3DDevice9 *device, unsigned width, unsigned height)
: device(device), texture(NULL), width(width), height(height)
{
    check_texture( D3DXCreateTexture(device, width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R8G8B8, D3DPOOL_DEFAULT, &texture) );
    old_surfaces[0] = old_surfaces[1] = NULL;
}

void Texture::set(unsigned samplers_count /*= 1*/)
{
    for( unsigned i = 0; i < samplers_count; ++i )
    {
        check_texture( device->SetTexture(i, texture) );
    }
}

void Texture::set_as_target(unsigned index /*= 0*/)
{
    _ASSERT( index <= 1 );
    IDirect3DSurface9 *surface;
    check_texture( texture->GetSurfaceLevel(0, &surface) );
    if( 0 == index)
    {
        check_texture( device->GetRenderTarget(index, &old_surfaces[index]) ); // Store old
    }
    else
    {
        old_surfaces[index] = NULL;
    }
    check_texture( device->SetRenderTarget(index, surface) );
    release_interface(surface);
}

void Texture::unset_as_target(unsigned index /*= 0*/)
{
    _ASSERT( index <= 1 );
    check_texture( device->SetRenderTarget(index, old_surfaces[index]) ); // Restore old
    release_interface(old_surfaces[index]);
}

Texture::~Texture()
{
    release_interface(texture);
    release_interface(old_surfaces[0]);
    release_interface(old_surfaces[1]);
}
