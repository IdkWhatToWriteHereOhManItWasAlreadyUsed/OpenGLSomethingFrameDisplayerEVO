#version 330 core

layout (location = 0) in ivec3 position;
layout (location = 1) in uvec4 texCoords0;
layout (location = 2) in uvec4 texCoords1;
layout (location = 3) in uvec4 texCoords2;
layout (location = 4) in uint geometryIndex;
layout (location = 5) in uint flags;
layout (location = 6) in uint visibleFaces;

out ivec3 vBlockPos;
out uvec4 vTexCoords0;
out uvec4 vTexCoords1;
out uvec4 vTexCoords2;
out uint vGeometryIndex;
out uint vFlags;
out uint vVisibleFaces;

void main()
{
    vBlockPos = position;
    vTexCoords0 = texCoords0;
    vTexCoords1 = texCoords1;
    vTexCoords2 = texCoords2;
    vGeometryIndex = geometryIndex;
    vFlags = flags;
    vVisibleFaces = visibleFaces;

    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
}