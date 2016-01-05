/*
 * oculusupdateslavecallback.h
 *
 *  Created on: Jul 07, 2015
 *      Author: Björn Blissing
 */
#include <osgViewer/View>
#include "oculusdevice.h"
#include "oculushealthwarning.h"

struct OculusUpdateSlaveCallback : public osg::View::Slave::UpdateSlaveCallback
{
	enum CameraType
	{
		LEFT_CAMERA,
		RIGHT_CAMERA
	};

	OculusUpdateSlaveCallback(CameraType cameraType, OculusDevice* device, OculusSwapCallback* swapCallback, OculusHealthAndSafetyWarning* warning, osg::ref_ptr<osg::Camera> camera) :
		m_cameraType(cameraType),
		m_device(device),
		m_swapCallback(swapCallback),
		m_warning(warning),
		//nicole,2015/12/09---------------------
		m_camera(camera)
		//--------------------------------------
		{}

	virtual void updateSlave(osg::View& view, osg::View::Slave& slave);

	CameraType m_cameraType;
	osg::ref_ptr<OculusDevice> m_device;
	//nicole,2015/12/09---------------------
	osg::ref_ptr<osg::Camera> m_camera;
	//--------------------------------------------
	osg::ref_ptr<OculusSwapCallback> m_swapCallback;
	osg::ref_ptr<OculusHealthAndSafetyWarning> m_warning;
};
