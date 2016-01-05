/*
 * oculusviewconfig.cpp
 *
 *  Created on: Sept 26, 2013
 *      Author: Bjorn Blissing & Jan Ciger
 */
#include "oculusviewconfig.h"

#include "oculuseventhandler.h"
#include "oculusupdateslavecallback.h"

//xuy----
#include <simgear/compiler.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <vector>
#include <typeinfo>

#include <boost/foreach.hpp>

#include <osg/ref_ptr>
#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/Camera>
#include <osg/CullFace>
#include <osg/CullStack>
#include <osg/Depth>
#include <osg/Fog>
#include <osg/Group>
#include <osg/Hint>
#include <osg/Light>
#include <osg/LightModel>
#include <osg/LightSource>
#include <osg/Material>
#include <osg/Math>
#include <osg/NodeCallback>
#include <osg/Notify>
#include <osg/PolygonMode>
#include <osg/PolygonOffset>
#include <osg/Program>
#include <osg/Version>
#include <osg/TexEnv>

#include <osgUtil/LineSegmentIntersector>

#include <osg/io_utils>
#include <osgDB/WriteFile>
#include <osgViewer/Renderer>

#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/material/EffectCullVisitor.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/material/EffectBuilder.hxx>
#include <simgear/scene/model/animation.hxx>
#include <simgear/scene/model/placement.hxx>
#include <simgear/scene/sky/sky.hxx>
#include <simgear/scene/util/DeletionManager.hxx>
#include <simgear/scene/util/SGUpdateVisitor.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>
#include <simgear/scene/tgdb/GroundLightManager.hxx>
#include <simgear/scene/tgdb/pt_lights.hxx>
#include <simgear/scene/tgdb/userdata.hxx>
#include <simgear/structure/OSGUtils.hxx>
#include <simgear/props/props.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/ephemeris/ephemeris.hxx>
#include <simgear/math/sg_random.h>

#include <Time/light.hxx>
#include <Time/light.hxx>
#include <Cockpit/panel.hxx>

#include <Model/panelnode.hxx>
#include <Model/modelmgr.hxx>
#include <Model/acmodel.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/redout.hxx>
#include <GUI/new_gui.hxx>
#include <Instrumentation/HUD/HUD.hxx>
#include <Environment/precipitation_mgr.hxx>
#include <Environment/environment_mgr.hxx>

//#include <Main/main.hxx>
//#include "viewer.hxx"
//#include "viewmgr.hxx"
//#include "splash.hxx"
//#include "renderer.hxx"
//#include "CameraGroup.hxx"
//#include "FGEventHandler.hxx"

#include <plib/pu.h>

using namespace osg;
using namespace simgear;
using namespace flightgear;

void installCullVisitor(Camera* camera)
{
    osgViewer::Renderer* renderer
        = static_cast<osgViewer::Renderer*>(camera->getRenderer());
    for (int i = 0; i < 2; ++i) {
        osgUtil::SceneView* sceneView = renderer->getSceneView(i);

        osg::ref_ptr<osgUtil::CullVisitor::Identifier> identifier;
        identifier = sceneView->getCullVisitor()->getIdentifier();
        sceneView->setCullVisitor(new simgear::EffectCullVisitor);
        sceneView->getCullVisitor()->setIdentifier(identifier.get());

        identifier = sceneView->getCullVisitorLeft()->getIdentifier();
        sceneView->setCullVisitorLeft(sceneView->getCullVisitor()->clone());
        sceneView->getCullVisitorLeft()->setIdentifier(identifier.get());

        identifier = sceneView->getCullVisitorRight()->getIdentifier();
        sceneView->setCullVisitorRight(sceneView->getCullVisitor()->clone());
        sceneView->getCullVisitorRight()->setIdentifier(identifier.get());

    }
}
//---
/* Public functions */
void OculusViewConfig::configure(osgViewer::View& view) const
{
	// Create a graphic context based on our desired traits
	osg::ref_ptr<osg::GraphicsContext::Traits> traits = m_device->graphicsContextTraits();
	osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits);
	if (!gc) {
		osg::notify(osg::NOTICE) << "Error, GraphicsWindow has not been created successfully" << std::endl;
		return;
	}

	// Attach to window, needed for direct mode
	m_device->attachToWindow(gc);
	
	// Attach a callback to detect swap
	osg::ref_ptr<OculusSwapCallback> swapCallback = new OculusSwapCallback(m_device);
	gc->setSwapCallback(swapCallback);

	osg::ref_ptr<osg::Camera> camera = view.getCamera();
	camera->setName("Main");
	// Use full view port
	camera->setViewport(new osg::Viewport(0, 0, traits->width, traits->height));
	// Disable automatic computation of near and far plane on main camera, will propagate to slave cameras
	camera->setComputeNearFarMode( osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR );
	const int textureWidth  = m_device->renderTargetWidth()/2;
	const int textureHeight = m_device->renderTargetHeight();
	// master projection matrix
	camera->setProjectionMatrix(m_device->projectionMatrixCenter());
	// Create textures for RTT cameras
	osg::ref_ptr<osg::Texture2D> textureLeft = new osg::Texture2D;
	textureLeft->setTextureSize( textureWidth, textureHeight );
	textureLeft->setInternalFormat( GL_RGBA );
	osg::ref_ptr<osg::Texture2D> textureRight = new osg::Texture2D;
	textureRight->setTextureSize( textureWidth, textureHeight );
	textureRight->setInternalFormat( GL_RGBA );
	// Create RTT cameras and attach textures
	osg::ref_ptr<osg::Camera> cameraRTTLeft = m_device->createRTTCamera(textureLeft, OculusDevice::LEFT, osg::Camera::RELATIVE_RF, gc);
	osg::ref_ptr<osg::Camera> cameraRTTRight = m_device->createRTTCamera(textureRight, OculusDevice::RIGHT, osg::Camera::RELATIVE_RF, gc);
	cameraRTTLeft->setName("LeftRTT");
	cameraRTTRight->setName("RightRTT");
	
	// Create warp ortho camera
	osg::ref_ptr<osg::Camera> cameraWarp = m_device->createWarpOrthoCamera(0.0, 1.0, 0.0, 1.0, gc);
	cameraWarp->setName("WarpOrtho");
	cameraWarp->setViewport(new osg::Viewport(0, 0, m_device->screenResolutionWidth(), m_device->screenResolutionHeight()));

	// Create shader program
	osg::ref_ptr<osg::Program> program = m_device->createShaderProgram();

	// Create distortionMesh for each camera
	osg::ref_ptr<osg::Geode> leftDistortionMesh = m_device->distortionMesh(OculusDevice::LEFT, program, 0, 0, textureWidth, textureHeight);
	cameraWarp->addChild(leftDistortionMesh);

	osg::ref_ptr<osg::Geode> rightDistortionMesh = m_device->distortionMesh(OculusDevice::RIGHT, program, 0, 0, textureWidth, textureHeight);
	cameraWarp->addChild(rightDistortionMesh);

	// Add pre draw camera to handle time warp
	cameraWarp->setPreDrawCallback(new WarpCameraPreDrawCallback(m_device));

	// Attach shaders to each distortion mesh
	osg::ref_ptr<osg::StateSet> leftEyeStateSet = leftDistortionMesh->getOrCreateStateSet();
	osg::ref_ptr<osg::StateSet> rightEyeStateSet = rightDistortionMesh->getOrCreateStateSet();

	m_device->applyShaderParameters(leftEyeStateSet, program.get(), textureLeft.get(), OculusDevice::LEFT);
	m_device->applyShaderParameters(rightEyeStateSet, program.get(), textureRight.get(), OculusDevice::RIGHT);

	// Add RTT cameras as slaves, specifying offsets for the projection
	view.addSlave(cameraRTTLeft, 
		m_device->projectionOffsetMatrixLeft(),
		m_device->viewMatrixLeft(), 
		true);
	installCullVisitor(cameraRTTLeft);
	//nicole,2015/12/09----------------------------------
	view.getSlave(0)._updateSlaveCallback = new OculusUpdateSlaveCallback(OculusUpdateSlaveCallback::LEFT_CAMERA, m_device.get(), swapCallback.get(), m_warning.get(),cameraRTTLeft);
	//-----------------------------------------------------------
	view.addSlave(cameraRTTRight, 
		m_device->projectionOffsetMatrixRight(),
		m_device->viewMatrixRight(),
		true);
	installCullVisitor(cameraRTTRight);
	//nicole,2015/12/09-------------------------------------
	view.getSlave(1)._updateSlaveCallback = new OculusUpdateSlaveCallback(OculusUpdateSlaveCallback::RIGHT_CAMERA, m_device.get(), swapCallback.get(), 0,cameraRTTRight);
	//-----------------------------------------------------

	// Use sky light instead of headlight to avoid light changes when head movements
	view.setLightingMode(osg::View::SKY_LIGHT);

	// Add warp camera as slave
	view.addSlave(cameraWarp, false);
	view.setName("Oculus");

	// Add Oculus keyboard handler
	view.addEventHandler(new OculusEventHandler(m_device));
	view.addEventHandler(new OculusWarningEventHandler(m_device, m_warning));

	// Start HSW timer
	m_device->getHealthAndSafetyDisplayState();
}


