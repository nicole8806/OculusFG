/*
 * oculusupdateslavecallback.cpp
 *
 *  Created on: Jul 07, 2015
 *      Author: Björn Blissing
 */
#include "oculusupdateslavecallback.h"
#include <iostream>
//nicole---------------------------------------------
#include <osg/Camera>

#include "Renderer.hxx"

#include <osgViewer/Renderer>
#include <simgear/scene/material/EffectCullVisitor.hxx>

//#include "Drawable.hxx"
#include "Viewer.hxx"
//#include "SlaveCamera.hxx"

#include <simgear/scene/util/RenderConstants.hxx>

//nicole----------------------------------------
extern osg::Node::NodeMask cullMaskl;
//-------------------------------------------
void OculusUpdateSlaveCallback::updateSlave(osg::View& view, osg::View::Slave& slave) {
	if (m_cameraType == LEFT_CAMERA) {
		m_device->updatePose(m_swapCallback->frameIndex());
	}

	osg::Vec3 position = m_device->position();
	osg::Quat orientation = m_device->orientation();

	osg::Matrix viewOffset = (m_cameraType == LEFT_CAMERA) ? m_device->viewMatrixLeft() : m_device->viewMatrixRight();

	viewOffset.preMultRotate(orientation);
	viewOffset.preMultTranslate(position);

	//nicole,2015/12/09----------------------
	view.findSlaveForCamera(m_camera.get())->_viewOffset = viewOffset;
	//slave._viewOffset = viewOffset;
	//---------------------------------------

	slave.updateSlaveImplementation(view);

	if (m_warning.valid()) {
		m_warning.get()->updatePosition(view.getCamera()->getInverseViewMatrix(), position, orientation);
	}
	//nicole---------------------------------------------
		 m_camera->setCullMask(cullMaskl);
     m_camera->setCullMaskLeft(cullMaskl);
     m_camera->setCullMaskRight(cullMaskl);
	 //-------------------------------------------------------
}
