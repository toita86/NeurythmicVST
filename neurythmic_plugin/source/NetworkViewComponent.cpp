#include "../include/Neurythmic/NetworkViewComponent.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

#include "../include/Neurythmic/GraphGeometry.h"
#include "NeurythmicPluginAssets.h"
#include "../include/Neurythmic/NetworkState.h"

namespace neurythmic {

using namespace juce::gl;

namespace {

constexpr float kPiFloat = 3.14159265358979323846f;

juce::String shaderSource(const char* data, int size) {
  return juce::String(data, static_cast<size_t>(size));
}

// Column-major orthographic matrix mapping (0..w, 0..h) -> NDC, with Y flipped
// because JUCE/legacy coordinates grow downwards.
std::array<float, 16> orthoMatrix(float w, float h) {
  return {2.0f / w, 0.0f, 0.0f, 0.0f, 0.0f,  -2.0f / h, 0.0f, 0.0f,
          0.0f,     0.0f, 1.0f, 0.0f, -1.0f, 1.0f,      0.0f, 1.0f};
}

juce::Colour scaleRGB(juce::Colour c, float m) {
  return juce::Colour::fromFloatRGBA(c.getFloatRed() * m, c.getFloatGreen() * m,
                                     c.getFloatBlue() * m, 1.0f);
}

}  // namespace

NetworkViewComponent::NetworkViewComponent(NetworkController& controller)
    : _controller(controller), _labels(controller) {
  addAndMakeVisible(_labels);

  _gl.setRenderer(this);
  _gl.setContinuousRepainting(true);
  _gl.setOpenGLVersionRequired(juce::OpenGLContext::openGL3_2);
  _gl.attachTo(*this);
}

NetworkViewComponent::~NetworkViewComponent() {
  _gl.detach();
  _gl.setRenderer(nullptr);
}

void NetworkViewComponent::update() {
  Scene scene = buildScene();
  {
    std::lock_guard<std::mutex> lock(_sceneMutex);
    _pendingScene = std::move(scene);
  }
  _labels.repaint();
  _gl.triggerRepaint();
}

void NetworkViewComponent::resized() {
  _labels.setBounds(getLocalBounds());
  update();
}

void NetworkViewComponent::visibilityChanged() {
  if (isShowing())
    update();
}

// ---------------------------------------------------------------------------
// Scene building (message thread)

NetworkViewComponent::Scene NetworkViewComponent::buildScene() const {
  const auto& cfg = ConfigManager::get();
  Scene scene;

  const float w = static_cast<float>(getWidth());
  const float h = static_cast<float>(getHeight());
  const float minDim = std::min(w, h);
  const float scaling = std::min(1.0f, minDim / 1000.0f);

  auto toPixel = [&](juce::Point<float> n) {
    return juce::Point<float>(n.getX() * minDim, n.getY() * minDim);
  };
  auto appendLine = [&](const std::vector<GraphGeometry::Vec2>& verts,
                        juce::Colour colour, float thickness, bool dotted) {
    LinePrimitive prim;
    prim.vertices.reserve(verts.size() * 8);
    for (const auto& v : verts)
      appendVertex(prim.vertices, v, colour);
    prim.thickness = thickness;
    prim.dotted = dotted;
    scene.lines.push_back(std::move(prim));
  };
  auto appendTriangle = [&](const std::vector<GraphGeometry::Vec2>& verts,
                            juce::Colour colour) {
    TrianglePrimitive prim;
    prim.vertices.reserve(verts.size() * 8);
    for (const auto& v : verts)
      appendVertex(prim.vertices, v, colour);
    scene.triangles.push_back(std::move(prim));
  };

  std::map<int, juce::Point<float>> pos;
  std::map<int, float> intensity;
  for (int id : _controller.getNodeIds()) {
    pos[id] = toPixel(_controller.getNodePosition(id));
    intensity[id] = static_cast<float>(_controller.getNodeIntensity(id));
  }

  // Nodes (circles + root halo).
  for (int id : _controller.getNodeIds()) {
    const float bright = GraphGeometry::getNodeBrightness(intensity[id], cfg);
    const juce::Colour colour = scaleRGB(cfg.getNodeColour(id), bright);
    const float thickness = GraphGeometry::getNodeSpread(intensity[id], cfg);

    if (id == NetworkState::kRootNodeId) {
      const float haloRadius = cfg.nodeRadius * cfg.node0HaloSize * scaling;
      appendLine(
          GraphGeometry::makeCircle(pos[id], haloRadius, cfg.pointsInCircle),
          colour, thickness, false);
    }
    appendLine(GraphGeometry::makeCircle(pos[id], cfg.nodeRadius * scaling,
                                         cfg.pointsInCircle),
               colour, thickness, false);
  }

  // Connections (parent-child straight, input curved) + arrowheads.
  for (const auto& conn : _controller.getConnections()) {
    const juce::Point<float> from = pos[conn.sourceId];
    const juce::Point<float> to = pos[conn.targetId];
    const float weight = static_cast<float>(conn.weight);
    const bool dotted = weight < 0.0001f;
    const float colourScale = GraphGeometry::getColourScale(weight, cfg);
    juce::Colour colour =
        juce::Colours::lightgrey.interpolatedWith(cfg.connColour, colourScale);
    if (dotted)
      colour = scaleRGB(colour, cfg.inactiveBrightnessMult);
    const float thickness = GraphGeometry::getLineWidth(weight, cfg);

    if (conn.isParentEdge) {
      const float distFromCentre = cfg.nodeRadius * cfg.node0HaloSize * scaling;
      const juce::Point<float> start =
          GraphGeometry::projectToCircumference(from, to, distFromCentre);
      const juce::Point<float> end = GraphGeometry::projectToCircumference(
          to, from, distFromCentre + cfg.arrowHeadSize);
      const juce::Point<float> arrowEnd =
          GraphGeometry::projectToCircumference(to, from, distFromCentre);

      appendLine(GraphGeometry::makeLine(start, end), colour, thickness,
                 dotted);

      const juce::Point<float> dv = start - arrowEnd;
      const float angleDeg =
          std::atan2(-dv.getX(), dv.getY()) * 180.0f / kPiFloat;
      appendTriangle(
          GraphGeometry::makeArrowHead(arrowEnd, angleDeg, weight, cfg),
          colour);
    } else {
      const auto ie = GraphGeometry::makeInputEdge(from, to, scaling, cfg);
      appendLine(GraphGeometry::makeArc(ie.arcCentre, ie.radius, ie.startAngle,
                                        ie.endAngle, cfg.pointsInArc, cfg),
                 colour, thickness, dotted);
      appendTriangle(GraphGeometry::makeArrowHead(
                         ie.drawArrowEnd, ie.arrowAngleDeg, weight, cfg),
                     colour);
    }
  }

  return scene;
}

void NetworkViewComponent::appendVertex(std::vector<float>& out,
                                        juce::Point<float> p,
                                        const juce::Colour& c) {
  out.push_back(p.getX());
  out.push_back(p.getY());
  out.push_back(0.0f);
  out.push_back(1.0f);
  out.push_back(c.getFloatRed());
  out.push_back(c.getFloatGreen());
  out.push_back(c.getFloatBlue());
  out.push_back(c.getFloatAlpha());
}

// ---------------------------------------------------------------------------
// OpenGL lifecycle

void NetworkViewComponent::newOpenGLContextCreated() {
  _lineShader = std::make_unique<juce::OpenGLShaderProgram>(_gl);
  _lineShader->addVertexShader(
      shaderSource(neurythmic::assets::dotted_vert_glsl,
                   neurythmic::assets::dotted_vert_glslSize));
  _lineShader->addFragmentShader(
      shaderSource(neurythmic::assets::dotted_frag_glsl,
                   neurythmic::assets::dotted_frag_glslSize));
  _lineShader->addShader(shaderSource(neurythmic::assets::dotted_geom_glsl,
                                      neurythmic::assets::dotted_geom_glslSize),
                         GL_GEOMETRY_SHADER);
  if (!_lineShader->link())
    std::cerr << "Neurythmic: line shader link failed: "
              << _lineShader->getLastError() << std::endl;

  _flatShader = std::make_unique<juce::OpenGLShaderProgram>(_gl);
  _flatShader->addVertexShader(
      shaderSource(neurythmic::assets::flat_vert_glsl,
                   neurythmic::assets::flat_vert_glslSize));
  _flatShader->addFragmentShader(
      shaderSource(neurythmic::assets::flat_frag_glsl,
                   neurythmic::assets::flat_frag_glslSize));
  if (!_flatShader->link())
    std::cerr << "Neurythmic: flat shader link failed: "
              << _flatShader->getLastError() << std::endl;

  glGenVertexArrays(1, &_vao);
  glGenBuffers(1, &_vbo);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void NetworkViewComponent::setupAttributes(
    const juce::OpenGLShaderProgram& program) {
  const GLint posLoc = glGetAttribLocation(program.getProgramID(), "in_pos");
  const GLint colLoc =
      glGetAttribLocation(program.getProgramID(), "vertex_color");
  if (posLoc >= 0) {
    glEnableVertexAttribArray(static_cast<GLuint>(posLoc));
    glVertexAttribPointer(static_cast<GLuint>(posLoc), 4, GL_FLOAT, GL_FALSE,
                          static_cast<GLsizei>(8 * sizeof(float)), nullptr);
  }
  if (colLoc >= 0) {
    glEnableVertexAttribArray(static_cast<GLuint>(colLoc));
    glVertexAttribPointer(static_cast<GLuint>(colLoc), 4, GL_FLOAT, GL_FALSE,
                          static_cast<GLsizei>(8 * sizeof(float)),
                          reinterpret_cast<void*>(4 * sizeof(float)));
  }
}

void NetworkViewComponent::renderOpenGL() {
  const float w = static_cast<float>(getWidth());
  const float h = static_cast<float>(getHeight());

  glViewport(0, 0, static_cast<GLsizei>(w), static_cast<GLsizei>(h));
  glClearColor(1.0f, 0.0f, 0.0f, 1.0f);  // TEMP diagnostic: bright red
  glClear(GL_COLOR_BUFFER_BIT);

  if (_lineShader == nullptr || _flatShader == nullptr)
    return;

  Scene scene;
  {
    std::lock_guard<std::mutex> lock(_sceneMutex);
    scene = std::move(_pendingScene);
  }

  const bool firstRender = !_firstRenderLogged;
  _firstRenderLogged = true;

  if (firstRender) {
    std::cerr << "Neurythmic: first render " << getWidth() << "x" << getHeight()
              << " lines=" << scene.lines.size()
              << " triangles=" << scene.triangles.size() << std::endl;
    if (_lineShader != nullptr) {
      const auto prog = _lineShader->getProgramID();
      std::cerr << "  line prog=" << prog << " mvp="
                << glGetUniformLocation(prog, "modelViewProjectionMatrix")
                << " thickness=" << glGetUniformLocation(prog, "thickness")
                << " dotted=" << glGetUniformLocation(prog, "dotted")
                << " in_pos=" << glGetAttribLocation(prog, "in_pos")
                << " vertex_color=" << glGetAttribLocation(prog, "vertex_color")
                << " glver="
                << reinterpret_cast<const char*>(glGetString(GL_VERSION))
                << std::endl;
    }
    if (_flatShader != nullptr) {
      const auto prog = _flatShader->getProgramID();
      std::cerr << "  flat prog=" << prog << " mvp="
                << glGetUniformLocation(prog, "modelViewProjectionMatrix")
                << " in_pos=" << glGetAttribLocation(prog, "in_pos")
                << " vertex_color=" << glGetAttribLocation(prog, "vertex_color")
                << std::endl;
    }
    if (!scene.lines.empty() && !scene.lines[0].vertices.empty()) {
      const auto& v = scene.lines[0].vertices;
      std::cerr << "  first line: verts=" << (v.size() / 8) << " v0=(x=" << v[0]
                << ",y=" << v[1] << ",z=" << v[2] << ",w=" << v[3]
                << " r=" << v[4] << " g=" << v[5] << " b=" << v[6]
                << " a=" << v[7] << ")" << std::endl;
    }
  }

  const auto mvp = orthoMatrix(w, h);

  glBindVertexArray(_vao);
  glBindBuffer(GL_ARRAY_BUFFER, _vbo);

  _lineShader->use();
  setupAttributes(*_lineShader);
  _lineShader->setUniformMat4("modelViewProjectionMatrix", mvp.data(), 1,
                              GL_FALSE);
  for (const auto& prim : scene.lines) {
    _lineShader->setUniform("thickness", std::max(1.0f, prim.thickness));
    _lineShader->setUniform("dotted", prim.dotted ? 1 : 0);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(prim.vertices.size() * sizeof(float)),
                 prim.vertices.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_LINE_STRIP_ADJACENCY, 0,
                 static_cast<GLsizei>(prim.vertices.size() / 8));
  }

  _flatShader->use();
  setupAttributes(*_flatShader);
  _flatShader->setUniformMat4("modelViewProjectionMatrix", mvp.data(), 1,
                              GL_FALSE);
  for (const auto& prim : scene.triangles) {
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(prim.vertices.size() * sizeof(float)),
                 prim.vertices.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0,
                 static_cast<GLsizei>(prim.vertices.size() / 8));
  }

  if (firstRender) {
    const int W = static_cast<int>(w), H = static_cast<int>(h);
    std::vector<unsigned char> px(static_cast<size_t>(W * H * 4));
    glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    int nonBg = 0;
    int maxDev = 0, maxX = -1, maxY = -1, maxR = 0, maxG = 0, maxB = 0;
    for (int y = 0; y < H; ++y)
      for (int x = 0; x < W; ++x) {
        const int i = (y * W + x) * 4;
        const int r = px[i], g = px[i + 1], b = px[i + 2];
        const int dev = std::abs(r - 25) + std::abs(g - 25) + std::abs(b - 30);
        if (dev > 30) {
          ++nonBg;
          if (dev > maxDev) {
            maxDev = dev;
            maxX = x;
            maxY = y;
            maxR = r;
            maxG = g;
            maxB = b;
          }
        }
      }
    std::cerr << "  readback: nonBgPixels=" << nonBg << " maxDev=" << maxDev
              << " at (" << maxX << "," << maxY << ") rgb=(" << maxR << ","
              << maxG << "," << maxB << ")" << std::endl;
  }

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  {
    const GLenum err = glGetError();
    if (err != GL_NO_ERROR)
      std::cerr << "Neurythmic: GL error after render: 0x" << std::hex << err
                << std::dec << std::endl;
  }
}

void NetworkViewComponent::openGLContextClosing() {
  if (_vbo != 0)
    glDeleteBuffers(1, &_vbo);
  if (_vao != 0)
    glDeleteVertexArrays(1, &_vao);
  _vbo = 0;
  _vao = 0;
  _lineShader.reset();
  _flatShader.reset();
}

}  // namespace neurythmic
