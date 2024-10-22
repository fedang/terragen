#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>
#include <string.h>

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

static Mesh GenerateMesh(float radius, float scale, float lacunarity, float gain, int octaves)
{
    const int longitudeCount = 50;
    const int latitudeCount = 50;

    Mesh mesh = { 0 };
    mesh.triangleCount = longitudeCount * (latitudeCount - 1) * 2;
    mesh.vertexCount = (longitudeCount + 1) * (latitudeCount + 1);

    mesh.vertices = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.colors = (unsigned char*)MemAlloc(mesh.vertexCount * 4 * sizeof(unsigned char));
    mesh.indices = (unsigned short*)MemAlloc(mesh.triangleCount * 3 * sizeof(unsigned short));

    mesh.normals = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
    memset(mesh.normals, 0, mesh.vertexCount * 3 * sizeof(float));

    mesh.texcoords = (float*)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
    memset(mesh.texcoords, 0, mesh.vertexCount * 2 * sizeof(float));

    float x, y, z, xy;                              // vertex position
    float nx, ny, nz, lengthInv = 1.0f / radius;    // vertex normal
    float s, t;                                     // vertex texCoord

    float longitudeStep = 2 * PI / longitudeCount;
    float latitudeStep = PI / latitudeCount;
    float longitudeAngle, latitudeAngle;

    int vertex = 0;

    for(int i = 0; i <= latitudeCount; ++i)
    {
        latitudeAngle = PI / 2 - i * latitudeStep;        // starting from pi/2 to -pi/2
        xy = radius * cosf(latitudeAngle);             // r * cos(u)
        z = radius * sinf(latitudeAngle);              // r * sin(u)

        for(int j = 0; j <= longitudeCount; ++j)
        {
            longitudeAngle = j * longitudeStep;           // starting from 0 to 2pi
            x = xy * cosf(longitudeAngle);             // r * cos(u) * cos(v)
            y = xy * sinf(longitudeAngle);             // r * cos(u) * sin(v)

            float noise = stb_perlin_fbm_noise3(x / scale, y / scale, z / scale, lacunarity, gain, octaves);

            float offsetX = noise * cosf(latitudeAngle) * cosf(latitudeAngle);
            float offsetY = noise * cosf(latitudeAngle) * sinf(latitudeAngle);
            float offsetZ = noise * sinf(latitudeAngle);

            mesh.vertices[vertex*3 + 0] = x + offsetX;
            mesh.vertices[vertex*3 + 1] = y + offsetY;
            mesh.vertices[vertex*3 + 2] = z + offsetZ;

            Color color = DARKBLUE;
            if (noise > 0.5)
                color = DARKGRAY;
            else if (noise > 0.3)
                color = DARKBROWN;
            else if (noise > 0)
                color = BROWN;
            else if (noise > -0.2)
                color = SKYBLUE;

            mesh.colors[vertex*4 + 0] = color.r;
            mesh.colors[vertex*4 + 1] = color.g;
            mesh.colors[vertex*4 + 2] = color.b;
            mesh.colors[vertex*4 + 3] = color.a;

            vertex++;
        }
    }

    int index = 0;

    int k1, k2;
    for(int i = 0; i < latitudeCount; ++i)
    {
        k1 = i * (longitudeCount + 1);     // beginning of current latitude
        k2 = k1 + longitudeCount + 1;      // beginning of next latitude

        for(int j = 0; j < longitudeCount; ++j, ++k1, ++k2)
        {
            // 2 triangles per longitude excluding first and last latitudes
            // k1 => k2 => k1+1
            if(i != 0)
            {
                mesh.indices[index++] = k1;
                mesh.indices[index++] = k2;
                mesh.indices[index++] = k1 + 1;
            }

            // k1+1 => k2 => k2+1
            if(i != (latitudeCount-1))
            {
                mesh.indices[index++] = k1 + 1;
                mesh.indices[index++] = k2;
                mesh.indices[index++] = k2 + 1;
            }
        }
    }

    UploadMesh(&mesh, false);

    return mesh;
}

int main(int argc, char **argv)
{
    int screenWidth = 1000;
    int screenHeight = 800;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "terragen");

    Camera camera = { 0 };
    camera.position = (Vector3){ 50.0f, 50.0f, 50.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    SetTargetFPS(60);

    float radius = 10;
    int scale = 4;
    float lacunarity = 2;
    float gain = 0.5;
    int octaves = 5;

    Mesh mesh = GenerateMesh(radius, scale, lacunarity, gain, octaves);
    Material material = LoadMaterialDefault();

    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_ORBITAL);

        BeginDrawing();

            ClearBackground(RAYWHITE);

            BeginMode3D(camera);

                Matrix transform = MatrixIdentity();
                DrawMesh(mesh, material, transform);

            EndMode3D();

            DrawFPS(10, 10);

        EndDrawing();
    }

    UnloadMesh(mesh);
    UnloadMaterial(material);

    CloseWindow();

    return 0;
}
