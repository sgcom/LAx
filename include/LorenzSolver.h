/*
 Copyright (C)2013 Stefan Ganev
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are permitted.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <vector>
#include "cinder/Cinder.h"
#include "cinder/Vector.h"

#define DEFAULT_PAR_S   10.0f   // default param sigma
#define DEFAULT_PAR_R   30.0f   // default param r
#define DEFAULT_PAR_B   3.0f    // default param b
#define DEFAULT_H       0.001f   // Integration step
#define DEFAULT_STRIDE  10       // See below

// Tweaking the {STRIDE,H} combination can be used to reduce the integration 
// step and visualize only each N'th solution. This allows for exploring the 
// system at higher integration precision while keepig the GPU load and the 
// viusual density manageable.
//
// Some {STRIDE,H} combinations to explore:
//
// STRIDE       1      10     100
// DEFAULT_H    0.01    0.001   0.0001
//
// *NOTE*: Increasing DEFAULT_H above 0.01 can lead quickly to getting of the 
// range of stability for the integration method and unexpected/unbounded 
// results. It would not mean that something happened to the attractor - instead,
// it would be a problem with how we use our tool to numerically calculate it.


class LorenzSolver
{
private:
    size_t      mNumPositions;
    ci::Vec3f   mU0, mU1, mOriginalInitCondition, mInitCondition;
    float       mS, mR, mB, mH;
    std::vector<ci::Vec3f> 
                mPos;
    size_t      mStride;
    bool        mUseRK4;
    ci::Vec3f   mMinPos, mMaxPos, mCenterPos;
    bool        mIsCenterCalculated;
public:
    LorenzSolver() {};
    LorenzSolver( size_t numPositions, ci::Vec3f initCondition, float H=DEFAULT_H, float pS=DEFAULT_PAR_S, float pR=DEFAULT_PAR_R, float pB=DEFAULT_PAR_B ) :
                  mNumPositions(numPositions), mS(pS), mR(pR), mB(pB), mH(H), mOriginalInitCondition(initCondition), mStride(DEFAULT_STRIDE)  { initOnce(); }
    void        setIntegrationStep( float h, size_t stride=DEFAULT_STRIDE );
    void        setInitialCondition( float x, float y, float z );
    void        updateInitialCondition( float dx, float dy, float dz );
    void        useRK4Toggle() { mUseRK4 = ! mUseRK4; }
    void        useRK4()       { mUseRK4 = true; }
    void        useEuler()     { mUseRK4 = false; }
    void        solve();
    std::vector<ci::Vec3f> 
                getPositions() { return mPos; }
    ci::Vec3f   getCenterPos();
private:
    void      initOnce() ;
    void      setInitCondition();
    void      nextStep( ci::Vec3f& u_t0, ci::Vec3f& u_t1 );
    void      nextStepRK4( ci::Vec3f& u_t0, ci::Vec3f& u_t1 );
    void      nextStepEuler( ci::Vec3f& u_t0, ci::Vec3f& u_t1 );
    void      trackBounds( ci::Vec3f& u_t );
    ci::Vec3f LorenzEquations( ci::Vec3f u );
};

