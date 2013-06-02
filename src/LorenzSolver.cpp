/*
 Copyright (C)2013 Stefan Ganev, https://github.com/stefan-g/
 All rights reserved. Licensed under the BSD 2-Clause License; 
 see License.txt and http://opensource.org/licenses/BSD-2-Clause.

 Note:
   This might be not the most computationally efficient implementation performance-wise; 
   the priority is on keeping the code clearly understandable.
*/

#include <vector>

#include "cinder/Cinder.h"
#include "cinder/Vector.h"
#include "cinder/Rand.h"
#include "LorenzSolver.h"

using namespace ci;


// Initialize
//
void LorenzSolver::initOnce()
{
    mUseRK4 = true;
    mInitCondition = mOriginalInitCondition;
    mSolutions = std::vector<Vec3f>();
    mSolutions.reserve( mNumPositions );
    mMaxPos = Vec3f( FLT_MIN, FLT_MIN, FLT_MIN );
    mMinPos = Vec3f( FLT_MAX, FLT_MAX, FLT_MAX );
    mCenterPos = Vec3f::zero();
    mIsCenterCalculated = false;
}


// Set integration step and stride
//
void LorenzSolver::setIntegrationStep( float h, size_t stride )
{
    mH = h;
    mStride = stride;
}


// Set the initial condition 
//
void LorenzSolver::setInitialCondition( float x, float y, float z )
{
    mInitCondition.x = x;
    mInitCondition.y = y;
    mInitCondition.z = z;
}


// Change the initial condition with some small delta,
// to experiment with the sensitive dependence
//
void LorenzSolver::updateInitialCondition( float dx, float dy, float dz )
{
    mInitCondition.x += dx;
    mInitCondition.y += dy;
    mInitCondition.z += dz;
}


// Calculate the solutions
//
void LorenzSolver::solve()
{
    mSolutions.clear();
    mU0 = mInitCondition;
    mSolutions.push_back(mU0);
    for (size_t i = 1; i < mStride*mNumPositions; i++) {
        nextStep( mU0, mU1 );
        mU0 = mU1;
        if( i%mStride==0 ) {
            mSolutions.push_back( Vec3f(mU1) );
            if( ! mIsCenterCalculated) { trackBounds( mU1 ); }
        }
    }
}


// Calculate one step
//
void LorenzSolver::nextStep( Vec3f& u_t0, Vec3f& u_t1 )
{
    mUseRK4 ? nextStepRK4( u_t0, u_t1 ) : nextStepEuler( u_t0, u_t1 );
}


// Euler integration step
//
void LorenzSolver::nextStepEuler( Vec3f& u_t0, Vec3f& u_t1 )
{
    u_t1 = u_t0 + mH * LorenzEquations( u_t0 );
}


// 4th order Runge-Kutta (RK4) integration step
//
void LorenzSolver::nextStepRK4( Vec3f& u_t0, Vec3f& u_t1 )
{
    Vec3f k1, k2, k3, k4;
    k1 = LorenzEquations( u_t0 );
    k2 = LorenzEquations( u_t0 + 0.5f*mH*k1 );
    k3 = LorenzEquations( u_t0 + 0.5f*mH*k2 );
    k4 = LorenzEquations( u_t0 + mH*k3 );
    u_t1 = u_t0 + (mH/6.0f)*( k1 + 2*k2 + 2*k3 + k4 );
}


// Used to find the geometric center of the model,
// used to visualize rotation around the center
//
void LorenzSolver::trackBounds( Vec3f& u_t )
{
    if( u_t.x > mMaxPos.x ) { mMaxPos.x = u_t.x; }
    if( u_t.y > mMaxPos.x ) { mMaxPos.y = u_t.y; }
    if( u_t.z > mMaxPos.z ) { mMaxPos.z = u_t.z; }
    if( u_t.x < mMinPos.x ) { mMinPos.x = u_t.x; }
    if( u_t.y < mMinPos.y ) { mMinPos.y = u_t.y; }
    if( u_t.z < mMinPos.z ) { mMinPos.z = u_t.z; }
}


// Get the geometric center of the model 
// (in phase space, as everything else).
//
Vec3f LorenzSolver::getCenterPos()
{
    if( ! mIsCenterCalculated ) {
        mCenterPos.x = (mMinPos.x + mMaxPos.x)/2.0f;
        mCenterPos.y = (mMinPos.y + mMaxPos.y)/2.0f;
        mCenterPos.z = (mMinPos.z + mMaxPos.z)/2.0f;
        mIsCenterCalculated = true;
    }
    return mCenterPos;
}


/////////////////////////////////////////
//
// Finally, the actual Lorenz equations:
//
Vec3f  LorenzSolver::LorenzEquations( Vec3f u )
{
  Vec3f dUdT;
  dUdT.x = mS * (u.y - u.x);
  dUdT.y = -u.x * u.z + mR * u.x - u.y;
  dUdT.z = u.x * u.y - mB * u.z;
  return dUdT;
}
//
/////////////////////////////////////////
