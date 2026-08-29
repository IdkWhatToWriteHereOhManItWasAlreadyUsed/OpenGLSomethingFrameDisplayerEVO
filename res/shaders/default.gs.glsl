#version 420 core

layout (points) in;
layout (triangle_strip, max_vertices = 24) out;

in ivec3 vBlockPos[];
in uvec4 vTexCoords0[];
in uvec4 vTexCoords1[];
in uvec4 vTexCoords2[];
in uint vGeometryIndex[];
in uint vFlags[];
in uint vVisibleFaces[];

out float FogFactor;
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec3 ViewPos;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 chunkOffset;

uniform vec3 viewPos;
uniform float fogDensity;
uniform float fogStart;
uniform float fogEnd;

const float TILE = 0.0625; // 1/16 - размер одной текстуры в атласе

// Cube geometry: 6 граней (back, front, top, bottom, left, right), по 4 вершины каждая.
// Позиции заданы в локальном пространстве блока (центр блока = 0).
const vec3 CUBE_POS[24] = vec3[24](
    // back (z = -0.5)
    vec3( 0.5,  0.5, -0.5), vec3(-0.5,  0.5, -0.5), vec3(-0.5, -0.5, -0.5), vec3( 0.5, -0.5, -0.5),
    // front (z = +0.5)
    vec3(-0.5,  0.5,  0.5), vec3( 0.5,  0.5,  0.5), vec3( 0.5, -0.5,  0.5), vec3(-0.5, -0.5,  0.5),
    // top (y = +0.5)
    vec3(-0.5,  0.5, -0.5), vec3( 0.5,  0.5, -0.5), vec3( 0.5,  0.5,  0.5), vec3(-0.5,  0.5,  0.5),
    // bottom (y = -0.5)
    vec3(-0.5, -0.5,  0.5), vec3( 0.5, -0.5,  0.5), vec3( 0.5, -0.5, -0.5), vec3(-0.5, -0.5, -0.5),
    // left (x = -0.5)
    vec3(-0.5,  0.5, -0.5), vec3(-0.5,  0.5,  0.5), vec3(-0.5, -0.5,  0.5), vec3(-0.5, -0.5, -0.5),
    // right (x = +0.5)
    vec3( 0.5,  0.5,  0.5), vec3( 0.5,  0.5, -0.5), vec3( 0.5, -0.5, -0.5), vec3( 0.5, -0.5,  0.5)
);

const vec3 CUBE_NORMAL[6] = vec3[6](
    vec3( 0.0,  0.0, -1.0), // back
    vec3( 0.0,  0.0,  1.0), // front
    vec3( 0.0,  1.0,  0.0), // top
    vec3( 0.0, -1.0,  0.0), // bottom
    vec3(-1.0,  0.0,  0.0), // left
    vec3( 1.0,  0.0,  0.0)  // right
);

// Water geometry: 1 грань (top)
const vec3 WATER_POS[4] = vec3[4](
    vec3(-0.5, 0.5, -0.5), vec3(0.5, 0.5, -0.5), vec3(0.5, 0.5, 0.5), vec3(-0.5, 0.5, 0.5)
);
const vec3 WATER_NORMAL[1] = vec3[1](
    vec3(0.0, 1.0, 0.0)
);

// Grass geometry: 2 диагональных квада (рисуются двусторонне)
const vec3 GRASS_POS[8] = vec3[8](
    // diagonal_1_front
    vec3(-0.5, -0.5,  0.5), vec3(-0.5, 0.5, 0.5), vec3(0.5, 0.5, -0.5), vec3(0.5, -0.5, -0.5),
    // diagonal_2_front
    vec3(-0.5, -0.5, -0.5), vec3(-0.5, 0.5, -0.5), vec3(0.5, 0.5, 0.5), vec3(0.5, -0.5, 0.5)
);
const vec3 GRASS_NORMAL[2] = vec3[2](
    vec3( 0.707, 0.0, -0.707),
    vec3(-0.707, 0.0, -0.707)
);

// Индексы вершин grass-квада (двусторонний, как в геометрии из JSON).
const int TRI_INDEX[12] = int[12](0, 3, 1,  1, 3, 2,  0, 1, 3,  1, 2, 3);
// Порядок вершин квада для triangle_strip: треугольники (0,3,1), (1,3,2).
const int QUAD_ORDER[4] = int[4](0, 3, 1, 2);

// Порядок байтов texCoords: 6 пар (texture_x, texture_z) по лицу, всего 12 байт.
uint tileByte(int index, uvec4 t0, uvec4 t1, uvec4 t2)
{
    if (index < 4)
        return t0[index];
    if (index < 8)
        return t1[index - 4];
    return t2[index - 8];
}

// Условный UV внутри тайла для гран-квада cube/water: 0..TILE на сторону.
vec2 quadUV(int vertex)
{
    return vec2((vertex == 1 || vertex == 2) ? TILE : 0.0,
                (vertex >= 2) ? TILE : 0.0);
}

// Условный UV внутри тайла для grass (текстуры перевёрнуты вертикально).
vec2 grassUV(int vertex)
{
    return vec2((vertex == 2 || vertex == 3) ? TILE : 0.0,
                (vertex == 0 || vertex == 3) ? TILE : 0.0);
}

int geometryFaceCount(uint geometryIndex)
{
    if (geometryIndex == uint(99)) return 6; // 'c'
    if (geometryIndex == uint(103)) return 2; // 'g'
    return 1; // water
}

// Отсечение по visibleFaces имеет смысл только для cube (есть can_cull_faces).
bool geometryCullsFaces(uint geometryIndex)
{
    return geometryIndex == uint(99); // 'c'
}

vec3 faceLocalPos(uint geometryIndex, int face, int vertex)
{
    if (geometryIndex == uint(99)) return CUBE_POS[face * 4 + vertex];
    if (geometryIndex == uint(103)) return GRASS_POS[face * 4 + vertex];
    return WATER_POS[vertex];
}

vec3 faceNormalLocal(uint geometryIndex, int face)
{
    if (geometryIndex == uint(99)) return CUBE_NORMAL[face];
    if (geometryIndex == uint(103)) return GRASS_NORMAL[face];
    return WATER_NORMAL[0];
}

vec2 faceBaseUV(uint geometryIndex, int vertex)
{
    if (geometryIndex == uint(103)) return grassUV(vertex);
    return quadUV(vertex);
}

void emitQuadVec(vec3 worldPos, vec3 normal, vec2 uv)
{
    FragPos = worldPos;
    Normal = normal;
    TexCoords = uv;
    ViewPos = viewPos;

    float fogFactor = (length(worldPos - viewPos) - fogStart) / (fogEnd - fogStart);
    FogFactor = clamp(fogFactor, 0.0, 1.0);

    gl_Position = projection * view * vec4(worldPos, 1.0);
    EmitVertex();
}

void main()
{
    uint geometryIndex = vGeometryIndex[0];

    // Прозрачный проход: непрозрачные блоки уже в gBuffer (lightPass)
    if ((vFlags[0] & 1u) == 0u)
        return;

    if (geometryIndex != uint(99) && geometryIndex != uint(103) && geometryIndex != uint(119))
        return;

    int faceCount = geometryFaceCount(geometryIndex);
    bool cull = geometryCullsFaces(geometryIndex);

    vec3 blockPos = chunkOffset + vec3(vBlockPos[0]);

    for (int face = 0; face < faceCount; face++)
    {
        if (cull && (vVisibleFaces[0] & (1u << uint(face))) == 0u)
            continue;

        vec2 tileOffset = vec2(
            float(tileByte(face * 2, vTexCoords0[0], vTexCoords1[0], vTexCoords2[0])),
            float(tileByte(face * 2 + 1, vTexCoords0[0], vTexCoords1[0], vTexCoords2[0]))
        ) * TILE;

        if (geometryIndex == uint(103)) // 'g'
        {
            // Двусторонний квад: ровно те же треугольники, что и в геометрии из JSON:
            // (0,3,1), (1,3,2), (0,1,3), (1,2,3)
            vec3 normal = faceNormalLocal(geometryIndex, face);
            for (int t = 0; t < 12; t += 3)
            {
                for (int j = 0; j < 3; j++)
                {
                    int v = TRI_INDEX[t + j];
                    emitQuadVec(blockPos + faceLocalPos(geometryIndex, face, v),
                                normal,
                                tileOffset + faceBaseUV(geometryIndex, v));
                }
                EndPrimitive();
            }
        }
        else
        {
            // Обычный квад как triangle_strip: вершины 0,3,1,2 -> треугольники (0,3,1), (1,3,2)
            vec3 normal = faceNormalLocal(geometryIndex, face);
            for (int j = 0; j < 4; j++)
            {
                int v = QUAD_ORDER[j];
                emitQuadVec(blockPos + faceLocalPos(geometryIndex, face, v),
                            normal,
                            tileOffset + faceBaseUV(geometryIndex, v));
            }
            EndPrimitive();
        }
    }
}