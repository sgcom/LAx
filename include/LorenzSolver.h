/*
 Copyright (C)2013 Stefan Ganev, https://github.com/stefan-g/
 All rights reserved. Licensed under the BSD 2-Clause License; 
 see License.txt and http://opensource.org/licenses/BSD-2-Clause.
*/

#pragma once

#include <vector>
#include "cinder/Cinder.h"
#include "cinder/Vector.h"

#define DEFAULT_PAR_S   10.0f   // default param sigma
#define DEFAULT_PAR_R   30.0f   // default param r
#define DEFAULT_PAR_B   3.0f    // default param b
#define DEFAULT_H       0.01f   // Integration step
#define DEFAULT_STRIDE  1       // See below

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
// range of stability and unexpected/unbounded results.

class LorenzSolver
{
private:

    size_t      mNumPositions;
    ci::Vec3f   mU0, mU1, mOriginalInitCondition, mInitCondition;
    float       mS, mR, mB, mH;
    size_t      mStride;
    bool        mUseRK4;
    ci::Vec3f   mMinPos, mMaxPos, mCenterPos;
    bool        mIsCenterCalculated;

public:

    std::vector<ci::Vec3f> mSolutions;

    LorenzSolver() {};
    LorenzSolver( size_t numPositions, ci::Vec3f initCondition, float H=DEFAULT_H, float pS=DEFAULT_PAR_S, float pR=DEFAULT_PAR_R, float pB=DEFAULT_PAR_B ) :
                  mNumPositions(numPositions), mS(pS), mR(pR), mB(pB), mH(H), mOriginalInitCondition(initCondition), mStride(DEFAULT_STRIDE)  { initOnce(); }
    void        setParameters( float s, float r, float b ) { mS = s; mR = r; mB = b; }
    void        setIntegrationStep( float h, size_t stride=DEFAULT_STRIDE ) { mH = h; mStride = stride; }
    void        setInitialConditions( ci::Vec3f xyz ) { mInitCondition = xyz; }
    void        useRK4(bool b) { mUseRK4 = b; }
    void        solve();
    ci::Vec3f   getCenterPos();
    std::vector<ci::Vec3f> &   getSolutions() { return mSolutions; }

private:

    void      initOnce() ;
    void      nextStep( ci::Vec3f& u_t0, ci::Vec3f& u_t1 );
    void      nextStepRK4( ci::Vec3f& u_t0, ci::Vec3f& u_t1 );
    void      nextStepEuler( ci::Vec3f& u_t0, ci::Vec3f& u_t1 );
    void      trackBounds( ci::Vec3f& u_t );
    ci::Vec3f LorenzEquations( ci::Vec3f u );
};

