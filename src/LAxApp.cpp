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
#include "cinder/CinderMath.h"

#include "Resources.h"
#include "SphereMeshModel.h"
#include "LorenzSolver.h"


using namespace ci;
using namespace ci::app;
using namespace std;

#define NUM_ELEMENTS           3000    // Max number of steps (solutions) to be calculated and rendered
#define MODEL_SPHERE_STACKS 24
#define MODEL_SPHERE_SLICES 24


struct ModelDescriptor {
    uint32_t           mId;
    LorenzSolver       mSolver;
    gl::VboMesh        mModelMesh;
    //std::vector<Vec3f> mModelSolutions;
    Vec3f              mCenterPos;
    //uint32_t           mModelNumElements;
};


class LAxApp : public AppNative 
{
private:

    CameraPersp        mCam;
    Vec3f              mCamEyePoint, mCamTarget, mCamUp;
    Vec4f              mLightPosition;
    float              mCamFovAngle;
    float              mRotationStep;
    SphereMeshModel    mSphereModel;
    
    //LorenzSolver       mSolver;
    //gl::VboMesh        mModelMesh;
    uint32_t           mIndicesPerSphere;
    uint32_t           mNumStepsToRender;
    uint32_t           mIterationCnt;
    bool               mIterativeDraw;
    Vec2i              mCurrentMouseDown;
    Vec2i              mInitialMouseDown;
    Rand               mRand;
    gl::Texture        mInfoPanelTexture;
    Vec2i              mInfoPanelSize;
    bool               mDisplayInfoPanel;
    bool               mUpdateModel;
    Vec3f              mCenterPos;
    uint32_t           mNumModels;
    std::vector<ModelDescriptor> mModels;

public:

    void  prepareSettings( Settings *settings );
    void  setup();
    void  update();
    void  draw();
    void  resize();
    void  keyDown( KeyEvent event );
    void  mouseDown( MouseEvent event );
    void  mouseDrag( MouseEvent event );
    void  mouseWheel( MouseEvent event );

private:

    void  initModel();
    void  initInfoPanel();
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
    settings->enableConsoleWindow(true);
}


/*
** Application set-up. Executed once after application starts, 
** after prepareSettings() and before everything else.
*/
void LAxApp::setup()
{
    // Random numbers generator
    mRand = Rand();

    mNumModels = 2;

    // Info panel
    initInfoPanel();
    mDisplayInfoPanel = false;
    mUpdateModel = true;

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
    mNumStepsToRender = NUM_ELEMENTS;
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

    GLfloat lightColorDiffuse[] = {0.9f, 0.9f, 0.9f, 1.0f};
    GLfloat lightColorSpecular[] = { 0.4f, 0.4f, 0.4f, 1.0f };
    GLfloat lightColorAmbient[] = { 0.1f, 0.2f, 0.3f, 1.0f };
    glLightfv( GL_LIGHT0, GL_AMBIENT, lightColorAmbient );
    glLightfv( GL_LIGHT0, GL_DIFFUSE, lightColorDiffuse );
    glLightfv( GL_LIGHT0, GL_SPECULAR, lightColorSpecular );

    float materialSpecularRefl[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecularRefl);
    glMateriali(GL_FRONT, GL_SHININESS, 88);
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
    for( uint32_t i=0; i < mNumModels; i++ ) {
        ModelDescriptor md;
        mModels.push_back( md );
    }
    uint32_t mid = 0;
    for( auto mit=mModels.begin(); mit != mModels.end(); ++mit ) {
        mSphereModel = SphereMeshModel( MODEL_SPHERE_SLICES, MODEL_SPHERE_STACKS, 0.8f );
        //uint32_t numElements = MAX_STEPS;
        mIndicesPerSphere = 6 * MODEL_SPHERE_SLICES * (MODEL_SPHERE_STACKS-1);
        uint32_t nVerticesPerSphere= MODEL_SPHERE_SLICES * (MODEL_SPHERE_STACKS-1) + 2;
        uint32_t nVertices = NUM_ELEMENTS * nVerticesPerSphere;
        uint32_t nIndices  = NUM_ELEMENTS * mIndicesPerSphere;
        gl::VboMesh::Layout layout;
        layout.setStaticIndices();
        layout.setStaticNormals();
        layout.setDynamicPositions();
        layout.setDynamicColorsRGB();
        vector<uint32_t> indices;
        vector<Vec3f> normals;
        for( auto i=0; i<NUM_ELEMENTS; i++ ) {
            mSphereModel.getStaticNormals( normals );
            mSphereModel.getStaticIndices( i * nVerticesPerSphere, indices );
        }
        assert( nIndices == indices.size() );
        mit->mId = mid++;
        mit->mSolver = LorenzSolver( NUM_ELEMENTS, Vec3f(0.1f+0.001f*float(mid), 0.2f, 0.3f) );
        mit->mModelMesh = gl::VboMesh( nVertices, nIndices, layout, GL_TRIANGLES );
        mit->mModelMesh.bufferIndices( indices );
        mit->mModelMesh.bufferNormals( normals );
    }

    // Lorenz Equations Solver, starting from given initial condition
//    mSolver = LorenzSolver( MAX_STEPS, Vec3f(0.1f, 0.1f, 0.1f) );
    // 
    //mModelMesh = gl::VboMesh( nVertices, nIndices, layout, GL_TRIANGLES );
    //mModelMesh.bufferIndices( indices );
    //mModelMesh.bufferNormals( normals );
}


/*
** This callback is executed when the application window is resized.
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

    // Rebuild the model etc only on change.
    if( mUpdateModel ) {
        //console() << "Updating the model..." << endl;
        mUpdateModel = false;
        mCenterPos = Vec3f::zero();
        int i = 0;
        for( auto mit=mModels.begin(); mit != mModels.end(); ++mit ) {
            mit->mSolver.solve();
            vector<Vec3f> positions = mit->mSolver.getPositions();
            size_t numElements = positions.size();
            mCenterPos += mit->mSolver.getCenterPos();

            float h = 0.05f+float(i++)/mNumModels;
            Color clr = Color(CM_HSV, h, 0.75f, 0.9f);

            gl::VboMesh::VertexIter vertexIter = mit->mModelMesh.mapVertexBuffer();
            vector<Vec3f>::iterator e=positions.begin();
            for( uint32_t i=0; e != positions.end(); ++e, i++ ) {
                // update the VBO positions and colors
                mSphereModel.updateVBO( vertexIter, *e, clr);
            }

            ////Color clr = Color::black();
            ////gl::VboMesh::VertexIter vertexIter = mit->mModelMesh.mapVertexBuffer();
            ////vector<Vec3f>::iterator e=positions.begin();
            ////for( uint32_t i=0; e != positions.end(); ++e, i++ ) {
            ////    // color by iteration count; starting blue, each following solution gets warmer.
            ////    clr.r = 0.2f + 0.8f * float(i)/float(numElements);
            ////    clr.b = 0.2f + 0.8f * (1.0f-clr.r);
            ////    clr.g = 0.35f; 
            ////    // update the VBO positions and colors
            ////    mSphereModel.updateVBO( vertexIter, *e, clr);
            ////}
        }
        mCenterPos /= float(mNumModels);
        ////////vector<Vec3f> positions = mSolver.getPositions();
        ////////size_t numElements = positions.size();
        ////////mCenterPos = mSolver.getCenterPos();
        //////Color clr = Color::black();
        //////gl::VboMesh::VertexIter vertexIter = mModelMesh.mapVertexBuffer();
        //////vector<Vec3f>::iterator e=positions.begin();
        //////for( uint32_t i=0; e != positions.end(); ++e, i++ ) {
        //////    // color by iteration count; starting blue, each following solution gets warmer.
        //////    clr.r = 0.2f + 0.8f * float(i)/float(numElements);
        //////    clr.b = 0.2f + 0.8f * (1.0f-clr.r);
        //////    clr.g = 0.35f; 
        //////    // update the VBO positions and colors
        //////    mSphereModel.updateVBO( vertexIter, *e, clr);
        //////}
    }
    if( mIterativeDraw ) {
        //mIterationCnt++;
        if( mIterationCnt == 0 ) {
            step = 1;
        } else if( mIterationCnt == 30 ) {
            step = 2;
        } else if( mIterationCnt == 60 ) {
            step = 5;
        } else if( mIterationCnt == 90 ) {
            step = 10;
        } 
        if( mIterationCnt < mNumStepsToRender ) {
            mIterationCnt += 1;//step ;
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
        gl::translate( -mCenterPos );
        for( auto mit=mModels.begin(); mit != mModels.end(); ++mit ) {
            if( mit->mModelMesh ) {
                if( mIterativeDraw ) {
                    auto nv = min(uint32_t(50), mIterationCnt-1);
                    drawRange( mit->mModelMesh, (mIterationCnt-nv) * mIndicesPerSphere, nv * mIndicesPerSphere);
                } else {
                    drawRange( mit->mModelMesh, 0, mNumStepsToRender * mIndicesPerSphere);
                    //gl::draw( mModelMesh );
                }
            }
        }
    gl::popMatrices();
    //...............................and the info panel
    if( mDisplayInfoPanel ) {
        glDisable( GL_LIGHTING );
        glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
        gl::pushMatrices();
            gl::color( 1.0f, 1.0f, 1.0f, 0.66f );
            gl::setMatricesWindow( getWindowSize() );
            Vec2i textLoc = ( getWindowSize() - mInfoPanelSize ) / 2;
            Area textArea( textLoc, textLoc + mInfoPanelTexture.getSize() );
            gl::draw( mInfoPanelTexture, textLoc );
            gl::drawStrokedRect( textArea );
        gl::popMatrices();
    }
}


/*
** Keyboard input handler
*/
void LAxApp::keyDown( KeyEvent event ) 
{
    if( event.getChar() == '1' ) {
        for_each( mModels.begin(), mModels.end(), [](ModelDescriptor& m) { 
            m.mSolver.updateInitialCondition(0.001f, 0.0f, 0.0f);
        });
        mUpdateModel = true;
    } else if( event.getChar() == '2' ) {
        for_each( mModels.begin(), mModels.end(), [](ModelDescriptor& m) { 
            m.mSolver.updateInitialCondition(0.0f, 0.001f, 0.0f);
        });
        mUpdateModel = true;
    } else if( event.getChar() == '3' ) {
        for_each( mModels.begin(), mModels.end(), [](ModelDescriptor& m) { 
            m.mSolver.updateInitialCondition(0.0f, 0.0f, 0.001f);
        });
        mUpdateModel = true;
    } else if( event.getChar() == '4' ) {
        uint16_t i = 0;
        for_each( mModels.begin(), mModels.end(), [&i](ModelDescriptor& m) { 
            m.mSolver.setInitialCondition(0.1f+0.001f*float(++i), 0.2f, 0.3f);
        });
        mUpdateModel = true;
    } else if( event.getChar() == '5' ) {
        for_each( mModels.begin(), mModels.end(), [](ModelDescriptor& m) { 
            m.mSolver.setInitialCondition(12.0f,-41.0f,17.0f);
        });
        mUpdateModel = true;
    } else if( event.getChar() == '6' ) {
        for_each( mModels.begin(), mModels.end(), [](ModelDescriptor& m) { 
            m.mSolver.setInitialCondition(-4.0f,31.0f,-33.0f);
        });
        mUpdateModel = true;
    } else if( event.getChar() == '7' ) {
        for_each( mModels.begin(), mModels.end(), [](ModelDescriptor& m) { 
            m.mSolver.setInitialCondition(10.2f, -41.7f, -47.8f);
        });
        mUpdateModel = true;
    } else if( event.getChar() == 'r' ) {
        // random initial condition
        for_each( mModels.begin(), mModels.end(), [=](ModelDescriptor& m) { 
            Vec3f rv = mRand.nextFloat(70.0f) * mRand.nextVec3f();
            console() << "Init condition: " << rv << endl;
            m.mSolver.setInitialCondition( rv.x, rv.y, rv.z );
        });
        //mSolver.setInitialCondition( rv.x, rv.y, rv.z );
        mUpdateModel = true;
    } else if( event.getChar() == 't' ) {
        rotateModel(mRand.nextFloat(6.28f), mRand.nextFloat(6.28f) );
    } else if( event.getChar() == ',' ) {
        mNumStepsToRender =
            mNumStepsToRender == NUM_ELEMENTS     ? NUM_ELEMENTS/100 :
            mNumStepsToRender == NUM_ELEMENTS/100 ? NUM_ELEMENTS/10  :
            mNumStepsToRender == NUM_ELEMENTS/10  ? NUM_ELEMENTS/2   :
                                                    NUM_ELEMENTS     ;
        console() << "mNumStepsToRender: " << mNumStepsToRender << endl;
    } else if( event.getCode() == app::KeyEvent::KEY_LEFT ) {
        // rotate left
        rotateModel( mRotationStep, 0.0f );
    } else if( event.getCode() == app::KeyEvent::KEY_RIGHT ) {
        // rotate right
        rotateModel( -mRotationStep, 0.0f );
    } else if( event.getCode() == app::KeyEvent::KEY_UP ) {
        // rotate up
        rotateModel( 0.0f, -mRotationStep );
    } else if( event.getCode() == app::KeyEvent::KEY_DOWN ) {
        // rotate down
        rotateModel( 0.0f, mRotationStep );
    } else if( event.getChar() == '+' ) {
        zoom(-1.0f);
    } else if( event.getChar() == '-' ) {
        zoom(1.0f);
    } else if( event.getChar() == '/' ) {
        for_each( mModels.begin(), mModels.end(), [](ModelDescriptor& m) { 
            m.mSolver.useRK4Toggle();
        });
        mUpdateModel = true;
    } else if( event.getChar() == '.' ) {
        mIterationCnt = 0;
        mIterativeDraw = true; //! mIterativeDraw;
    } else if( event.getChar() == 'z' ) {
        for_each( mModels.begin(), mModels.end(), [](ModelDescriptor& m) { 
            m.mSolver.setIntegrationStep( 0.01f, 1 );
        });
        mUpdateModel = true;
    } else if( event.getChar() == 'x' ) {
        for_each( mModels.begin(), mModels.end(), [](ModelDescriptor& m) { 
            m.mSolver.setIntegrationStep( 0.001f, 10 );
        });
        mUpdateModel = true;
    } else if( event.getChar() == 'c' ) {
        for_each( mModels.begin(), mModels.end(), [](ModelDescriptor& m) { 
            m.mSolver.setIntegrationStep( 0.0001f, 100 );
        });
        mUpdateModel = true;
    } else if( event.getChar() == '?' ) {
        mDisplayInfoPanel = ! mDisplayInfoPanel;
    }
    
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
    zoom( event.getWheelIncrement() );
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


/*
** Init the info panel.
*/
void LAxApp::initInfoPanel() 
{
    TextLayout layout;
    layout.clear( ColorA::black() );
    layout.setColor(ColorA::white() );
    layout.setBorder(10, 10);
    layout.setFont( Font( "Arial", 18.0 ) );
    layout.addLine( "Lorenz Attractor Explorer" );
    layout.addLine( " " );
    layout.addLine( "1   increase initial condition x by 0.0001" );
    layout.addLine( "2   increase initial condition y by 0.0001" );
    layout.addLine( "3   increase initial condition z by 0.0001" );
    layout.addLine( "4   reset the initial condition" );
    layout.addLine( "r   random initial condition" );
    layout.addLine( "t   random model rotation" );
    layout.addLine( ".  (period) start iterative draw" );
    layout.addLine( "/   toggle RK4 / Euler integration" );
    layout.addLine( ",  (comma) toggle number steps to render" );
    layout.addLine( "z   reset integration step to 0.01" );
    layout.addLine( "x   set integration step to 0.001" );
    layout.addLine( "c   set integration step to 0.0001" );
    layout.addLine( "   " );
    layout.addLine( "arrows, mouse drag - rotate model   " );
    layout.addLine( "+|-, mouse wheel - zoom in/out   " );
    layout.addLine( "   " );
    layout.addLine( "?   toggle this information panel" );
    Surface8u rendered = layout.render( false, false );
    mInfoPanelSize = rendered.getSize();
    //console() << "mInfoPanelSize: " << mInfoPanelSize << endl;
    mInfoPanelTexture = gl::Texture( rendered );
}


CINDER_APP_NATIVE( LAxApp, RendererGl )
