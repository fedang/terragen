#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>
#include "rlgl.h"

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

int main(int argc, char **argv)
{
    const int screenWidth = 1000;
    const int screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "terragen");

    Camera camera = { 0 };
    camera.position = (Vector3){ 50.0f, 50.0f, 50.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        UpdateCamera(&camera, CAMERA_ORBITAL);

        BeginDrawing();

            ClearBackground(RAYWHITE);

            BeginMode3D(camera);

                Vector3 centerPos = { 0, 0, 0 };
                float radius = 10;
                int rings = 100;
                int slices = 100;

                rlPushMatrix();
                    rlTranslatef(centerPos.x, centerPos.y, centerPos.z);
                    rlScalef(radius, radius, radius);

                    rlBegin(RL_TRIANGLES);

                        float ringangle = DEG2RAD*(180.0f/(rings + 1)); // Angle between latitudinal parallels
                        float sliceangle = DEG2RAD*(360.0f/slices); // Angle between longitudinal meridians

                        float cosring = cosf(ringangle);
                        float sinring = sinf(ringangle);
                        float cosslice = cosf(sliceangle);
                        float sinslice = sinf(sliceangle);

                        Vector3 vertices[4] = { 0 }; // Required to store face vertices
                        vertices[2] = (Vector3){ 0, 1, 0 };
                        vertices[3] = (Vector3){ sinring, cosring, 0 };

                        for (int i = 0; i < rings + 1; i++) {
                            for (int j = 0; j < slices; j++) {
                                vertices[0] = vertices[2]; // Rotate around y axis to set up vertices for next face
                                vertices[1] = vertices[3];
                                vertices[2] = (Vector3){ cosslice*vertices[2].x - sinslice*vertices[2].z, vertices[2].y, sinslice*vertices[2].x + cosslice*vertices[2].z }; // Rotation matrix around y axis
                                vertices[3] = (Vector3){ cosslice*vertices[3].x - sinslice*vertices[3].z, vertices[3].y, sinslice*vertices[3].x + cosslice*vertices[3].z };

                                float offset[4][3];
                                Color color[4];

                                for (int i = 0; i < 4; i++) {
                                    float noise = stb_perlin_fbm_noise3(vertices[i].x, vertices[i].y, vertices[i].z, 2, 0.5, 4);
                                    noise = Remap(noise, -1, 1, 0, 0.5);

                                    offset[i][0] = noise * sinring * cosslice;
                                    offset[i][1] = noise * sinring * sinslice;
                                    offset[i][2] = noise * cosring;

                                    if (noise > 0.4)
                                        color[i] = DARKGRAY;
                                    else if (noise > 0.3)
                                        color[i] = DARKBROWN;
                                    else if (noise > 0.2)
                                        color[i] = BROWN;
                                    else if (noise > 0.1)
                                        color[i] = SKYBLUE;
                                    else
                                        color[i] = DARKBLUE;
                                }

                                // Triangle 1

                                rlColor4ub(color[0].r, color[0].g, color[0].b, color[0].a);
                                rlVertex3f(vertices[0].x + offset[0][0], vertices[0].y + offset[0][1], vertices[0].z + offset[0][2]);

                                rlColor4ub(color[3].r, color[3].g, color[3].b, color[3].a);
                                rlVertex3f(vertices[3].x + offset[3][0], vertices[3].y + offset[3][1], vertices[3].z + offset[3][2]);

                                rlColor4ub(color[1].r, color[1].g, color[1].b, color[1].a);
                                rlVertex3f(vertices[1].x + offset[1][0], vertices[1].y + offset[1][1], vertices[1].z + offset[1][2]);

                                // Triangle 2

                                rlColor4ub(color[0].r, color[0].g, color[0].b, color[0].a);
                                rlVertex3f(vertices[0].x + offset[0][0], vertices[0].y + offset[0][1], vertices[0].z + offset[0][2]);

                                rlColor4ub(color[2].r, color[2].g, color[2].b, color[2].a);
                                rlVertex3f(vertices[2].x + offset[2][0], vertices[2].y + offset[2][1], vertices[2].z + offset[2][2]);

                                rlColor4ub(color[3].r, color[3].g, color[3].b, color[3].a);
                                rlVertex3f(vertices[3].x + offset[3][0], vertices[3].y + offset[3][1], vertices[3].z + offset[3][2]);
                            }

                            vertices[2] = vertices[3]; // Rotate around z axis to set up  starting vertices for next ring
                            vertices[3] = (Vector3){ cosring*vertices[3].x + sinring*vertices[3].y, -sinring*vertices[3].x + cosring*vertices[3].y, vertices[3].z }; // Rotation matrix around z axis
                        }
                    rlEnd();
                rlPopMatrix();

            EndMode3D();

            DrawFPS(10, 10);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
