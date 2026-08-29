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

uniform mat4 view;
uniform mat4 projection;
uniform vec3 chunkOffset;

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

// Water geometry: 1 грань (top)
const vec3 WATER_POS[4] = vec3[4](
    vec3(-0.5, 0.5, -0.5), vec3(0.5, 0.5, -0.5), vec3(0.5, 0.5, 0.5), vec3(-0.5, 0.5, 0.5)
);

// Grass geometry: 2 диагональных квада (рисуются двусторонне)
const vec3 GRASS_POS[8] = vec3[8](
    // diagonal_1_front
    vec3(-0.5, -0.5,  0.5), vec3(-0.5, 0.5, 0.5), vec3(0.5, 0.5, -0.5), vec3(0.5, -0.5, -0.5),
    // diagonal_2_front
    vec3(-0.5, -0.5, -0.5), vec3(-0.5, 0.5, -0.5), vec3(0.5, 0.5, 0.5), vec3(0.5, -0.5, 0.5)
);

// Индексы вершин grass-квада (двусторонний, как в геометрии из JSON).
const int TRI_INDEX[12] = int[12](0, 3, 1,  1, 3, 2,  0, 1, 3,  1, 2, 3);
// Порядок вершин квада для triangle_strip: треугольники (0,3,1), (1,3,2).
const int QUAD_ORDER[4] = int[4](0, 3, 1, 2);

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

void main()
{
    uint geometryIndex = vGeometryIndex[0];

    // Непрозрачный проход: прозрачные блоки рисуются отдельно (default)
    if ((vFlags[0] & 1u) != 0u)
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

        if (geometryIndex == uint(103)) // 'g'
        {
            // Двусторонний квад: тот же набор треугольников, что и в других проходах.
            for (int t = 0; t < 12; t += 3)
            {
                for (int j = 0; j < 3; j++)
                {
                    int v = TRI_INDEX[t + j];
                    gl_Position = projection * view * vec4(blockPos + faceLocalPos(geometryIndex, face, v), 1.0);
                    EmitVertex();
                }
                EndPrimitive();
            }
        }
        else
        {
            // Обычный квад как triangle_strip: вершины 0,3,1,2 -> треугольники (0,3,1), (1,3,2)
            for (int j = 0; j < 4; j++)
            {
                int v = QUAD_ORDER[j];
                gl_Position = projection * view * vec4(blockPos + faceLocalPos(geometryIndex, face, v), 1.0);
                EmitVertex();
            }
            EndPrimitive();
        }
    }
}