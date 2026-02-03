#pragma once

#include "VectorPool/VectorPool.h"
#include "renderer/Mesh.h"

namespace OpenGLSomethingFrameDisplayerEVO {

static VectorPool<Vertex> vertexVectorPool(4444);
static VectorPool<unsigned int> indexVectorPool(6666);

}