#include "SSAONode.h"
#include <osg/Texture2D>
#include <osg/Texture>
#include <osgDB/ReadFile> 
#include <osgDB/FileUtils>
#include <osgViewer/View>
#include <QTextStream>
#include <QFile>

// Default settings constructor
SSAONode::SSAONode(int width,
     int height,
     int kernelSize,
     int noiseSize,
     int blurSize,
     float radius,
     float power)
     : m_kernelSize(kernelSize),
       m_noiseSize(noiseSize),
       m_blurSize(blurSize),
       m_ssaoRadius(radius),
       m_ssaoPower(power),
       m_blurAOEnabled(true),
       m_haloRemovalEnabled(true),
       m_haloTreshold(radius),
       m_width(width),
       m_height(height),

       m_kernelData(nullptr),
       m_noiseData(nullptr),

       displayType(SSAO_ColorAndAO)
{
    Initialize();
}

SSAONode::~SSAONode() {
    if (m_kernelData != NULL) {
        delete[] m_kernelData;
        m_kernelData = NULL;
    }
    if (m_noiseData != NULL) {
        delete[] m_noiseData;
        m_noiseData = NULL;
    }
}


void SSAONode::addKernelUniformToStateSet(osg::StateSet *stateset, int kernelLength)
{
    // Set ssao sampling kernel as uniform
    osg::Uniform *ssaoKernelUniform =
            new osg::Uniform(osg::Uniform::FLOAT_VEC3,
                             "ssaoKernel",
                             kernelLength);

    // Create hemisphere kernel and noise texture
    if (m_kernelData == nullptr) {
        m_kernelData = new osg::Vec3f[kernelLength];
        generateHemisphereSamples(m_kernelData, kernelLength);
    }

    for (int i = 0; i < kernelLength; i++) {
        ssaoKernelUniform->setElement(i, m_kernelData[i]);
    }

    stateset->addUniform(ssaoKernelUniform);
}

void SSAONode::removeAttachedCameras()
{
    int numChildren = this->getNumChildren();

    for (int i = numChildren - 1; i >= 0; i--)
    {
        osg::Camera* cam = this->getChild(i)->asCamera();
        if (cam)
        {
            this->removeChild(cam);
            //delete &cam;
        }
    }
}

void SSAONode::createFirstPassCamera()
{
    // Initialize programmable pipeline for multipass deferred rendering

    // -------------------------------------------------------------------------

    // Create texture for deferred rendering (1st pass) - G-Buffer: Color
    colorTex = new osg::Texture2D();
    colorTex->setTextureSize(m_width, m_height);
    colorTex->setInternalFormat(GL_RGB);

    // Create texture for deferred rendering (1st pass) - G-Buffer: Depth
    linearDepthTex = new osg::Texture2D();
    linearDepthTex->setTextureSize(m_width, m_height);
    linearDepthTex->setInternalFormat(GL_DEPTH_COMPONENT24);
    linearDepthTex->setSourceFormat(GL_DEPTH_COMPONENT);
    linearDepthTex->setSourceType(GL_FLOAT);

    // Create texture for deferred rendering (1st pass) - G-Buffer: Normal
    normalTex = new osg::Texture2D;
    normalTex->setTextureSize(m_width, m_height);
    normalTex->setInternalFormat(GL_RGB);

    // Create camera for rendering to texture (to G-buffer)
    rttCamera = createRTTCameraGBuffer(osg::Camera::DEPTH_BUFFER,
                                       linearDepthTex.get(),
                                       osg::Camera::COLOR_BUFFER0,
                                       colorTex.get(),
                                       osg::Camera::COLOR_BUFFER1,
                                       normalTex.get(),
                                       false);

    // Load deferred phong shader for rendering scene into G-buffer
    // (color + view space normal + linear depth)
    osg::StateSet* phongState = rttCamera->getOrCreateStateSet();

    osg::Program* phongProgramObject = new osg::Program;
    osg::Shader* phongVertexObject = new osg::Shader(osg::Shader::VERTEX);
    osg::Shader* phongFragmentObject = new osg::Shader(osg::Shader::FRAGMENT);
    phongProgramObject->addShader(phongFragmentObject);
    phongProgramObject->addShader(phongVertexObject);

    setShaderStringFromResource(phongVertexObject, ":/shaders/phong.vp");
    setShaderStringFromResource(phongFragmentObject, ":/shaders/phong.fp");

    phongState->setAttributeAndModes(phongProgramObject,
                                     osg::StateAttribute::ON);

    rttCamera->setRenderOrder(osg::Camera::PRE_RENDER, 0);
    this->addChild(rttCamera.get());

}

void SSAONode::createSecondPassCamera(int kernelLength)
{
    // Create texture for deferred rendering (2nd pass - blur)
    secondPassTex = new osg::Texture2D;
    secondPassTex->setTextureSize(m_width, m_height);
    secondPassTex->setInternalFormat(GL_RGBA);

    // Create ssao camera for deffered rendering (first pass)
    ssaoCamera = createRTTCamera(osg::Camera::COLOR_BUFFER,
                                 secondPassTex.get(),
                                 true);

    // Load ssao shader
    osg::ref_ptr<osg::Program> ssaoProgram = new osg::Program;
    osg::Shader* ssaoVertexObject = new osg::Shader(osg::Shader::VERTEX);
    osg::Shader* ssaoFragmentObject = new osg::Shader(osg::Shader::FRAGMENT);
    ssaoProgram->addShader(ssaoFragmentObject);
    ssaoProgram->addShader(ssaoVertexObject);
    setShaderStringFromResource(ssaoVertexObject, ":/shaders/ssao.vp");
    setShaderStringFromResource(ssaoFragmentObject, ":/shaders/ssao.fp");

    osg::StateSet* stateset = ssaoCamera->getOrCreateStateSet();

    stateset->setTextureAttributeAndModes(0, colorTex.get());
    stateset->setTextureAttributeAndModes(1, createTexture2D(m_noiseSize,
                                                             m_noiseSize,
                                                             m_noiseData));
    stateset->setTextureAttributeAndModes(2, linearDepthTex.get());
    stateset->setTextureAttributeAndModes(3, normalTex.get());

    stateset->setAttributeAndModes(ssaoProgram.get());
    stateset->addUniform(new osg::Uniform("colorTexture", 0));
    stateset->addUniform(new osg::Uniform("noiseTexture", 1));
    stateset->addUniform(new osg::Uniform("linearDepthTexture", 2));
    stateset->addUniform(new osg::Uniform("normalTexture", 3));

    radiusUniform = new osg::Uniform("ssaoRadius", m_ssaoRadius);
    stateset->addUniform(radiusUniform);

    powerUniform = new osg::Uniform("ssaoPower", m_ssaoPower);
    stateset->addUniform(powerUniform);


    stateset->addUniform(new osg::Uniform("kernelSize", kernelLength));

    noiseTextureRcpUniform =
            new osg::Uniform("noiseTextureRcp",
                             osg::Vec2f(float(m_width) / float(m_noiseSize),
                                        (float(m_height)/float(m_noiseSize))));
    stateset->addUniform(noiseTextureRcpUniform);

    projMatUniform = new osg::Uniform(osg::Uniform::FLOAT_MAT4, "projMatrix", 1);
    stateset->addUniform(projMatUniform);

    invProjMatrixUniform = new osg::Uniform(osg::Uniform::FLOAT_MAT4, "invProjMatrix", 1);
    stateset->addUniform(invProjMatrixUniform);

    addKernelUniformToStateSet(stateset, kernelLength);

    ssaoCamera->setRenderOrder(osg::Camera::PRE_RENDER, 1);
    this->addChild(ssaoCamera.get());

}

void SSAONode::createThirdPassCamera()
{
    // Create blur camera for deffered rendering (second pass)
    blurCamera = createHUDCamera(0.0, 1.0, 0.0, 1.0);
    blurCamera->addChild(createScreenQuad(1.0f, 1.0f));

    // Load blur shader
    osg::ref_ptr<osg::Program> blurProgram = new osg::Program;
    osg::Shader* blurVertexObject = new osg::Shader(osg::Shader::VERTEX);
    osg::Shader* blurFragmentObject = new osg::Shader(osg::Shader::FRAGMENT);
    blurProgram->addShader(blurFragmentObject);
    blurProgram->addShader(blurVertexObject);
    setShaderStringFromResource(blurVertexObject, ":/shaders/blur.vp");
    setShaderStringFromResource(blurFragmentObject, ":/shaders/blur.fp");

    // Set blur shader to blur camera
    osg::StateSet* statesetBlur = blurCamera->getOrCreateStateSet();
    statesetBlur->setTextureAttributeAndModes(0, secondPassTex.get()); // Set screen texture from first pass to texture channel 0
    statesetBlur->setTextureAttributeAndModes(1, linearDepthTex.get());
    statesetBlur->setAttributeAndModes(blurProgram.get());

    statesetBlur->addUniform(new osg::Uniform("sceneTex", 0));
    statesetBlur->addUniform(new osg::Uniform("linearDepthTexture", 1));
    statesetBlur->addUniform(new osg::Uniform("uBlurSize", int(m_blurSize)));

    haloRemovalUniform = new osg::Uniform("haloRemoval", m_haloRemovalEnabled ? 1 : 0);
    haloTresholdUniform = new osg::Uniform("haloTreshold", m_haloTreshold);
    blurAOUniform = new osg::Uniform("blurAOEnable", m_blurAOEnabled ? 1 : 0);

    statesetBlur->addUniform(haloRemovalUniform);
    statesetBlur->addUniform(haloTresholdUniform);
    statesetBlur->addUniform(blurAOUniform);

    blurProjMatrixUniform = new osg::Uniform(osg::Uniform::FLOAT_MAT4, "projMatrix", 1);
    statesetBlur->addUniform(blurProjMatrixUniform);

    sceneSizeUniform = new osg::Uniform("sceneSize", osg::Vec2f(m_width, m_height));
    statesetBlur->addUniform(sceneSizeUniform);

    displayTypeUniform = new osg::Uniform("displayType", displayType);
    statesetBlur->addUniform(displayTypeUniform);

    // Ensure rendering order
    blurCamera->setRenderOrder(osg::Camera::POST_RENDER, 0);

    this->addChild(blurCamera.get());
}

void SSAONode::Initialize()
{
    int kernelLength = m_kernelSize * m_kernelSize;

    if (m_noiseData == NULL)
    {
        long nsquared = m_noiseSize * m_noiseSize;
        m_noiseData = new osg::Vec3f[nsquared];
        generateNoise(m_noiseData, nsquared);
    }

    removeAttachedCameras();

    createFirstPassCamera();

    createSecondPassCamera(kernelLength);

    createThirdPassCamera();
	// ------------------------------------------------------------------------------------------------------------------------

	// ------------------------------------------------------------------------------------------------------------------------
		
	// Set user definable uniforms
	setUniforms();

	// Create ssao group
}

bool SSAONode::IsHaloRemovalEnabled() {
    return this->m_haloRemovalEnabled;
}

bool SSAONode::IsAOBlurEnabled() {
    return this->m_blurAOEnabled;
}

void SSAONode::SetHaloTreshold(float treshold) {
    this->m_haloTreshold = treshold;
}

float SSAONode::GetHaloTreshold() {
    return this->m_haloTreshold;
}

void SSAONode::addNode(osg::Node* node)
{
    rttCamera->addChild(node);
}

void SSAONode::Resize(int width, int height)
{
    if (width == this->m_width && height == this->m_height) return;

    m_width = width;
    m_height = height;

    // Remember nodes attached to ssao
    unsigned int nodeNum = rttCamera->getNumChildren();

    std::vector<osg::Node*> nodes;

    for (unsigned int i = 0; i < nodeNum; ++i){
        nodes.push_back(rttCamera->getChild(i));
    }

    // Initialize again
    Initialize();

    // Add nodes back
    for (unsigned int i = 0; i < nodeNum; ++i){
        addNode(nodes[i]);
    }

    sceneSizeUniform->set(osg::Vec2f(width, height));
    noiseTextureRcpUniform->set(osg::Vec2f(float(width) / float(m_noiseSize), (float(height) / float(m_noiseSize))));
}

void SSAONode::SetSSAORadius(float radius) {
    this->m_ssaoRadius = radius;
    setUniforms();
}

void SSAONode::SetSSAOPower(float power) {
    this->m_ssaoPower = power;
    setUniforms();
}

void SSAONode::SetDisplayMode(SSAONode::DisplayMode mode)
{
	this->displayType = mode;
    setUniforms();
}

float SSAONode::GetSSAORadius() {
    return this->m_ssaoRadius;
}

float SSAONode::GetSSAOPower() {
    return this->m_ssaoPower;
}

SSAONode::DisplayMode SSAONode::GetDisplayMode() {
	return this->displayType;
}

void SSAONode::setProjectionMatrixUniforms()
{
    projMatUniform->set(projMatrix);
    invProjMatrixUniform->set(osg::Matrixd::inverse(projMatrix));
    blurProjMatrixUniform->set(projMatrix);
}

void SSAONode::setUniforms()
{
    setProjectionMatrixUniforms();
    radiusUniform->set(m_ssaoRadius);
    powerUniform->set(m_ssaoPower);
    displayTypeUniform->set((int) displayType);
    haloRemovalUniform->set(m_haloRemovalEnabled ? 1 : 0);
    haloTresholdUniform->set(m_haloTreshold);
    blurAOUniform->set(m_blurAOEnabled ? 1 : 0);
}

void SSAONode::updateProjectionMatrix(osg::Matrixd projMatrix)
{
    this->projMatrix = projMatrix;
    setProjectionMatrixUniforms();
}

// Random number generator
unsigned int SSAONode::xorshift32() {
	static unsigned int x = 1424447641;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return x;
}

float SSAONode::random(float min, float max) {
    return min + static_cast <float> (xorshift32()) /
            ( static_cast <float> (0xFFFFFFFF/(max-min)) );
}

float SSAONode::lerp(float min, float max, float t) {
	return min * (1.0f - t) + max * t;
}


void SSAONode::generateHemisphereSamples(osg::Vec3f* kernel, size_t kernelSize)
{
    QFile f("kernel.obj");
    f.open(QIODevice::WriteOnly);
    QTextStream fs(&f);
    fs << "o kernel" << "\n";
    for (size_t i = 0; i < kernelSize; ++i) {

        // Actually, do not generate hemisphere,
        // but a cone to minimize self-intersections
		kernel[i] = osg::Vec3f(
            random(-0.95f, 0.95f),
			random(-0.95f, 0.95f),
            random(0.0f, 1.0f));

        kernel[i].normalize();
        fs << "v "
           << kernel[i].x()
           << " "
           << kernel[i].y()
           << " "
           << kernel[i].z()
           << "\n";

#if 1
        // XXX why are we scaling by the index into the kernel?
        float scale = float(i) / float(kernelSize);
		scale = lerp(0.1f, 1.0f, scale * scale);
		kernel[i] *= scale;
#endif
        fs << "f " << i << "\n";
    }
    fs.flush();
    f.flush();
    f.close();
}

void SSAONode::generateNoise(osg::Vec3f* noise, size_t noiseSize) {

	for (size_t i = 0; i < noiseSize; ++i) {
		noise[i] = osg::Vec3f(
			random(-1.0f, 1.0f),
			random(-1.0f, 1.0f),
			0.0f
			);

		noise[i].normalize();

		// Scale to <0; 1> - it will be stored in RGB24
		noise[i] = noise[i] + osg::Vec3f(1.0f, 1.0f, 1.0f);
		noise[i] = noise[i] / 2.0f;
	}
}



osg::Camera* SSAONode::createRTTCamera(osg::Camera::BufferComponent buffer, osg::Texture* tex, bool isAbsolute)
{
	osg::ref_ptr<osg::Camera> camera = new osg::Camera;
	camera->setClearColor( osg::Vec4() );
	camera->setClearMask( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT );
	camera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT );
	camera->setRenderOrder( osg::Camera::PRE_RENDER );
	if ( tex )
	{
		tex->setFilter( osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR );
		tex->setFilter( osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR );
		camera->setViewport( 0, 0, tex->getTextureWidth(), tex->getTextureHeight() );
		camera->attach( buffer, tex );
	}

	if ( isAbsolute )
	{
		camera->setReferenceFrame( osg::Transform::ABSOLUTE_RF );
		camera->setProjectionMatrix( osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0) );
		camera->setViewMatrix( osg::Matrix::identity() );
        camera->addChild( createScreenQuad(1.0f, 1.0f) );
	}
	return camera.release();
}

osg::Camera* SSAONode::createRTTCameraGBuffer( osg::Camera::BufferComponent buffer1,
                                           osg::Texture* tex1,
                                           osg::Camera::BufferComponent buffer2,
                                           osg::Texture* tex2,
                                           osg::Camera::BufferComponent buffer3,
                                           osg::Texture* tex3,
                                           bool isAbsolute )
{
	osg::ref_ptr<osg::Camera> camera = new osg::Camera;
    camera->setClearColor( osg::Vec4(0.2, 0.2, 0.4, 1.0) );
	camera->setClearMask( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT );
	camera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT );
	camera->setRenderOrder( osg::Camera::PRE_RENDER );
	if ( tex1 )
	{
		tex1->setFilter( osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR );
		tex1->setFilter( osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR );
		camera->setViewport( 0, 0, tex1->getTextureWidth(), tex1->getTextureHeight() );
		camera->attach( buffer1, tex1 );
	}

	if ( tex2 )
	{
		tex2->setFilter( osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR );
		tex2->setFilter( osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR );
		camera->setViewport( 0, 0, tex2->getTextureWidth(), tex2->getTextureHeight() );
		camera->attach( buffer2, tex2 );
	}

	if ( tex3 )
	{
		tex3->setFilter( osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR );
		tex3->setFilter( osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR );
		camera->setViewport( 0, 0, tex3->getTextureWidth(), tex3->getTextureHeight() );
		camera->attach( buffer3, tex3 );
	}

	if ( isAbsolute )
	{
		camera->setReferenceFrame( osg::Transform::ABSOLUTE_RF );
		camera->setProjectionMatrix( osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0) );
		camera->setViewMatrix( osg::Matrix::identity() );
		camera->addChild(createScreenQuad(1.0f, 1.0f) );
	}
	return camera.release();
}

osg::Texture* SSAONode::createTexture2D(int width, int height, osg::Vec3f* data)
{
	osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D; 
	osg::Image* image = new osg::Image;

	image->setImage(width, height, 1, GL_RGB, GL_RGB, GL_FLOAT, (unsigned char*) data, osg::Image::NO_DELETE); 

	texture->setImage( image ); 
	texture->setWrap( osg::Texture::WRAP_S, osg::Texture::REPEAT ); 
	texture->setWrap( osg::Texture::WRAP_T, osg::Texture::REPEAT ); 
	texture->setFilter( osg::Texture::MIN_FILTER, osg::Texture::LINEAR ); 
	texture->setFilter( osg::Texture::MAG_FILTER, osg::Texture::LINEAR ); 
	return texture.release(); 
}

osg::Geode* SSAONode::createScreenQuad( float width, float height, float scale )
{
    osg::Geometry* geom =
            osg::createTexturedQuadGeometry(
                osg::Vec3(),
                osg::Vec3(width,0.0f,0.0f),
                osg::Vec3(0.0f,height,0.0f),
                0.0f,
                0.0f,
                width*scale,
                height*scale );

    osg::ref_ptr<osg::Geode> quad = new osg::Geode;
	quad->addDrawable( geom );

	int values = osg::StateAttribute::OFF|osg::StateAttribute::PROTECTED;
    quad->getOrCreateStateSet()->setAttribute(
                new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK,
                                     osg::PolygonMode::FILL),
                values );
    quad->getOrCreateStateSet()->setMode( GL_LIGHTING, values );
	return quad.release();
}

osg::Camera* SSAONode::createHUDCamera( double left, double right,
                                    double bottom, double top )
{
	osg::ref_ptr<osg::Camera> camera = new osg::Camera;
	camera->setReferenceFrame( osg::Transform::ABSOLUTE_RF );
	camera->setClearMask( GL_DEPTH_BUFFER_BIT );
    camera->setRenderOrder( osg::Camera::POST_RENDER );
	camera->setAllowEventFocus( false );
	camera->setProjectionMatrix( osg::Matrix::ortho2D(left, right, bottom, top) );
	camera->getOrCreateStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
    return camera.release();
}

#include <QFile>
#include <QDir>
bool SSAONode::setShaderStringFromResource(osg::Shader* shader,
        const std::string resourceName)
{
    QFile file( QString::fromStdString(resourceName) );
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug("%s does ont exist", resourceName.c_str());
        return false;
    }

    QString str = file.readAll();
    file.close();
    shader->setShaderSource(str.toStdString());
    return true;
}
