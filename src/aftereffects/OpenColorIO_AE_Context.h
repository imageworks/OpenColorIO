/*
 *  OpenColorIO_AE_Context.h
 *  OpenColorIO_AE
 *
 *  Created by Brendan Bolles on 11/22/11.
 *  Copyright 2011 fnord. All rights reserved.
 *
 */

#ifndef _OPENCOLORIO_AE_CONTEXT_H_
#define _OPENCOLORIO_AE_CONTEXT_H_

#include "OpenColorIO_AE.h"

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;


#include <string>
#include <vector>

class OpenColorIO_AE_Context
{
  public:
	OpenColorIO_AE_Context(const std::string path);
	OpenColorIO_AE_Context(const ArbitraryData *arb_data);
	~OpenColorIO_AE_Context() {}
	
	OCIO::ConstProcessorRcPtr & processor() { return _processor; }
	
	void setupOCIO(const char *input, const char *transform, const char *device);
	
	typedef std::vector<std::string> SpaceVec;
	
	bool isOCIO() const { return _ocio; }
	const std::string & getInput() const { return _input; }
	const std::string & getTransform() const { return _transform; }
	const std::string & getDevice() const { return _device; }
	const SpaceVec & getInputs() const { return _inputs; }
	const SpaceVec & getTransforms() const { return _transforms; }
	const SpaceVec & getDevices() const { return _devices; }

  private:
	void setupLUT(const char *path);
  
	OCIO::ConstConfigRcPtr		_config;
	OCIO::ConstProcessorRcPtr	_processor;
	
	bool _ocio;
	std::string _input;
	std::string _transform;
	std::string _device;
	SpaceVec _inputs;
	SpaceVec _transforms;
	SpaceVec _devices;
};


#endif // _OPENCOLORIO_AE_CONTEXT_H_