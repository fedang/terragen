#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>
#include <string.h>
#include <rlgl.h>

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

static Color heightToColor(float noise)
{
    if (noise > 0.5)
        return DARKGRAY;

    if (noise > 0.3)
        return DARKBROWN;

    if (noise > 0.15)
        return BROWN;

    if (noise > 0)
        return DARKGREEN;

    if (noise > -0.2)
        return SKYBLUE;

    return DARKBLUE;
}

static Mesh GenerateMesh(int longitudeSlices, int latitudeSlices, float radius, float scale, float lacunarity, float gain, int octaves)
{
    Mesh mesh = { 0 };
    mesh.triangleCount = longitudeSlices * (latitudeSlices - 1) * 2;
    mesh.vertexCount = (longitudeSlices + 1) * (latitudeSlices + 1);

    mesh.vertices = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.colors = (unsigned char *)MemAlloc(mesh.vertexCount * 4 * sizeof(unsigned char));
    mesh.indices = (unsigned short *)MemAlloc(mesh.triangleCount * 3 * sizeof(unsigned short));
    mesh.normals = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));

    // TODO
    mesh.texcoords = (float*)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
    memset(mesh.texcoords, 0, mesh.vertexCount * 2 * sizeof(float));

    const float lengthInv = 1.0f / radius;
    const float longitudeStep = 2 * PI / longitudeSlices;
    const float latitudeStep = PI / latitudeSlices;

    for (int i = 0, v = 0; i <= latitudeSlices; i++) {

        float latitudeAngle = PI / 2 - i * latitudeStep;
        float latitudeCos = cosf(latitudeAngle);
        float latitudeSin = sinf(latitudeAngle);
        float z = radius * latitudeSin;

        for (int j = 0; j <= longitudeSlices; j++, v++) {

            float longitudeAngle = j * longitudeStep;
            float longitudeCos = cosf(longitudeAngle);
            float longitudeSin = sinf(longitudeAngle);

            float x = radius * latitudeCos * longitudeCos;
            float y = radius * latitudeCos * longitudeSin;

            float noise = stb_perlin_fbm_noise3(x / scale, y / scale, z / scale, lacunarity, gain, octaves);

            float offsetX = noise * latitudeCos * longitudeCos;
            float offsetY = noise * latitudeCos * longitudeSin;
            float offsetZ = noise * latitudeSin;

            mesh.vertices[v * 3 + 0] = x + offsetX;
            mesh.vertices[v * 3 + 1] = y + offsetY;
            mesh.vertices[v * 3 + 2] = z + offsetZ;

            mesh.normals[v * 3 + 0] = mesh.vertices[v * 3 + 0] * lengthInv;
            mesh.normals[v * 3 + 1] = mesh.vertices[v * 3 + 1] * lengthInv;
            mesh.normals[v * 3 + 2] = mesh.vertices[v * 3 + 2] * lengthInv;

            Color color = heightToColor(noise);

            mesh.colors[v * 4 + 0] = color.r;
            mesh.colors[v * 4 + 1] = color.g;
            mesh.colors[v * 4 + 2] = color.b;
            mesh.colors[v * 4 + 3] = color.a;
        }
    }

    for (int i = 0, v = 0; i < latitudeSlices; i++) {

        int k1 = i * (longitudeSlices + 1);
        int k2 = k1 + longitudeSlices + 1;

        for (int j = 0; j < longitudeSlices; j++, k1++, k2++) {

            // k1 => k2 => k1+1
            if (i != 0) {
                mesh.indices[v++] = k1;
                mesh.indices[v++] = k2;
                mesh.indices[v++] = k1 + 1;
            }

            // k1+1 => k2 => k2+1
            if (i != latitudeSlices - 1) {
                mesh.indices[v++] = k1 + 1;
                mesh.indices[v++] = k2;
                mesh.indices[v++] = k2 + 1;
            }
        }
    }

    //ExportMesh(mesh, "mesh.obj");

    UploadMesh(&mesh, false);

    return mesh;
}

static RenderTexture2D LoadShadowmapTexture(int width, int height)
{
    RenderTexture2D target = { 0 };

    target.id = rlLoadFramebuffer(width, height);
    target.texture.width = width;
    target.texture.height = height;

    if (target.id > 0) {
        rlEnableFramebuffer(target.id);

        target.depth.id = rlLoadTextureDepth(width, height, false);
        target.depth.width = width;
        target.depth.height = height;
        target.depth.format = 24;
        target.depth.mipmaps = 1;

        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

        if (rlFramebufferComplete(target.id))
            TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", target.id);

        rlDisableFramebuffer();
    }
    else
        TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");

    return target;
}

int main(int argc, char **argv)
{
    int screenWidth = 1000;
    int screenHeight = 800;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "terragen");

    Camera camera = { 0 };
    camera.position = (Vector3){ 50.0f, 50.0f, 50.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    int longitudeSlices = 200;
    int latitudeSlices = 200;

    float radius = 10;
    float scale = 4;
    float lacunarity = 2;
    float gain = 0.5;
    int octaves = 6;

    Shader shadowShader = LoadShader("shadowmap.vs", "shadowmap.fs");
    shadowShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shadowShader, "viewPos");

    Vector3 lightDir = Vector3Normalize((Vector3){ 0.35f, -1.0f, -0.35f });
    int lightDirLoc = GetShaderLocation(shadowShader, "lightDir");
    SetShaderValue(shadowShader, lightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);

    Color lightColor = { 237, 221, 128, 255 };
    Vector4 lightColorNormalized = ColorNormalize(lightColor);
    int lightColLoc = GetShaderLocation(shadowShader, "lightColor");
    SetShaderValue(shadowShader, lightColLoc, &lightColorNormalized, SHADER_UNIFORM_VEC4);

    int ambientLoc = GetShaderLocation(shadowShader, "ambient");
    float ambient[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    SetShaderValue(shadowShader, ambientLoc, ambient, SHADER_UNIFORM_VEC4);

    int shadowMapResolution = 1024;
    SetShaderValue(shadowShader, GetShaderLocation(shadowShader, "shadowMapResolution"), &shadowMapResolution, SHADER_UNIFORM_INT);

    int lightVPLoc = GetShaderLocation(shadowShader, "lightVP");
    int shadowMapLoc = GetShaderLocation(shadowShader, "shadowMap");

    // TODO: Use fibonacci/cube sphere instead of UV
    Mesh mesh = GenerateMesh(longitudeSlices, latitudeSlices, radius, scale, lacunarity, gain, octaves);
    Matrix meshTransform = MatrixIdentity();
    Material material = LoadMaterialDefault();
    material.shader = shadowShader;

    RenderTexture2D shadowMap = LoadShadowmapTexture(shadowMapResolution, shadowMapResolution);

    Camera3D lightCam = { 0 };
    lightCam.position = Vector3Scale(lightDir, -15.0f);
    lightCam.target = Vector3Zero();
    lightCam.projection = CAMERA_ORTHOGRAPHIC;
    lightCam.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    lightCam.fovy = 20.0f;

    bool menu = false;

    while (!WindowShouldClose()) {
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();

        // Update cameras
        Vector3 cameraPos = camera.position;
        SetShaderValue(shadowShader, shadowShader.locs[SHADER_LOC_VECTOR_VIEW], &cameraPos, SHADER_UNIFORM_VEC3);
        UpdateCamera(&camera, CAMERA_ORBITAL);

        // Update menu
        if (IsKeyPressed(KEY_ESCAPE))
            menu = !menu;

        // Update light
        lightDir = Vector3Normalize(lightDir);
        lightCam.position = Vector3Scale(lightDir, -15.0f);
        SetShaderValue(shadowShader, lightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);

        BeginDrawing();

            Matrix lightView;
            Matrix lightProj;

            BeginTextureMode(shadowMap);
                ClearBackground(WHITE);

                BeginMode3D(lightCam);

                    lightView = rlGetMatrixModelview();
                    lightProj = rlGetMatrixProjection();
                    DrawMesh(mesh, material, meshTransform);

                EndMode3D();
            EndTextureMode();

            Matrix lightViewProj = MatrixMultiply(lightView, lightProj);
            SetShaderValueMatrix(shadowShader, lightVPLoc, lightViewProj);

            rlEnableShader(shadowShader.id);
            int slot = 10;
            rlActiveTextureSlot(slot);
            rlEnableTexture(shadowMap.depth.id);
            rlSetUniform(shadowMapLoc, &slot, SHADER_UNIFORM_INT, 1);

            ClearBackground(RAYWHITE);

            BeginMode3D(camera);

                DrawMesh(mesh, material, meshTransform);

            EndMode3D();

            if (menu) {
                Color infoColor = Fade(LIGHTGRAY, 0.6f);

                int paddingX = screenWidth / 8;
                int paddingY = screenHeight / 8;

                DrawRectangle(paddingX, paddingY, screenWidth - 2 * paddingX, screenHeight - 2 * paddingY, infoColor);

                float fontSize = Clamp(screenHeight / 100.0 * 3.2, 16, 40);

                DrawText("perlin noise", paddingX * 2, paddingY * 2, fontSize * 1.2, BLACK);
                int spacing = fontSize * 2;

                DrawText(TextFormat("scale: %f", scale), paddingX * 2, paddingY * 2 + spacing, fontSize, BLACK);
                spacing += fontSize;

                DrawText(TextFormat("lacunarity: %f", lacunarity), paddingX * 2, paddingY * 2 + spacing, fontSize, BLACK);
                spacing += fontSize;

                DrawText(TextFormat("gain: %f", gain), paddingX * 2, paddingY * 2 + spacing, fontSize, BLACK);
                spacing += fontSize;

                DrawText(TextFormat("octaves: %d", octaves), paddingX * 2, paddingY * 2 + spacing, fontSize, BLACK);
                spacing += fontSize * 2;

                DrawText("window", paddingX * 2, paddingY * 2 + spacing, fontSize * 1.2, BLACK);
                spacing += fontSize * 2;

                DrawText(TextFormat("width: %d", screenWidth), paddingX * 2, paddingY * 2 + spacing, fontSize, BLACK);
                spacing += fontSize;

                DrawText(TextFormat("height: %d", screenHeight), paddingX * 2, paddingY * 2 + spacing, fontSize, BLACK);
                spacing += fontSize;

                DrawText(TextFormat("shadowmap resolution: %d", shadowMapResolution), paddingX * 2, paddingY * 2 + spacing, fontSize, BLACK);
                spacing += fontSize * 2;

                DrawText("mesh", paddingX * 2, paddingY * 2 + spacing, fontSize * 1.2, BLACK);
                spacing += fontSize * 2;

                DrawText(TextFormat("triangles: %d", mesh.triangleCount), paddingX * 2, paddingY * 2 + spacing, fontSize, BLACK);
                spacing += fontSize;

                DrawText(TextFormat("vertices: %d", mesh.vertexCount), paddingX * 2, paddingY * 2 + spacing, fontSize, BLACK);
                spacing += fontSize;

                DrawText(TextFormat("longitude slices: %d", longitudeSlices), paddingX * 2, paddingY * 2 + spacing, fontSize, BLACK);
                spacing += fontSize;

                DrawText(TextFormat("latitude slices: %d", latitudeSlices), paddingX * 2, paddingY * 2 + spacing, fontSize, BLACK);
                spacing += fontSize;
            }

        EndDrawing();
    }

    if (shadowMap.id > 0)
        rlUnloadFramebuffer(shadowMap.id);

    UnloadMesh(mesh);
    UnloadMaterial(material);

    CloseWindow();

    return 0;
}
