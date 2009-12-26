#include "Texture.h"

Texture::Texture(IDirect3DDevice9 *device, unsigned width, unsigned height)
: device(device), texture(NULL), width(width), height(height), old_surface(NULL)
{
    check_texture( D3DXCreateTexture(device, width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R8G8B8, D3DPOOL_DEFAULT, &texture) );
}

void Texture::set(unsigned samler_index /*= 0*/)
{
    check_texture( device->SetTexture(samler_index, texture) );
}

void Texture::unset(unsigned samler_index /*= 0*/)
{
    check_texture( device->SetTexture(samler_index, NULL) );
}

void Texture::set_as_target(unsigned index /*= 0*/)
{
    IDirect3DSurface9 *surface;
    check_texture( texture->GetSurfaceLevel(0, &surface) );
    if( 0 == index)
    {
        check_texture( device->GetRenderTarget(index, &old_surface) ); // Store old
    }
    check_texture( device->SetRenderTarget(index, surface) );
    release_interface(surface);
}

void Texture::unset_as_target(unsigned index /*= 0*/)
{
    if( 0 == index)
    {
        check_texture( device->SetRenderTarget(index, old_surface) ); // Restore old
        release_interface(old_surface);
    }
    else
    {
        check_texture( device->SetRenderTarget(index, NULL) ); // Set none
    }
}

Texture::~Texture()
{
    release_interface(texture);
}
