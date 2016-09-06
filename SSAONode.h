// Utility functions for SSAO implementation

#ifndef SSAO_H
#define SSAO_H

#include "osg/Vec3f"
#include <osg/Node>
#include <osg/Switch>
#include <osg/Group>
#include <osg/Texture2D>
#include <osg/PolygonMode>
#include <osg/Camera>
#include <osgViewer/Viewer>
#include <QString>


class  SSAONode : public osg::Group {
public:
    enum DisplayMode {
        SSAO_AOOnly = 2,
        SSAO_ColorAndAO = 1,
        SSAO_ColorOnly = 0
    };
    SSAONode(int m_width,
         int m_height,
         int m_kernelSize = 8, // 4 = good performance, 10 = good quality
         int m_noiseSize = 2, // should be same as blurSize
         int m_blurSize = 2, // 2-4, performance goes to hell above 4
         float radius = 0.65f,
         float power = 3.0f);
    ~SSAONode();

	// Initialize SSAO for given node. New Group will be returned that you should add to your scene
    void Initialize();

    static void buildGraph(osg::ref_ptr<osg::Switch> root,
                           osg::ref_ptr<osg::Group> scene,
                           osg::ref_ptr<SSAONode> ssao)
    {
        root->setNewChildDefaultValue(true);
        root->addChild(scene);

        ssao->addNode(scene);

        root->addChild(ssao);
        root->setValue(root->getChildIndex(ssao), false );
    }

    static void setSSAOEnabled(osg::ref_ptr<osg::Switch> root,
                               osg::ref_ptr<osg::Group> scene,
                               osg::ref_ptr<SSAONode> ssao,
                               bool tf)
    {
        int ssaoIndex = root->getChildIndex(ssao);
        int sceneIndex = root->getChildIndex(scene);

        root->setValue(ssaoIndex, tf);
        root->setValue(sceneIndex, !tf);
    }



	void SetSSAORadius(float radius);
    float GetSSAORadius();
    void SetSSAOPower(float power);
    float GetSSAOPower();

    void SetDisplayMode(SSAONode::DisplayMode mode);
    DisplayMode GetDisplayMode();

    void setHaloRemovalEnabled(bool tf) { m_haloRemovalEnabled = tf; }
    bool IsHaloRemovalEnabled();
    void setAOBlurEnabled(bool tf) { m_blurAOEnabled = tf; }
    bool IsAOBlurEnabled();

    void SetHaloTreshold(float treshold);
    float GetHaloTreshold();

    void updateProjectionMatrix(osg::Matrixd projMatrix);

    void addNode(osg::Node* node);

    void Resize(int m_width, int m_height);

private:

	// Effect settings
    int m_kernelSize; // 4 = good performance, 10 = good quality
    int m_noiseSize; // should be same as blurSize
    int m_blurSize; // make this 2 to 4. Performance goes to hell above 4
    float m_ssaoRadius;
    float m_ssaoPower;
	
    bool m_blurAOEnabled;
    bool m_haloRemovalEnabled;
    float m_haloTreshold;

    int m_width;
    int m_height;

    osg::Vec3f* m_kernelData;
    osg::Vec3f* m_noiseData;

	osg::ref_ptr<osg::Camera> rttCamera;
    osg::ref_ptr<osg::Camera> ssaoCamera;
    osg::ref_ptr<osg::Camera> blurCamera;
	osg::Matrixd projMatrix;

    DisplayMode displayType;

	// Shader uniforms
    osg::Uniform* projMatUniform;
    osg::Uniform* invProjMatrixUniform;
    osg::Uniform* radiusUniform;
    osg::Uniform* powerUniform;
    osg::Uniform* displayTypeUniform;
    osg::Uniform* noiseTextureRcpUniform;
    osg::Uniform* haloRemovalUniform;
    osg::Uniform* haloTresholdUniform;
    osg::Uniform* blurProjMatrixUniform;
    osg::Uniform* blurAOUniform;
    osg::Uniform* sceneSizeUniform;

	void setUniforms();

    // G Buffer
    osg::ref_ptr<osg::Texture2D> colorTex;
    osg::ref_ptr<osg::Texture2D> linearDepthTex;
    osg::ref_ptr<osg::Texture2D> normalTex;
    osg::ref_ptr<osg::Texture2D> secondPassTex;

	// Math utils - possibly replace with calls to some math library
	unsigned int xorshift32();
	float random(float min, float max);
	float lerp(float min, float max, float t);

	// SSAO data generation
    void generateHemisphereSamples(osg::Vec3f* kernel, size_t kernelSize);
    void generateNoise(osg::Vec3f* noise, size_t m_noiseSize);

    osg::StateSet* phongState;

	// OSG Utils
    bool setShaderStringFromResource(osg::Shader* shader,
                                     const std::string resourceName);

    osg::Camera* createRTTCamera(osg::Camera::BufferComponent buffer, osg::Texture* tex, bool isAbsolute);
    osg::Camera* createRTTCameraGBuffer(osg::Camera::BufferComponent buffer1, osg::Texture* tex1, osg::Camera::BufferComponent buffer2, osg::Texture* tex2, osg::Camera::BufferComponent buffer3, osg::Texture* tex3, bool isAbsolute);
    osg::Texture* createTexture2D(int m_width, int m_height, osg::Vec3f* data);
    osg::Geode* createScreenQuad(float m_width, float m_height, float scale = 1.0f);
    osg::Camera* createHUDCamera(double left, double right, double bottom, double top);

    std::string stringFromResource(const char *resourceName);
    void addKernelUniformToStateSet(osg::StateSet *stateset, int kernelLength);
    void createFirstPassCamera();
    void createSecondPassCamera(int kernelLength);
    void createThirdPassCamera();
    void setProjectionMatrixUniforms();
};

#endif // SSAO_H
