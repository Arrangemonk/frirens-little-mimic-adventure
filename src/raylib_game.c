/*******************************************************************************************
*
*   raylib game template
*
*   <Game title>
*   <Game description>
*
*   This game has been created using raylib (www.raylib.com)
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
*   Copyright (c) 2021 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"
#include "raymath.h"
#include "screens.h"    // NOTE: Declares global (extern) variables and screens functions

#if defined(PLATFORM_DESKTOP)
#define GLSL_VERSION            330
#elif defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#define GLSL_VERSION            100
#else   // PLATFORM_ANDROID
#define GLSL_VERSION            100
#endif

#define MAP_SIZE 10
//----------------------------------------------------------------------------------
// Shared Variables Definition (global)
// NOTE: Those variables are shared between modules through screens.h
//----------------------------------------------------------------------------------
GameScreen currentScreen = LOGO;
Font font = { 0 };
float volume[8];
Music music[8];
Model dungeon;
Model chest;
Material material = { 0 };
Material cmaterial = { 0 };
Camera camera = { 0 };

//----------------------------------------------------------------------------------
// Local Variables Definition (local to this module)
//----------------------------------------------------------------------------------
static const int screenWidth = 1280;
static const int screenHeight = 720;

// Required variables to manage screen transitions (fade-in, fade-out)
static float transAlpha = 0.0f;
static bool onTransition = false;
static bool transFadeOut = false;
static int transFromScreen = -1;
static GameScreen transToScreen = UNKNOWN;

//----------------------------------------------------------------------------------
// Local Functions Declaration
//----------------------------------------------------------------------------------
static void ChangeToScreen(int screen);     // Change to screen, no transition effect

static void TransitionToScreen(int screen); // Request transition to next screen
static void UpdateTransition(void);         // Update transition effect
static void DrawTransition(void);           // Draw transition effect (full-screen rectangle)

static void UpdateDrawFrame(void);          // Update and draw one frame

static void setvolume(Music* music,float* volume,int i);
static void updateMusic(Music* music,float* volume)
{
    float timePlayed = GetMusicTimePlayed(music[0])/GetMusicTimeLength(music[0]);

    if (timePlayed > 1.0f){
        timePlayed = 1.0f;   // Make sure time played is no longer than music
    }
    if(fmod(timePlayed,0.5f) < 0.05f)
    {
        for(int i = 0;i < 8; i++)
        {
            SetMusicVolume(music[i],volume[i]);
        }
    }
}

//----------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //---------------------------------------------------------
    SetConfigFlags(FLAG_MSAA_4X_HINT);  // Enable Multi Sampling Anti Aliasing 4x (if available)
    InitWindow(screenWidth, screenHeight, "Frirens little Mimic Adventure");

    InitAudioDevice();      // Initialize audio device

    // Load global data (assets that must be available in all screens, i.e. font)
    font = LoadFont("resources/mecha.png");
    for(int i = 0; i< 8; i++)
    {
        music[i] = LoadMusicStream(TextFormat("resources/%1i.wav",i));
        volume[i] = 0.0f;
        PlayMusicStream(music[i]);
    }
    volume[6] = 1.0f;
    volume[1] = 1.0f;

    // Define the camera to look into our 3d world
    camera.position = (Vector3){ 0.0f, 2.0f, -5.5f };    // Camera position
    camera.target = (Vector3){ 0.0f, 2.0f, -10.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

    dungeon = LoadModel("resources/dungeon.gltf");
    chest = LoadModel("resources/chest.obj");

    // Load lightmap shader
    Shader shader = LoadShader(TextFormat("resources/glsl%i/lightmap.vs", GLSL_VERSION),
                               TextFormat("resources/glsl%i/lightmap.fs", GLSL_VERSION));

    Shader cshader = LoadShader(TextFormat("resources/glsl%i/chest.vs", GLSL_VERSION),
                               TextFormat("resources/glsl%i/chest.fs", GLSL_VERSION));

    Texture texture = LoadTexture("resources/dungeon_dif.png");
    Texture light = LoadTexture("resources/dungeon_lit.png");

    Texture ctexture = LoadTexture("resources/chest_dif.png");
    Texture clight = LoadTexture("resources/chest_lit.png");

    GenTextureMipmaps(&texture);
    SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);

    GenTextureMipmaps(&light);
    SetTextureFilter(light, TEXTURE_FILTER_TRILINEAR);

    GenTextureMipmaps(&ctexture);
    SetTextureFilter(ctexture, TEXTURE_FILTER_TRILINEAR);

    GenTextureMipmaps(&clight);
    SetTextureFilter(clight, TEXTURE_FILTER_TRILINEAR);

    material = LoadMaterialDefault();
    material.shader = shader;
    material.maps[MATERIAL_MAP_ALBEDO].texture = texture;
    material.maps[MATERIAL_MAP_METALNESS].texture = light;

    cmaterial = LoadMaterialDefault();
    cmaterial.shader = cshader;
    cmaterial.maps[MATERIAL_MAP_ALBEDO].texture = ctexture;
    cmaterial.maps[MATERIAL_MAP_METALNESS].texture = clight;
    chest.materials[0] = cmaterial;

    // Setup and init first screen
    currentScreen = LOGO;
    InitLogoScreen();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);       // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        UpdateDrawFrame();
    }
#endif

    // De-Initialization
    //--------------------------------------------------------------------------------------
    // Unload current screen data before closing
    switch (currentScreen)
    {
        case LOGO: UnloadLogoScreen(); break;
        case TITLE: UnloadTitleScreen(); break;
        case GAMEPLAY: UnloadGameplayScreen(); break;
        case ENDING: UnloadEndingScreen(); break;
        default: break;
    }

    // Unload global data loaded
    UnloadFont(font);
    for(int i = 0; i< 8; i++)
    {
        UnloadMusicStream(music[i]);   // Unload music stream buffers from RAM
    }
    UnloadModel(dungeon);       // Unload the mesh
    UnloadShader(shader);   // Unload shader
    UnloadTexture(texture);
    UnloadTexture(light);

    UnloadModel(chest);       // Unload the mesh
    UnloadShader(cshader);   // Unload shader
    UnloadTexture(ctexture);
    UnloadTexture(clight);

    CloseAudioDevice();     // Close audio context

    CloseWindow();          // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//----------------------------------------------------------------------------------
// Module specific Functions Definition
//----------------------------------------------------------------------------------
// Change to next screen, no transition
static void ChangeToScreen(GameScreen screen)
{
    // Unload current screen
    switch (currentScreen)
    {
        case LOGO: UnloadLogoScreen(); break;
        case TITLE: UnloadTitleScreen(); break;
        case GAMEPLAY: UnloadGameplayScreen(); break;
        case ENDING: UnloadEndingScreen(); break;
        default: break;
    }

    // Init next screen
    switch (screen)
    {
        case LOGO: InitLogoScreen(); break;
        case TITLE: InitTitleScreen(); break;
        case GAMEPLAY: InitGameplayScreen(); break;
        case ENDING: InitEndingScreen(); break;
        default: break;
    }

    currentScreen = screen;
}

// Request transition to next screen
static void TransitionToScreen(GameScreen screen)
{
    onTransition = true;
    transFadeOut = false;
    transFromScreen = currentScreen;
    transToScreen = screen;
    transAlpha = 0.0f;
}

// Update transition effect (fade-in, fade-out)
static void UpdateTransition(void)
{
    if (!transFadeOut)
    {
        transAlpha += 0.05f;

        // NOTE: Due to float internal representation, condition jumps on 1.0f instead of 1.05f
        // For that reason we compare against 1.01f, to avoid last frame loading stop
        if (transAlpha > 1.01f)
        {
            transAlpha = 1.0f;

            // Unload current screen
            switch (transFromScreen)
            {
                case LOGO: UnloadLogoScreen(); break;
                case TITLE: UnloadTitleScreen(); break;
                case OPTIONS: UnloadOptionsScreen(); break;
                case GAMEPLAY: UnloadGameplayScreen(); break;
                case ENDING: UnloadEndingScreen(); break;
                default: break;
            }

            // Load next screen
            switch (transToScreen)
            {
                case LOGO: InitLogoScreen(); break;
                case TITLE: InitTitleScreen(); break;
                case GAMEPLAY: InitGameplayScreen(); break;
                case ENDING: InitEndingScreen(); break;
                default: break;
            }

            currentScreen = transToScreen;

            // Activate fade out effect to next loaded screen
            transFadeOut = true;
        }
    }
    else  // Transition fade out logic
    {
        transAlpha -= 0.02f;

        if (transAlpha < -0.01f)
        {
            transAlpha = 0.0f;
            transFadeOut = false;
            onTransition = false;
            transFromScreen = -1;
            transToScreen = UNKNOWN;
        }
    }
}

// Draw transition effect (full-screen rectangle)
static void DrawTransition(void)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, transAlpha));
}

void setvolume(Music* music,float* volume,int i)
{
    volume[i] = 1.0f -volume[i];
}

// Update and draw game frame
static void UpdateDrawFrame(void)
{
    // Update
    //----------------------------------------------------------------------------------
    for(int i = 0; i< 8; i++)
    {
        UpdateMusicStream(music[i]);
        if(IsKeyPressed(KEY_ONE + i))
            setvolume(music,volume,i);
    }
    updateMusic(music,volume);

    if (!onTransition)
    {
        switch(currentScreen)
        {
            case LOGO:
            {
                UpdateLogoScreen();

                if (FinishLogoScreen()) TransitionToScreen(TITLE);

            } break;
            case TITLE:
            {
                UpdateTitleScreen();

                if (FinishTitleScreen() == 1) TransitionToScreen(OPTIONS);
                else if (FinishTitleScreen() == 2) TransitionToScreen(GAMEPLAY);

            } break;
            case OPTIONS:
            {
                UpdateOptionsScreen();

                if (FinishOptionsScreen()) TransitionToScreen(TITLE);

            } break;
            case GAMEPLAY:
            {
                UpdateGameplayScreen();

                if (FinishGameplayScreen() == 1) TransitionToScreen(ENDING);
                //else if (FinishGameplayScreen() == 2) TransitionToScreen(TITLE);

            } break;
            case ENDING:
            {
                UpdateEndingScreen();

                if (FinishEndingScreen() == 1) TransitionToScreen(TITLE);

            } break;
            default: break;
        }
    }
    else UpdateTransition();    // Update transition (fade-in, fade-out)
    //----------------------------------------------------------------------------------
    UpdateCamera(&camera, CAMERA_ORBITAL);
    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        DrawMesh(dungeon.meshes[0],material,MatrixIdentity());
        //DrawMesh(chest.meshes[0],cmaterial,MatrixIdentity());
        DrawModel(chest, Vector3Zero(),1.0f,WHITE);
        EndMode3D();

        switch(currentScreen)
        {
            case LOGO: DrawLogoScreen(); break;
            case TITLE: DrawTitleScreen(); break;
            case OPTIONS: DrawOptionsScreen(); break;
            case GAMEPLAY: DrawGameplayScreen(); break;
            case ENDING: DrawEndingScreen(); break;
            default: break;
        }

        // Draw full screen rectangle in front of everything
        if (onTransition) DrawTransition();

        //DrawFPS(10, 10);

    EndDrawing();
    //----------------------------------------------------------------------------------
}
