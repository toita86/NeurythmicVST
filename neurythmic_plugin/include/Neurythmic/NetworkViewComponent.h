#pragma once

#include <mutex>
#include <vector>

#include <juce_opengl/juce_opengl.h>

#include "ConfigManager.h"
#include "NetworkController.h"
#include "NodeLabelOverlay.h"

namespace neurythmic {

// OpenGL renderer for the network graph. Faithful port of the legacy
// GraphVis: node circles, parent-child straight edges, curved input edges and
// arrowheads, with the geometry-shader thick lines and fragment-shader dashes.
//
// The scene is built on the message thread (from the ValueTree via
// NetworkController) and handed to the GL thread under a mutex, so renderOpenGL
// never touches the ValueTree or takes a MessageManagerLock.
class NetworkViewComponent : public juce::Component,
                             public juce::OpenGLRenderer {
public:
  explicit NetworkViewComponent(NetworkController& controller);
  ~NetworkViewComponent() override;

  // Rebuild the scene from the controller and request a GL repaint.
  void update();

  void resized() override;
  void visibilityChanged() override;

  void newOpenGLContextCreated() override;
  void renderOpenGL() override;
  void openGLContextClosing() override;

private:
  struct LinePrimitive {
    std::vector<float> vertices;  // interleaved x,y,z,w + r,g,b,a
    float thickness = 1.0f;
    bool dotted = false;
  };
  struct TrianglePrimitive {
    std::vector<float> vertices;
  };
  struct Scene {
    std::vector<LinePrimitive> lines;
    std::vector<TrianglePrimitive> triangles;
  };

  NetworkController& _controller;
  juce::OpenGLContext _gl;
  NodeLabelOverlay _labels;

  std::unique_ptr<juce::OpenGLShaderProgram> _lineShader;
  std::unique_ptr<juce::OpenGLShaderProgram> _flatShader;
  GLuint _vao = 0;
  GLuint _vbo = 0;

  std::mutex _sceneMutex;
  Scene _pendingScene;
  bool _firstRenderLogged = false;

  Scene buildScene() const;
  void setupAttributes(const juce::OpenGLShaderProgram& program);
  static void appendVertex(std::vector<float>& out,
                           juce::Point<float> p,
                           const juce::Colour& c);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NetworkViewComponent)
};

}  // namespace neurythmic
