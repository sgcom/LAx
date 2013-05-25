/*
 Copyright (C)2013 Stefan Ganev, https://github.com/stefan-g/
 All rights reserved. Licensed under the BSD 2-Clause License; 
 see License.txt and http://opensource.org/licenses/BSD-2-Clause.

 The purpose of this class is to model a VBO-friendly sphere
 with dynamic positions and color.

 The design consideration is to use minimal number of vertices, 
 for computational efficiency when dynamically updating positions 
 or colors. The number of vertices is much smaller that the number
 of indices.

 */

#pragma once

#include "cinder/Cinder.h"
#include "cinder/gl/Vbo.h"
#include "cinder/Color.h"
#include "cinder/Vector.h"
#include <vector>
#include <stdint.h>


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
