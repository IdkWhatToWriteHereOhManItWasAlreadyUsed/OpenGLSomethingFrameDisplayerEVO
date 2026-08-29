//
// Created by dmitry on 21.08.2026.
//

#ifndef BADPLAYER_VERTEX_H
#define BADPLAYER_VERTEX_H
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace OpenGLSomethingFrameDisplayerEVO
{
    struct Vertex
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoords;
    };

}

#endif //BADPLAYER_VERTEX_H
