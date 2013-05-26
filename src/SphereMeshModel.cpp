/*
 Copyright (C)2013 Stefan Ganev, https://github.com/stefan-g/
 All rights reserved. Licensed under the BSD 2-Clause License; 
 see License.txt and http://opensource.org/licenses/BSD-2-Clause.

 The purpose of this class is to model a VBO-friendly sphere mesh
 with dynamic positions and color.

 The design consideration is to use minimal number of vertices, 
 for computational efficiency when dynamically updating positions 
 or colors. The number of vertices is much smaller that the number
 of indices.

 NOTE: Building the mesh may take some significant time.
 ----  It is supposed to be executed only once for the app life cycle.

 */

#include "cinder/Cinder.h"
#include "cinder/gl/Vbo.h"
#include "cinder/Color.h"
#include "cinder/Vector.h"
#include "cinder/Rand.h"
#include <cinder/app/App.h>
#include <vector>

#include "../include/SphereMeshModel.h"

using namespace ci;
using namespace std;

#define PI  3.141592653589f


SphereMeshModel::SphereMeshModel( const int n_slices, const int n_stacks, const float radius, const ci::Colorf color ) 
{
    mRadius      = radius;
    mColor       = color;
    nSlices      = n_slices;
    nStacks      = n_stacks;
    nVertices    = n_slices * (n_stacks-1) + 2;
    nIndices     = 6 * n_slices * (n_stacks-1);
    pNormals     = NULL;
    pPositions   = NULL;
    mEggFactor1  = 1.0;    // these two "egg-factors" are not being used now
    mEggFactor2  = 1.0;    // but could be used to make the sphere look like egg

    initUnitSphere();
}


SphereMeshModel::SphereMeshModel( const SphereMeshModel& o )
{
    deepCopy( o );
}


SphereMeshModel& SphereMeshModel::operator=(const SphereMeshModel &o)
{
    if( &o != this ) {
        deepCopy( o );
    }
    return *this;
}


void SphereMeshModel::deepCopy( const SphereMeshModel& o )
{
    mRadius    = o.mRadius;
    mColor     = o.mColor;
    nSlices    = o.nSlices;
    nStacks    = o.nStacks;
    nVertices  = o.nVertices;
    nIndices   = o.nIndices;
    mEggFactor1= o.mEggFactor1;
    mEggFactor2= o.mEggFactor2;
    if( pNormals != NULL )   delete [] pNormals;
    if( pPositions != NULL ) delete [] pPositions;
    pNormals   = new Vec3f[ nVertices ];
    pPositions = new Vec3f[ nVertices ];
    memcpy ( pNormals, o.pNormals, nVertices*sizeof(Vec3f) );
    memcpy ( pPositions, o.pPositions, nVertices*sizeof(Vec3f) );
}


SphereMeshModel::~SphereMeshModel() 
{
    if( pNormals != NULL )   delete [] pNormals;
    if( pPositions != NULL ) delete [] pPositions;
}


void SphereMeshModel::initUnitSphere() 
{
    pNormals = new Vec3f[ nVertices ];
    pPositions = new Vec3f[ nVertices ];

    Vec3f *pn = pNormals;
    Vec3f *pp = pPositions;

    float dRho = PI / (float) nStacks;
    float dTheta = 2.0f * PI / (float) nSlices;
    float dSlice = 1.0f / (float) nSlices;
    float dStack = 1.0f / (float) nStacks;
    float texT = 1.0f;	
    float texS = 0.0f;
    uint32_t i, j;
    //--------------------------------
    uint32_t  index = 0;
    bool      v0done = false;
    bool      vNdone = false;
    //--------------------------------
    for ( i = 0; i <= nStacks; i++ ) {
        float rho = (float)i * dRho;
        float sinRho = (float)(sin(rho));
        float cosRho = (float)(cos(rho));
        texS = 0.0f;
        //-------------------------------
        for ( j = 0; j < nSlices; j++ ) {
            if( i == 0 ) {
                if( v0done ) continue;
                v0done = true;
            } else if( i == nStacks ) {
                if( vNdone ) continue;
                vNdone = true;
            }
            float theta = (j == nSlices) ? 0.0f : j * dTheta;
            float minusSinTheta = (float)(-sin(theta)) * (theta < PI ? mEggFactor1 : mEggFactor2);
            float cosTheta = (float)(cos(theta));

            float x = minusSinTheta * sinRho;
            float y = cosTheta * sinRho;
            float z = cosRho;

            Vec2f t0 = Vec2f(texS, texT);
            Vec3f n0 = Vec3f(x, y, z);

            *pp = n0 * mRadius;
            n0.normalize();
            *pn = n0;
            //texture?? *pt++ = t0;
            ++pn;
            ++pp;

            texS += dSlice;
        }
        texT -= dStack;
    }
    assert( pn - pNormals   == nVertices );
    assert( pp - pPositions == nVertices );
}


void SphereMeshModel::getStaticIndices( uint32_t startIndex, vector<uint32_t> &indices ) 
{
    int a, b, c, d, e, f;
    bool _00done = false;

    for ( uint32_t i = 0; i < nStacks; i++ ) {
        for ( uint32_t j = 0; j < nSlices; j++ ) {

            if( i == 0 ) {
                a = 0;
                b = j + 1;
                c = (j < nSlices-1) ? j + 2 : 1;
                indices.push_back( startIndex + a );
                indices.push_back( startIndex + b );
                indices.push_back( startIndex + c );
            } else if( i+1 == nStacks ) {
                a = i*nSlices-(nSlices-1) + j;
                b = i*nSlices-(nSlices-1) + nSlices;
                c = (j < nSlices-1) ? i*nSlices-(nSlices-1) + j + 1 : i*nSlices-(nSlices-1);
                indices.push_back( startIndex + a );
                indices.push_back( startIndex + b );
                indices.push_back( startIndex + c );
            } else {
                if( j < nSlices-1 ) {
                    a =     i*nSlices-(nSlices-1) + j;
                    b = (i+1)*nSlices-(nSlices-1) + j;
                    c = (i+1)*nSlices-(nSlices-1) + j + 1;
                    d = a;
                    e = c;
                    f = i*nSlices-(nSlices-1) + j + 1;
                } else { 
                    a =     i*nSlices-(nSlices-1) + j;
                    b = (i+1)*nSlices-(nSlices-1) + j;
                    c = (i+1)*nSlices-(nSlices-1) + 0 + 0;
                    d = a;
                    e = c;
                    f = i*nSlices-(nSlices-1) + 0 + 0;
                }
                indices.push_back( startIndex + a );
                indices.push_back( startIndex + b );
                indices.push_back( startIndex + c );
                indices.push_back( startIndex + d );
                indices.push_back( startIndex + e );
                indices.push_back( startIndex + f );
            }
        }
    }
}


void SphereMeshModel::getStaticNormals( vector<Vec3f> &normals ) 
{
    float dRho = PI / (float) nStacks;
    float dTheta = 2.0f * PI / (float) nSlices;
    float dSlice = 1.0f / (float) nSlices;
    float dStack = 1.0f / (float) nStacks;
    float texT = 1.0f;	
    float texS = 0.0f;
    uint32_t i, j;
    //--------------------------------
    bool v0done = false;
    bool vNdone = false;
    //--------------------------------
    for ( i = 0; i <= nStacks; i++ ) {
        float rho = (float)i * dRho;
        float sinRho = (float)(sin(rho));
        float cosRho = (float)(cos(rho));
        //-------------------------------
        for ( j = 0; j < nSlices; j++ ) {
            if( i == 0 ) {
                if( v0done ) continue;
                v0done = true;
            } else if( i == nStacks ) {
                if( vNdone ) continue;
                vNdone = true;
            }
            float theta = (j == nSlices) ? 0.0f : j * dTheta;
            float minusSinTheta = (float)(-sin(theta));
            float cosTheta = (float)(cos(theta));

            float x = minusSinTheta * sinRho;
            float y = cosTheta * sinRho;
            float z = cosRho;

            Vec3f n0 = Vec3f(x, y, z).normalized();

            normals.push_back( n0 );
        }
    }
}

/*
** Update a VBO from the sphere model.
** Dynamic position and color; everything else - static.
*/
void SphereMeshModel::updateVBO( ci::gl::VboMesh::VertexIter &vertexIter, const Vec3f sphereCenterLocation, const Colorf color) 
{
    Color effectiveColor = mColor;
    if( color != Color::black() ) {
        effectiveColor = color;
    }
    Vec3f *pp = pPositions; // unit sphere positions
    for( uint32_t i=0; i<nVertices; i++ ) {
        Vec3f loc = *pp * mRadius;
        vertexIter.setPosition( sphereCenterLocation + loc );
        vertexIter.setColorRGB( effectiveColor );
        // consider setting texture dynamically, too
        ++pp;
        ++vertexIter;
    }
}

