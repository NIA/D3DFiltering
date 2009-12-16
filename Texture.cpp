#include "Texture.h"

Texture::Texture(IDirect3DDevice9 *device, unsigned width, unsigned height)
: device(device), texture(NULL)
{
    check_texture( D3DXCreateTexture(device, width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R8G8B8, D3DPOOL_DEFAULT, &texture) );
    check_texture( device->SetTextureStageState(0, D3DTSS_COLOROP,  D3DTOP_MODULATE) );
    check_texture( device->SetTextureStageState(0, D3DTSS_COLORARG1,D3DTA_TEXTURE) );
    check_texture( device->SetTextureStageState(0, D3DTSS_COLORARG2,D3DTA_DIFFUSE) );
    check_texture( device->SetTextureStageState(0, D3DTSS_ALPHAOP,  D3DTOP_DISABLE) );
}

void Texture::set()
{
    check_texture( device->SetTexture(0, texture) );
}

void Texture::set_as_target()
{
    IDirect3DSurface9 *surface;
    check_texture( texture->GetSurfaceLevel(0, &surface) );
    check_texture( device->GetRenderTarget(0, &old_surface) ); // Store old
    check_texture( device->SetRenderTarget(0, surface) );
}

void Texture::unset_as_target()
{
    check_texture( device->SetRenderTarget(0, old_surface) ); // Restore old
}

Texture::~Texture()
{
    release_interface(texture);
}