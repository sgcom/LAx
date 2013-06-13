/*
 Copyright (C)2013 Stefan Ganev, https://github.com/stefan-g/
 All rights reserved. Licensed under the BSD 2-Clause License; 
 see License.txt and http://opensource.org/licenses/BSD-2-Clause.
*/

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/Camera.h"
#include "cinder/Rand.h"
#include "cinder/gl/Texture.h"
#include "cinder/Text.h"
#include "cinder/Font.h"
#include "cinder/params/Params.h"

#include "Resources.h"
#include "SphereMeshModel.h"
#include "LorenzSolver.h"


using namespace ci;
using namespace ci::app;
using namespace std;

#define MODEL_SPHERE_STACKS 10
#define MODEL_SPHERE_SLICES 20

#define MAX_STEPS   3000    // Max number of steps (solutions)


#define LORENZ_DEFAULT_INITIAL_CONDITION    Vec3f(0.1f, 0.1f, 0.1f)
#define LORENZ_DEFAULT_PARAM_S              10.0f
#define LORENZ_DEFAULT_PARAM_R              30.0f
#define LORENZ_DEFAULT_PARAM_B               3.0f

struct LorenzParams {
    int32_t mNumSteps;
    bool    mUseRK4;
    Vec3f   mInitialCondition;
    float   mParam_S, mParam_R, mParam_B;
    bool    mAutoIncementX;
};


class LAxApp : public AppNative 
{
private:

    CameraPersp        mCam;
    Vec3f              mCamEyePoint, mCamTarget, mCamUp;
    float              mCamFovAngle;
    Vec4f              mLightPosition;
    float              mRotationStep;
    LorenzSolver       mSolver;
    SphereMeshModel    mSphereModel;
    gl::VboMesh        mModelMesh;
    int32_t            mIndicesPerSphere;
    int32_t            mModelNumElements;
    Vec3f              mCenterPos;
    int32_t            mIterationCnt;
    bool               mIterativeDraw;
    Vec2i              mCurrentMouseDown;
    Vec2i              mInitialMouseDown;
    Rand               mRand;
    bool               mViewModelEnabled;
    bool               mAutoRotate;
   
    params::InterfaceGlRef	mParams;
    LorenzParams       mLorenzParams, mOrigParams;
    float              mAverageFps;

public:

    void  prepareSettings( Settings *settings );
    void  setup();
    void  update();
    void  draw();
    void  resize();
    void  mouseDown( MouseEvent event );
    void  mouseDrag( MouseEvent event );
    void  mouseWheel( MouseEvent event );

private:

    void  ppl_initModel();
    void  initModel();
    void  updateCameraPerspective();
    void  rotateModel( float leftRight, float upDown );
    void  zoom( float w );

};


/*
** Called once before setup(). Used for windows settings mostly.
*/

void LAxApp::prepareSettings( Settings *settings ) 
{
    // Window size and frame rate
    settings->setWindowSize( 1280, 720 );
    settings->setFrameRate( 30.0f );
    settings->setTitle( "LAx" );
    //settings->enableConsoleWindow(true);
}


/*
** Application set-up. Executed once after application starts, 
** after prepareSettings() and before everything else.
*/
void LAxApp::setup()
{
    // Random numbers generator
    mRand = Rand();

    //Initial model params
    mLorenzParams.mNumSteps = MAX_STEPS;
    mLorenzParams.mUseRK4 = true;
    mLorenzParams.mInitialCondition = LORENZ_DEFAULT_INITIAL_CONDITION;
    mLorenzParams.mParam_S = LORENZ_DEFAULT_PARAM_S;
    mLorenzParams.mParam_R = LORENZ_DEFAULT_PARAM_R;
    mLorenzParams.mParam_B = LORENZ_DEFAULT_PARAM_B;
    mLorenzParams.mAutoIncementX = false;
    mOrigParams = mLorenzParams;

    mViewModelEnabled = true; // currently not used
    mAutoRotate = false;

    // CAMERA: ...
    mCamEyePoint = Vec3f( 30.6671f, -40.4094f, -33.9354f ); // initial eye point
    mCamUp = Vec3f( -0.401262f, -0.801144f, 0.444025f );    // initial camera up vector
    mCamTarget = Vec3f::zero();                             // we keep looking at point zero.
    mCamFovAngle = 60.0f;                                   // field of view angle
    mRotationStep = 0.1f;

    // LIGHT Position:
    mLightPosition = Vec4f(50.0f, -270.0f, 230.0f, 1.0f);

    // MODEL: Init the model, see initModel()
    initModel();
    mIterationCnt = 0;
    mIterativeDraw = false;
    mCenterPos = Vec3f::zero(); // model center - will be updated later

    // Standard legacy OpenGL stuff below; mostly light and material properties.
    // TODO better light with custom shader
    //
    gl::enableDepthWrite();
    gl::enableDepthRead();
    gl::enableAlphaBlending();

    glEnable( GL_LIGHTING );
    glEnable( GL_COLOR_MATERIAL );
    glEnable( GL_LIGHT0 );
    
    GLfloat global_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);

    GLfloat lightColorDiffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat lightColorSpecular[] = { 0.4f, 0.4f, 0.4f, 1.0f };
    GLfloat lightColorAmbient[] = { 0.1f, 0.2f, 0.3f, 1.0f };
    glLightfv( GL_LIGHT0, GL_AMBIENT, lightColorAmbient );
    glLightfv( GL_LIGHT0, GL_DIFFUSE, lightColorDiffuse );
    glLightfv( GL_LIGHT0, GL_SPECULAR, lightColorSpecular );

    float materialSpecularRefl[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecularRefl);
    glMateriali(GL_FRONT, GL_SHININESS, 88);

    // Params (AntTweakBar)
    //
    mParams = params::InterfaceGl::create( "Lorenz Attractor Explorer", Vec2i( 320, 300 ) );
    //int32_t iii = 0;
    // addParam( const std::string &name, int32_t *intParam, const std::string &optionsStr = "", bool readOnly = false );
    stringstream ss;
    ss << "min=50 max=" << MAX_STEPS << " step=10 keyIncr=> keyDecr=<";
    mParams->addParam( "Steps to render", &mLorenzParams.mNumSteps, ss.str() );
    mParams->addParam( "Lorenz system param S", &mLorenzParams.mParam_S, "min=1 max=50 step=0.1 keyIncr=S keyDecr=s" );
    mParams->addParam( "Lorenz system param R", &mLorenzParams.mParam_R, "min=1 max=50 step=0.1 keyIncr=S keyDecr=s" );
    mParams->addParam( "Lorenz system param B", &mLorenzParams.mParam_B, "min=1 max=50 step=0.1 keyIncr=S keyDecr=s" );
    mParams->addParam( "Init condition X", &mLorenzParams.mInitialCondition.x, "min=-50 max=50 step=0.01 keyIncr=X keyDecr=x" );
    mParams->addParam( "Init condition Y", &mLorenzParams.mInitialCondition.y, "min=-50 max=50 step=0.01 keyIncr=Y keyDecr=y" );
    mParams->addParam( "Init condition Z", &mLorenzParams.mInitialCondition.z, "min=-50 max=50 step=0.01 keyIncr=Z keyDecr=z" );
    mParams->addParam( "Auto increment initial X by 0.001", &mLorenzParams.mAutoIncementX, "keyIncr=1" );
    mParams->addParam( "Use RK4 integration", &mLorenzParams.mUseRK4, "keyIncr=/" );
    mParams->addSeparator();
    mParams->addButton( "Random initial condition", [this](){mLorenzParams.mInitialCondition = mRand.nextFloat(50.0f) * mRand.nextVec3f();}, "keyIncr=r" );
    mParams->addButton( "Random rotation", [this](){rotateModel(mRand.nextFloat(6.28f),mRand.nextFloat(6.28f));}, "keyIncr=t" );
    mParams->addButton( "Start iterative draw", [this](){mIterationCnt = 0;mIterativeDraw = true;}, "keyIncr=." );
    mParams->addButton( "Reset model", [this](){mLorenzParams=mOrigParams;}, "keyIncr=0" );
    //Vec3f rv = mRand.nextFloat(70.0f) * mRand.nextVec3f();
    mParams->addSeparator();
    mParams->addParam( "Frames per seconf (FPS)", &mAverageFps, "step=0.1", true );
}


/*
** This method builds the model. 
** Our model here can be thought of as having 3 components:
**
**   o The actual Lorenz equations solver makes the domain model. 
**     The other parts only help visualize the result;
**   o A single "VBO-ready" sphere mesh defined with vertices, positions and normals;
**   o The Cinder VBO mesh that holds everything together and interfaces with OpenGL.
**
** In this implementation we have dynamic positions and color; static vertices & normals.
*/
void LAxApp::initModel ()
{
    // Lorenz Equations Solver, starting from given initial condition
    mSolver = LorenzSolver( MAX_STEPS, Vec3f(0.1f, 0.1f, 0.1f) );
    // 
    // 3D sphere mesh model to visualize the solution
    mSphereModel = SphereMeshModel( MODEL_SPHERE_SLICES, MODEL_SPHERE_STACKS, 0.8f );
    // here we put the different parts of the model together;
    mModelNumElements = MAX_STEPS;
    mIndicesPerSphere = 6 * MODEL_SPHERE_SLICES * (MODEL_SPHERE_STACKS-1);
    uint32_t nVerticesPerSphere= MODEL_SPHERE_SLICES * (MODEL_SPHERE_STACKS-1) + 2;
    uint32_t nVertices = mModelNumElements * nVerticesPerSphere;
    uint32_t nIndices  = mModelNumElements * mIndicesPerSphere;
    gl::VboMesh::Layout layout;
    layout.setStaticIndices();
    layout.setStaticNormals();
    layout.setDynamicPositions();
    layout.setDynamicColorsRGB();
    vector<uint32_t> indices;
    indices.reserve( nIndices );  // this saves the vector from having to grow many times
    vector<Vec3f> normals;
    normals.reserve( nVertices ); // this saves the vector from having to grow many times
    for( int32_t i=0; i<mModelNumElements; i++ ) {
        mSphereModel.getStaticNormals( normals );
        mSphereModel.getStaticIndices( i * nVerticesPerSphere, indices );
    }
    assert( nIndices == indices.size() );
    assert( nVertices == normals.size() );
    mModelMesh = gl::VboMesh( nVertices, nIndices, layout, GL_TRIANGLES );
    mModelMesh.bufferIndices( indices );
    mModelMesh.bufferNormals( normals );
}


/*
** The application window has been resized: update anything window-bounds-sensitive.
*/
void LAxApp::resize()
{
    App::resize();
    updateCameraPerspective();
}


/*
** Update the camera perspective
*/
void LAxApp::updateCameraPerspective()
{
    mCam.lookAt( mCamEyePoint, mCamTarget, mCamUp );
    mCam.setPerspective( mCamFovAngle, getWindowAspectRatio(), 0.5f, 999.0f );
    gl::setMatrices( mCam );
}


/*
** update() is called on each frame before rendering.
** Updates anything that need to change between frames.
*/
void LAxApp::update()
{
    static size_t step = 0;

    mAverageFps = getAverageFps();
    if( mLorenzParams.mAutoIncementX ) {
        mLorenzParams.mInitialCondition.x += 0.001f;
    }

    //TODO rebuild model only on change
    {
        //mSolver.updateInitialCondition(0.0001f, 0.0f, 0.0f);
        //console() << "Updating the model..." << endl;
        //mNeed2updateModel = false;
        mSolver.useRK4( mLorenzParams.mUseRK4 );
        mSolver.setParameters( mLorenzParams.mParam_S, mLorenzParams.mParam_R, mLorenzParams.mParam_B );
        mSolver.setInitialConditions( mLorenzParams.mInitialCondition );
        mSolver.solve();
        vector<ci::Vec3f>& positions = mSolver.getSolutions();
        mCenterPos = mSolver.getCenterPos();
        Color clr = Color::black();
        gl::VboMesh::VertexIter vertexIter = mModelMesh.mapVertexBuffer();
        auto e = positions.begin();
        for( uint32_t i=0; e != positions.end(); ++e, i++ ) {
            // color by iteration count; starting blue, each following solution gets warmer.
            clr.r = float(i)/float(MAX_STEPS);
            clr.b = 1.0f - clr.r;
            clr.g = 0.33f; 
            // update the VBO positions and colors
            mSphereModel.updateVBO( vertexIter, *e, clr);
        }
    }
    if( mAutoRotate ) {
        rotateModel( 0.005f, 0.005f );
    }
    if( mIterativeDraw ) {
        //mIterationCnt++;
        if( mIterationCnt == 0 ) {
            step = 1;
        } else if( mIterationCnt == 50 ) {
            step = 2;
        } else if( mIterationCnt == 100 ) {
            step = 5;
        } else if( mIterationCnt == 150 ) {
            step = 10;
        } 
        if( mIterationCnt < mLorenzParams.mNumSteps ) {
            mIterationCnt += step ;
        } else {
            mIterativeDraw = false;
        }
    }
}


/*
** The actual rendering... Called once per each frame after update().
*/
void LAxApp::draw()
{
    // Render the model................................
    gl::clear( Color( 0.0f, 0.05f, 0.1f ) );
    glLightfv( GL_LIGHT0, GL_POSITION, (const GLfloat *) &mLightPosition );
    glEnable( GL_LIGHTING );
    gl::pushMatrices();
        if( mViewModelEnabled ) {
            gl::translate( -mCenterPos );
            if( mModelMesh ) {
                if( mIterativeDraw ) {
                    drawRange( mModelMesh, 0, mIterationCnt * mIndicesPerSphere);
                } else {
                    drawRange( mModelMesh, 0, mLorenzParams.mNumSteps * mIndicesPerSphere);
                    //gl::draw( mModelMesh );
                }
            }
        }
    gl::popMatrices();
    mParams->draw();
}


/*
** One of the mouse buttons is pressed
*/
void LAxApp::mouseDown( MouseEvent event )
{
    mCurrentMouseDown = mInitialMouseDown = event.getPos();
}


/*
** Mouse dragged: rotate the model
*/
void LAxApp::mouseDrag( MouseEvent event )
{
    mCurrentMouseDown = event.getPos();
    Vec2f dm = mCurrentMouseDown - mInitialMouseDown;
    //console() << "mouse: " << mInitialMouseDown << " : " << mCurrentMouseDown << " : " << dm << endl;
    rotateModel(float(-dm.x)/50.0f, float(dm.y)/50.0f);
    mInitialMouseDown = mCurrentMouseDown;
}


/*
** Mouse whell rotated: zoom in/out
*/
void LAxApp::mouseWheel( MouseEvent event )
{
    //console() << "mouse wheel: " << event.getWheelIncrement() << endl;
    zoom( -event.getWheelIncrement() );
}


/*
** rotateModel()
**
** This method creates an illusion of rotating the model.
** It actually rotates the camera and the light position 
** simultaneously around the center. In case it is desired 
** to appear that the camera rotates around the model -
** as opposed to the model rotating in front of the camera -
** then try to disable the light rotation and possibly swap 
** the left-right and the up-down rotation direction (+- sign) 
** correspondingly, for more intuitive result.
*/
void LAxApp::rotateModel( float leftRight, float upDown )
{
    Matrix44f rotationMatrix;
    if( leftRight != 0.0f ) {
        rotationMatrix = Matrix44f::createRotation( mCamUp, leftRight );
        mCamEyePoint = rotationMatrix * mCamEyePoint;
        mLightPosition = rotationMatrix * mLightPosition;
    }
    if( upDown != 0.0f ) {
        rotationMatrix = Matrix44f::createRotation( mCamEyePoint.cross(mCamUp), upDown );
        mCamEyePoint = rotationMatrix * mCamEyePoint;
        mCamUp = rotationMatrix * mCamUp;
        mLightPosition = rotationMatrix * mLightPosition;
    }
    updateCameraPerspective(); 
    //console() << "mCamEyePoint: " << mCamEyePoint << ", mCamUp: " << mCamUp << ", mLightPosition: " << mLightPosition << endl;
}


/*
** Camera zoom-in / zoom-out
*/
void LAxApp::zoom( float w )
{
    if( w < 0.0f && mCamFovAngle < 5 ) return;
    if( w > 0.0f && mCamFovAngle > 90 ) return;
    mCamFovAngle += w;
    updateCameraPerspective(); 
}


CINDER_APP_NATIVE( LAxApp, RendererGl )
