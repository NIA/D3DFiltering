#include "Application.h"
#include <time.h>

const unsigned VECTORS_IN_MATRIX = sizeof(D3DXMATRIX)/sizeof(D3DXVECTOR4);

namespace
{
    const int         WINDOW_SIZE = 600;
    const D3DCOLOR    BACKGROUND_COLOR = D3DCOLOR_XRGB( 15, 15, 25 );
    const bool        INITIAL_WIREFRAME_STATE = false;
    const D3DCOLOR    BLACK = D3DCOLOR_XRGB( 0, 0, 0 );
    const float       ROTATE_STEP = D3DX_PI/30.0f;
    const float       POINT_MOVING_STEP = 0.03f;
    const char       *SHADOW_SHADER_FILENAME = "shadow.vsh";
    const DWORD       STENCIL_REF_VALUE = 50;
    const unsigned    FILTER_SIZE = 3;
    const unsigned    FILTER_REGS_COUNT = 5;


    //---------------- VERTEX SHADER CONSTANTS ---------------------------
    //    c0-c3 is the view matrix
    const unsigned    SHADER_REG_VIEW_MX = 0;
    //    c4-c7 is the first bone matrix for SKINNING
    //    c8-c11 is the second bone matrix for SKINNING
    //    c4 is final radius for MORPHING
    //    c5 is MORPHING parameter
    const unsigned    SHADER_REG_MODEL_DATA = 4;
    const unsigned    SHADER_SPACE_MODEL_DATA = 8; // number of registers available for
    //    c14 is diffuse coefficient
    const unsigned    SHADER_REG_DIFFUSE_COEF = 14;
    const float       SHADER_VAL_DIFFUSE_COEF = 0.7f;
    //    c15 is ambient color
    const unsigned    SHADER_REG_AMBIENT_COLOR = 15;
    const D3DCOLOR    SHADER_VAL_AMBIENT_COLOR = D3DCOLOR_XRGB(20, 20, 20);
    //    c16 is point light color
    const unsigned    SHADER_REG_POINT_COLOR = 16;
    const D3DCOLOR    SHADER_VAL_POINT_COLOR = D3DCOLOR_XRGB(204, 204, 100);
    //    c17 is point light position
    const unsigned    SHADER_REG_POINT_POSITION = 17;
    const D3DXVECTOR3 SHADER_VAL_POINT_POSITION  (0.2f, -0.91f, 1.5f);
    //    c18 are attenuation constants
    const unsigned    SHADER_REG_ATTENUATION = 18;
    const D3DXVECTOR3 SHADER_VAL_ATTENUATION  (1.0f, 0, 0.3f);
    //    c19 is specular coefficient
    const unsigned    SHADER_REG_SPECULAR_COEF = 19;
    const float       SHADER_VAL_SPECULAR_COEF = 0.4f;
    //    c20 is specular constant 'f'
    const unsigned    SHADER_REG_SPECULAR_F = 20;
    const float       SHADER_VAL_SPECULAR_F = 35.0f;
    //    c21 is eye position
    const unsigned    SHADER_REG_EYE = 21;
    //    c27-c30 is position and rotation of model matrix
    const unsigned    SHADER_REG_POS_AND_ROT_MX = 27;
    //    c31-c34 is shadow projection matrix
    const unsigned    SHADER_REG_SHADOW_PROJ_MX = 31;
    //    c35 are attenuation constants
    const unsigned    SHADER_REG_SHADOW_ATTENUATION = 35;
    const D3DXVECTOR3 SHADER_VAL_SHADOW_ATTENUATION  (0.8f, 0, 0.1f);
    //    c40 - c44 are filter cells shift coordinates of ... (needed to be multiplied by 1/w and 1/h)
    const unsigned    SHADER_REG_FILTER_TEXCOORD_SHIFT = 40;
    const D3DXVECTOR3 SHADER_VAL_FILTER_TEXCOORD_SHIFT[FILTER_REGS_COUNT] =
    {
        D3DXVECTOR3(  0.5f, -0.5f, 0 ),
        D3DXVECTOR3( -0.5f,  0.5f, 0 ),
        D3DXVECTOR3(  0.5f,  0.5f, 0 ),
        D3DXVECTOR3(  1.5f,  0.5f, 0 ),
        D3DXVECTOR3(  0.5f,  1.5f, 0 ),
    };

    const float       NO_FILTER[FILTER_SIZE*FILTER_SIZE] = 
    {
         0,  0,  0,
         0,  1,  0,
         0,  0,  0,
    };
    const float       BLUR_FILTER[FILTER_SIZE*FILTER_SIZE] = 
    {
              0,  1.0f/6,       0,
         1.0f/6,  1.0f/3,  1.0f/6,
              0,  1.0f/6,       0,
    };
    const float       EDGE_FILTER[FILTER_SIZE*FILTER_SIZE] = 
    {
         0, -1,  0,
        -1,  4, -1,
         0, -1,  0,
    };
    const float FILTER_COEFF = 4;//8; // each constant is divided by FILTER_COEFF before sending to pixel shader
    //---------------- VERTEX SHADER CONSTANTS ---------------------------
    //    c0 - c4 are filter values of ...
    const unsigned    SHADER_REG_FILTER = 0;
    const unsigned    SHADER_VAL_INDEX_FILTER[FILTER_REGS_COUNT] =
    {
        1, // ... ( 0, -1)
        3, // ... (-1,  0)
        4, // ... ( 0,  0)
        5, // ... ( 1,  0)
        7, // ... ( 0,  1)
    };

    enum TargetIndex
    {
        TARGET_INDEX_COLOR,
        TARGET_INDEX_NORMALS,
    };
    const unsigned SAMPLER_INDEX_MODELS = 0;
    const unsigned SAMPLER_INDEX_TARGET = 0;
    const unsigned SAMPLER_INDEX_EDGES = 1;

    const char *TARGET_EDGES_PIXEL_SHADER_FILENAME = "target_edge.psh";
    const char *TARGET_BLUR_PIXEL_SHADER_FILENAME = "target_blur.psh";
}

Texture *create_texture(IDirect3DDevice9 *device, RECT const &rect)
{
    return new Texture(device, rect.right - rect.left, rect.bottom - rect.top);
}

Application::Application()
: d3d(NULL), device(NULL), window(WINDOW_SIZE, WINDOW_SIZE), camera(2.6f, 0.68f, 0), // Constants selected for better view of the scene
  point_light_enabled(true), ambient_light_enabled(true), point_light_position(SHADER_VAL_POINT_POSITION),
  plane(NULL), light_source(NULL), target_texture(NULL), target_plane(NULL), edges_texture(NULL),
  normals_texture(NULL), second_blur_texture(NULL), target_blur_pixel_shader(NULL), target_edges_pixel_shader(NULL),
  filter(NO_FILTER), do_filtering(false), multiple_blur_factor(0)
{
    try
    {
        init_device();
        RECT rect = window.get_client_rect();
        target_texture = create_texture(device, rect);
        edges_texture = create_texture(device, rect);
        normals_texture = create_texture(device, rect);
        second_blur_texture = create_texture(device, rect);
        target_edges_pixel_shader = new PixelShader(device, TARGET_EDGES_PIXEL_SHADER_FILENAME);
        target_blur_pixel_shader =  new PixelShader(device, TARGET_BLUR_PIXEL_SHADER_FILENAME);
    }
    // using catch(...) because every caught exception is rethrown
    catch(...)
    {
        release_interfaces();
        throw;
    }
}

void Application::init_device()
{
    d3d = Direct3DCreate9( D3D_SDK_VERSION );
    if( NULL == d3d )
    {
        throw D3DInitError();
    }

    // Set up the structure used to create the device
    D3DPRESENT_PARAMETERS present_parameters;
    ZeroMemory( &present_parameters, sizeof( present_parameters ) );
    present_parameters.Windowed = TRUE;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferFormat = D3DFMT_UNKNOWN;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;
    // Create the device
    if( FAILED( d3d->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
                                      D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                      &present_parameters, &device ) ) )
    {
        throw D3DInitError();
    }

    set_render_state( D3DRS_CULLMODE, D3DCULL_NONE );
    // Enable stencil testing
    set_render_state( D3DRS_STENCILENABLE, TRUE );
    // Set the comparison reference value
    set_render_state( D3DRS_STENCILREF, STENCIL_REF_VALUE );
    //  Specify a stencil mask 
    set_render_state( D3DRS_STENCILMASK, 0xff);
    //  Configure alpha-blending
    set_render_state( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
    set_render_state( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );

    check_texture( device->SetTextureStageState(0, D3DTSS_COLOROP,  D3DTOP_MODULATE) );
    check_texture( device->SetTextureStageState(0, D3DTSS_COLORARG1,D3DTA_TEXTURE) );
    check_texture( device->SetTextureStageState(0, D3DTSS_COLORARG2,D3DTA_DIFFUSE) );
    check_texture( device->SetTextureStageState(0, D3DTSS_ALPHAOP,  D3DTOP_DISABLE) );

    toggle_wireframe();
}

inline void Application::draw_model(Model *model, float time, bool shadow)
{
    static D3DXVECTOR4 model_constants[SHADER_SPACE_MODEL_DATA];
    static unsigned constants_used;


    // Setting constants
    model->set_time( time );
    constants_used = model->set_constants( model_constants, array_size(model_constants) );
    set_shader_const( SHADER_REG_MODEL_DATA, *model_constants, constants_used );
    set_shader_matrix( SHADER_REG_POS_AND_ROT_MX, model->get_rotation_and_position() );
    
    // Draw
    model->set_shaders_and_decl(shadow);
    model->set_textures(shadow, SAMPLER_INDEX_MODELS);
    model->draw();
}

void Application::render_scene(float time)
{
    // Draw Light Source
    draw_model( light_source, time, false );
    // Draw Plane
    draw_model( plane, time, false );
    // Draw models
    for ( Models::iterator iter = models.begin(); iter != models.end(); ++iter )
    {
        draw_model( *iter, time, false );
    }
}
void Application::set_filter(const float *filter)
{
    D3DXVECTOR3 texcoord_multiplier (1.0f/target_texture->get_float_width(), 1.0f/target_texture->get_float_height(), 0);

    for( unsigned i = 0; i < FILTER_REGS_COUNT; ++i )
    {
        D3DXVECTOR3 texcoord_shift( 0, 0, 0 );
        texcoord_shift.x = SHADER_VAL_FILTER_TEXCOORD_SHIFT[i].x*texcoord_multiplier.x;
        texcoord_shift.y = SHADER_VAL_FILTER_TEXCOORD_SHIFT[i].x*texcoord_multiplier.y;
        set_shader_vector( SHADER_REG_FILTER_TEXCOORD_SHIFT + i, texcoord_shift );

        set_pixel_shader_float( SHADER_REG_FILTER + i, filter[ SHADER_VAL_INDEX_FILTER[i] ]/FILTER_COEFF );
    }
}

void Application::render()
{
    // Begin the scene
    check_render( device->BeginScene() );
    // Setting constants
    float time = static_cast<float>( clock() )/static_cast<float>( CLOCKS_PER_SEC );

    D3DCOLOR ambient_color = ambient_light_enabled ? SHADER_VAL_AMBIENT_COLOR : BLACK;
    D3DCOLOR point_color = point_light_enabled ? SHADER_VAL_POINT_COLOR : BLACK;
    D3DXMATRIX shadow_proj_matrix = plane->get_projection_matrix(point_light_position);

    set_shader_matrix( SHADER_REG_VIEW_MX,        camera.get_matrix()       );
    set_shader_float ( SHADER_REG_DIFFUSE_COEF,   SHADER_VAL_DIFFUSE_COEF   );
    set_shader_color ( SHADER_REG_AMBIENT_COLOR,  ambient_color             );
    set_shader_color ( SHADER_REG_POINT_COLOR,    point_color               );
    set_shader_point ( SHADER_REG_POINT_POSITION, point_light_position      );
    set_shader_vector( SHADER_REG_ATTENUATION,    SHADER_VAL_ATTENUATION    );
    set_shader_float ( SHADER_REG_SPECULAR_COEF,  SHADER_VAL_SPECULAR_COEF  );
    set_shader_float ( SHADER_REG_SPECULAR_F,     SHADER_VAL_SPECULAR_F     );
    set_shader_point ( SHADER_REG_EYE,            camera.get_eye()          );
    set_shader_matrix( SHADER_REG_SHADOW_PROJ_MX, shadow_proj_matrix        );
    set_shader_vector( SHADER_REG_SHADOW_ATTENUATION, SHADER_VAL_SHADOW_ATTENUATION );

    // Set render target
    target_texture->unset( SAMPLER_INDEX_TARGET );
    target_texture->set_as_target( TARGET_INDEX_COLOR );
    normals_texture->set_as_target( TARGET_INDEX_NORMALS );
    check_render( device->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, BACKGROUND_COLOR, 1.0f, 0 ) );

    // Render scene (color into target #0, normals into target #1)
    render_scene( time );
    // Unset render target
    target_texture->unset_as_target( TARGET_INDEX_COLOR );
    normals_texture->unset_as_target( TARGET_INDEX_NORMALS );

    // Draw target plane
    if( do_filtering )
    {
        // - - - - If filtering enabled - - - - - 
        // render edges
        set_filter( EDGE_FILTER );
        target_plane->set_shaders_and_decl(false);
        target_edges_pixel_shader->set();
        
        edges_texture->set_as_target( TARGET_INDEX_COLOR );
        normals_texture->set( SAMPLER_INDEX_TARGET );
        target_plane->draw();
        edges_texture->unset_as_target( TARGET_INDEX_COLOR );
        normals_texture->unset( SAMPLER_INDEX_TARGET );
        
        // set edges as texture
        edges_texture->set( SAMPLER_INDEX_EDGES );
        if( multiple_blur_factor != 0 )
        {
            // - - - - If multiple filtering enabled - - - - - 
            set_filter( BLUR_FILTER );
            target_blur_pixel_shader->set();
            for( unsigned i = 0; i < multiple_blur_factor; ++i )
            {
                // render blur from target_texture into second_blur_texture
                second_blur_texture->set_as_target( TARGET_INDEX_COLOR );
                target_texture->set( SAMPLER_INDEX_TARGET );
                target_plane->draw();
                target_texture->unset( SAMPLER_INDEX_TARGET );
                second_blur_texture->unset_as_target( TARGET_INDEX_COLOR );
                
                // render blur from second_blur_texture into target_texture (or, finally, into backbuffer)
                if( i != multiple_blur_factor - 1 )
                {
                    // if not the last step (because on last step we just render to backbuffer)
                    target_texture->set_as_target( TARGET_INDEX_COLOR );
                }
                second_blur_texture->set( SAMPLER_INDEX_TARGET );
                target_plane->draw();
                second_blur_texture->unset( SAMPLER_INDEX_TARGET );
                if( i != multiple_blur_factor - 1 )
                {
                    // if not the last step (because on last step we just render to backbuffer)
                    target_texture->unset_as_target( TARGET_INDEX_COLOR );
                }
            }
        }
        else
        {
            // - - - - Else - single filtering - - - - - 
            // render blur
            target_texture->set( SAMPLER_INDEX_TARGET );
            set_filter( BLUR_FILTER );
            
            target_blur_pixel_shader->set();
            target_plane->draw();
            target_texture->unset( SAMPLER_INDEX_TARGET );
        }

        // unset edges as texture
        edges_texture->unset( SAMPLER_INDEX_EDGES );
    }
    else
    {
        // - - - - Else - no filtering - - - - - 
        set_filter( NO_FILTER );
        draw_model( target_plane, time, false );
    }

    // End the scene
    check_render( device->EndScene() );
    check_render( device->Present( NULL, NULL, NULL, NULL ) );
}

IDirect3DDevice9 * Application::get_device()
{
    return device;
}

void Application::add_model(Model &model)
{
    models.push_back( &model );
}

void Application::remove_model(Model &model)
{
    models.remove( &model );
}

void Application::rotate_models(float phi)
{
    for ( Models::iterator iter = models.begin(); iter != models.end(); ++iter )
    {
        (*iter)->rotate(phi);
    }
}

void Application::process_key(unsigned code)
{
    switch( code )
    {
    case VK_ESCAPE:
        PostQuitMessage( 0 );
        break;
    case VK_UP:
        camera.move_up();
        break;
    case VK_DOWN:
        camera.move_down();
        break;
    case VK_PRIOR:
    case VK_ADD:
    case VK_OEM_PLUS:
        camera.move_nearer();
        break;
    case VK_NEXT:
    case VK_SUBTRACT:
    case VK_OEM_MINUS:
        camera.move_farther();
        break;
    case VK_LEFT:
        camera.move_clockwise();
        break;
    case VK_RIGHT:
        camera.move_counterclockwise();
        break;
    case 'A':
        point_light_position.y += POINT_MOVING_STEP;
        break;
    case 'D':
        point_light_position.y -= POINT_MOVING_STEP;
        break;
    case 'W':
        point_light_position.x -= POINT_MOVING_STEP;
        break;
    case 'S':
        point_light_position.x += POINT_MOVING_STEP;
        break;
    case 'R':
        point_light_position.z += POINT_MOVING_STEP;
        break;
    case 'F':
        point_light_position.z -= POINT_MOVING_STEP;
        break;
    case '0':
    case VK_OEM_3:
        do_filtering = false;
        break;
    case '1':
        do_filtering = true;
        multiple_blur_factor = 0;
        break;
    case '2':
        do_filtering = true;
        multiple_blur_factor = 1;
        break;
    case '3':
        do_filtering = true;
        multiple_blur_factor = 2;
        break;
    case '4':
        do_filtering = true;
        multiple_blur_factor = 4;
        break;
    case '5':
        do_filtering = true;
        multiple_blur_factor = 8;
        break;
    case '6':
        do_filtering = true;
        multiple_blur_factor = 16;
        break;
    }
}

void Application::run()
{
    if( NULL == plane )
    {
        throw NoPlaneError();
    }
    if( NULL == light_source )
    {
        throw NoLightSourceModelError();
    }
    if( NULL == target_plane )
    {
        throw NoTargetPlaneError();
    }

    window.show();
    window.update();

    // Enter the message loop
    MSG msg;
    ZeroMemory( &msg, sizeof( msg ) );
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
        {
            if( WM_KEYDOWN == msg.message )
            {
                process_key( static_cast<unsigned>( msg.wParam ) );
            }

            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            render();
        }
    }
}

void Application::toggle_wireframe()
{
    static bool wireframe = !INITIAL_WIREFRAME_STATE;
    wireframe = !wireframe;

    if( wireframe )
    {
        set_render_state( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
    }
    else
    {
        set_render_state( D3DRS_FILLMODE, D3DFILL_SOLID );
    }
}

void Application::release_interfaces()
{
    release_interface( d3d );
    release_interface( device );
    delete_pointer( &target_plane );
    delete_pointer( &target_texture );
    delete_pointer( &edges_texture );
    delete_pointer( &normals_texture );
    delete_pointer( &second_blur_texture );
    delete_pointer( &target_blur_pixel_shader );
    delete_pointer( &target_edges_pixel_shader );
}

Application::~Application()
{
    release_interfaces();
}