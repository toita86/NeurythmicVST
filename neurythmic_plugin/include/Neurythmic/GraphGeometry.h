#pragma once

#include <vector>

#include <juce_core/juce_core.h>

#include "ConfigManager.h"

/*
 * Pure, GL-free geometry port of the legacy matsuoka_frontend `GraphVis` mesh
 * generation. Everything here is deterministic math so it can be unit-tested
 * headlessly; the OpenGL layer (NetworkViewComponent) only owns buffers,
 * shaders and draw calls, and feeds these helpers the config-driven numbers.
 */

namespace neurythmic::GraphGeometry {

using Vec2 = juce::Point<float>;

// Legacy `projectToCircumference`: a point at distance `radius + 1` from
// `start`, in the direction of `end`. Returns `start` when start == end.
Vec2 projectToCircumference(Vec2 start, Vec2 end, float radius);

// Legacy `makeCircle`: `nPts + 3` vertices (GL_LINE_STRIP_ADJACENCY) around
// `centre` at `radius`.
std::vector<Vec2> makeCircle(Vec2 centre, float radius, int nPts);

// Legacy `makeLine`: 4 vertices [pre, start, end, post] where pre/post extend
// slightly past the endpoints for the geometry shader's adjacency.
std::vector<Vec2> makeLine(Vec2 start, Vec2 end);

// Legacy `makeArrowHead`: triangle (tip, tip + left, tip + right). The arrow
// half-width is derived from the raw connection weight via
// `arrowHeadWidthMultiplier` clamped to `arrowHeadWidthMin/Max`.
std::vector<Vec2> makeArrowHead(Vec2 tip,
                                float angleDeg,
                                float rawWidth,
                                const ConfigManager& cfg);

// Legacy `makeArc`: `nPts + 2` vertices around `centre` between `startAngle`
// and `endAngle` (radians), with the `arcAdjust` end-trimming applied.
std::vector<Vec2> makeArc(Vec2 centre,
                          float radius,
                          float startAngle,
                          float endAngle,
                          int nPts,
                          const ConfigManager& cfg);

// Legacy `makeInputEdge` geometry: arc centre/radius plus the trimmed draw
// endpoints and angles, ready for makeArc + makeArrowHead.
struct InputEdge {
  Vec2 arcCentre;
  float radius = 0.0f;
  Vec2 drawStart;
  Vec2 drawEnd;
  Vec2 drawArrowEnd;
  float startAngle = 0.0f;
  float endAngle = 0.0f;
  float arrowAngleDeg = 0.0f;
};
InputEdge makeInputEdge(Vec2 start,
                        Vec2 end,
                        float scaling,
                        const ConfigManager& cfg);

// Legacy `getLineWidth`: connection weight -> line thickness.
float getLineWidth(float intensity, const ConfigManager& cfg);

// Legacy `getNodeSpread`: node intensity -> node line thickness.
float getNodeSpread(float intensity, const ConfigManager& cfg);

// Legacy `getColourScale`: connection weight -> colour lerp factor (0..1).
float getColourScale(float width, const ConfigManager& cfg);

// Legacy node brightness: `scale(intensity, nodeIntensityFloor, 1.0)`.
float getNodeBrightness(float intensity, const ConfigManager& cfg);

// Legacy `makeConnectionID`: ((from + 1) * 1000) + to.
int makeConnectionID(int fromId, int toId);

}  // namespace neurythmic::GraphGeometry
