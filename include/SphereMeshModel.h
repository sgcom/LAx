<<<<<<< HEAD
/*
 Copyright (C)2013 Stefan Ganev 
 All rights reserved.
 https://github.com/stefan-g/

 Redistribution and use in source and binary forms, with or without modification, are permitted.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.

 The purpose of this class is to model a VBO-friendly sphere
 with dynamic positions and color.

 The design consideration is to use minimal number of vertices, 
 for computational efficiency when dynamically updating positions 
 or colors. The number of vertices is much smaller that the number
 of indices.

 */

=======
>>>>>>> e2d5a2a7bfa20c82541a72fcff8a1b39c94fed12
#pragma once

#include "cinder/Cinder.h"
#include "cinder/gl/Vbo.h"
#include "cinder/Color.h"
#include "cinder/Vector.h"
#include <vector>
#include <stdint.h>

/*
** The purpose of this class is to model a VBO-friendly sphere
** with dynamic positions and color.
**
** The design consideration is to use minimal number of vertices, 
** for computational efficiency when dynamically updating positions 
** or colors. The number of vertices is much smaller that the number
** of indices.
**
** Otherwise, the parametric calculations are based on multiple other 
** implementations.
**
** Author: Stefan Ganev, https://github.com/stefan-g/
** The code is free to use with no warranty and no expectations for support.
*/

class SphereMeshModel 
{
    uint32_t    nSlices;        // number of slices
    uint32_t    nStacks;        // number of stacks
    uint32_t    nVertices;      // number of vertices
    uint32_t    nIndices;       // number of indices
    float       mRadius;        // sphere radius
    ci::Colorf  mColor;         // sphere default color; can be modified per vertex later
    float       mEggFactor1;    // these so called egg-factors can make the sphere look
    float       mEggFactor2;    // like an egg, if needed.

    ci::Vec3f  *pNormals;       // normals
    ci::Vec3f  *pPositions;     // positions

public:

    SphereMeshModel( const int n_slices=20, const int n_stacks=20, const float radius=1.0f, const ci::Colorf defaultColor = ci::Colorf::white() );
    ~SphereMeshModel();
    SphereMeshModel( const SphereMeshModel& o );
    SphereMeshModel& operator=(const SphereMeshModel &o);

    void getStaticIndices( uint32_t startIndex, std::vector<uint32_t> &indices );
    void getStaticNormals( std::vector<ci::Vec3f> &normals );
    void updateVBO( ci::gl::VboMesh::VertexIter &vertexIter, const ci::Vec3f sphereCenterLocation, const ci::Colorf color=ci::Colorf::black());

private:

    void initUnitSphere();
    void deepCopy( const SphereMeshModel& o );

};
