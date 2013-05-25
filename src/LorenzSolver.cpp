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

 Note:
   This might be not the most computationally efficient implementation performance-wise; 
   the priority is on keeping the code clearly understandable.
*/
=======
>>>>>>> e2d5a2a7bfa20c82541a72fcff8a1b39c94fed12

#include <vector>

#include "cinder/Cinder.h"
#include "cinder/Vector.h"
#include "cinder/Rand.h"
#include "LorenzSolver.h"

using namespace ci;


<<<<<<< HEAD
=======
LorenzSolver::LorenzSolver() 
{
}


LorenzSolver::LorenzSolver( size_t numPositions, Vec3f initCondition, float H, float pS, float pR, float pB )
{
    mNumPositions = numPositions;
    mS = pS; 
    mR = pR; 
    mB = pB;
    mH = H;
    mOriginalInitCondition = initCondition;
    mStride = DEFAULT_STRIDE;
    initOnce();
}


>>>>>>> e2d5a2a7bfa20c82541a72fcff8a1b39c94fed12
void LorenzSolver::initOnce()
{
    mUseRK4 = true;
    mInitCondition = mOriginalInitCondition;
    mPos = std::vector<Vec3f>( mNumPositions );
    mMaxPos = Vec3f( FLT_MIN, FLT_MIN, FLT_MIN );
    mMinPos = Vec3f( FLT_MAX, FLT_MAX, FLT_MAX );
    mCenterPos = Vec3f::zero();
    mIsCenterCalculated = false;
}


void LorenzSolver::setInitCondition()
{
    mPos.clear();
    mU0 = mInitCondition;
    mPos.push_back(mU0);
}


void LorenzSolver::setIntegrationStep( float h, size_t stride )
{
    mH = h;
    mStride = stride;
}


void LorenzSolver::setInitialCondition( float x, float y, float z )
{
    mInitCondition.x = x;
    mInitCondition.y = y;
    mInitCondition.z = z;
}


void LorenzSolver::updateInitialCondition( float dx, float dy, float dz )
{
    mInitCondition.x += dx;
    mInitCondition.y += dy;
    mInitCondition.z += dz;
}


void LorenzSolver::solve()
{
    setInitCondition();
    for (size_t i = 1; i < mStride*mNumPositions; i++) {
        nextStep( mU0, mU1 );
        mU0 = mU1;
        if( i%mStride==0 ) {
            mPos.push_back( Vec3f(mU1) );
            if( ! mIsCenterCalculated) { trackBounds( mU1 ); }
        }
    }
}


void LorenzSolver::nextStep( Vec3f& u_t0, Vec3f& u_t1 )
{
    mUseRK4 ? nextStepRK4( u_t0, u_t1 ) : nextStepEuler( u_t0, u_t1 );
}


void LorenzSolver::nextStepEuler( Vec3f& u_t0, Vec3f& u_t1 )
{
    // Euler integration step
    //
    u_t1 = u_t0 + mH * LorenzEquations( u_t0 );
}

<<<<<<< HEAD
=======
/////////////////////////////////////////////////////////////////////////////////
// Note:
//   This might be not the most computationally efficient implementation 
//   performance-wise, but I'd like  to keeep it this way for its clarity 
//   about what is going on.
/////////////////////////////////////////////////////////////////////////////////
>>>>>>> e2d5a2a7bfa20c82541a72fcff8a1b39c94fed12

void LorenzSolver::nextStepRK4( Vec3f& u_t0, Vec3f& u_t1 )
{
    // 4th order Runge-Kutta (RK4) integration step
    //
    Vec3f k1, k2, k3, k4;
    k1 = LorenzEquations( u_t0 );
    k2 = LorenzEquations( u_t0 + 0.5f*mH*k1 );
    k3 = LorenzEquations( u_t0 + 0.5f*mH*k2 );
    k4 = LorenzEquations( u_t0 + mH*k3 );
    u_t1 = u_t0 + (mH/6.0f)*( k1 + 2*k2 + 2*k3 + k4 );
}


void LorenzSolver::trackBounds( Vec3f& u_t )
{
    if( u_t.x > mMaxPos.x ) { mMaxPos.x = u_t.x; }
    if( u_t.y > mMaxPos.x ) { mMaxPos.y = u_t.y; }
    if( u_t.z > mMaxPos.z ) { mMaxPos.z = u_t.z; }
    if( u_t.x < mMinPos.x ) { mMinPos.x = u_t.x; }
    if( u_t.y < mMinPos.y ) { mMinPos.y = u_t.y; }
    if( u_t.z < mMinPos.z ) { mMinPos.z = u_t.z; }
}


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
// The actual Lorenz equations:
//
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
