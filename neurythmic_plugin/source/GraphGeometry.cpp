#include "../include/Neurythmic/GraphGeometry.h"

#include <algorithm>
#include <cmath>

namespace neurythmic::GraphGeometry {

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;

Vec2 rotate(Vec2 pt, float angleRad) {
  const float s = std::sin(angleRad);
  const float c = std::cos(angleRad);
  return {pt.getX() * c - pt.getY() * s, pt.getX() * s + pt.getY() * c};
}

Vec2 rotateAround(Vec2 pt, float angleRad, Vec2 pivot) {
  return pivot + rotate(pt - pivot, angleRad);
}

}  // namespace

Vec2 projectToCircumference(Vec2 start, Vec2 end, float radius) {
  Vec2 d = end - start;
  const float len = std::hypot(d.getX(), d.getY());
  if (len < 1e-9f)
    return start;
  return start + d * ((radius + 1.0f) / len);
}

std::vector<Vec2> makeCircle(Vec2 centre, float radius, int nPts) {
  std::vector<Vec2> pts;
  pts.reserve(static_cast<size_t>(nPts + 3));
  for (int i = 0; i < nPts + 3; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(nPts);
    const float a = kTwoPi * t;
    pts.emplace_back(centre.getX() + radius * std::cos(a),
                     centre.getY() + radius * std::sin(a));
  }
  return pts;
}

std::vector<Vec2> makeLine(Vec2 start, Vec2 end) {
  Vec2 d = end - start;
  Vec2 pre = start - d * 0.00001f;
  Vec2 post = end + d * 0.00001f;
  return {pre, start, end, post};
}

std::vector<Vec2> makeArrowHead(Vec2 tip,
                                float angleDeg,
                                float rawWidth,
                                const ConfigManager& cfg) {
  float drawWidth = rawWidth * cfg.arrowHeadWidthMultiplier;
  drawWidth =
      std::clamp(drawWidth, cfg.arrowHeadWidthMin, cfg.arrowHeadWidthMax);

  const float angleRad = angleDeg * kPi / 180.0f;
  Vec2 left(-drawWidth, cfg.arrowHeadSize);
  Vec2 right(drawWidth, cfg.arrowHeadSize);
  left = rotate(left, angleRad);
  right = rotate(right, angleRad);
  return {tip, tip + left, tip + right};
}

std::vector<Vec2> makeArc(Vec2 centre,
                          float radius,
                          float startAngle,
                          float endAngle,
                          int nPts,
                          const ConfigManager& cfg) {
  float adj;
  if (radius < cfg.minArcAdjustRad) {
    adj = 1.0f;
  } else if (radius > cfg.maxArcAdjustRad) {
    adj = 0.0f;
  } else {
    adj = 1.0f - (radius - cfg.minArcAdjustRad) /
                     (cfg.maxArcAdjustRad - cfg.minArcAdjustRad);
  }
  startAngle += cfg.arcAdjust * adj;
  endAngle -= cfg.arcAdjust * adj;
  if (startAngle >= endAngle)
    endAngle += kTwoPi;
  if (radius < 0.0f)
    radius = 0.0f;
  if (nPts < 3)
    nPts = 3;

  const float angleStep = (endAngle - startAngle) / (nPts - 1);

  std::vector<Vec2> pts;
  pts.reserve(static_cast<size_t>(nPts + 2));
  float angle = startAngle - angleStep;
  for (int i = 0; i < nPts + 2; ++i) {
    pts.emplace_back(centre.getX() + radius * std::cos(angle),
                     centre.getY() + radius * std::sin(angle));
    angle += angleStep;
  }
  return pts;
}

InputEdge makeInputEdge(Vec2 start,
                        Vec2 end,
                        float scaling,
                        const ConfigManager& cfg) {
  const float strghtDist =
      std::hypot(end.getX() - start.getX(), end.getY() - start.getY());
  const float curveAmt =
      cfg.defaultCurveAmount * (strghtDist * strghtDist * 0.2f / 1200.0f);

  Vec2 perpendicular(-(end.getY() - start.getY()), end.getX() - start.getX());
  // Normalise: the legacy code passed the full-length perpendicular into the
  // arc-centre offset, which made the centre inconsistent with the radius
  // formula (radius assumes a unit perpendicular) and produced looping arcs.
  const float perpLen = std::hypot(perpendicular.getX(), perpendicular.getY());
  if (perpLen > 1e-9f)
    perpendicular = perpendicular * (1.0f / perpLen);
  Vec2 midpoint((start.getX() + end.getX()) / 2.0f,
                (start.getY() + end.getY()) / 2.0f);
  Vec2 arcCentre = perpendicular * (curveAmt * strghtDist) + midpoint;

  const float radius =
      std::sqrt((curveAmt * curveAmt + 0.25f) * strghtDist * strghtDist);
  const float nodeLineGap = cfg.node0HaloSize * cfg.node0HaloSize;
  const float distFromCentre = cfg.nodeRadius * scaling * nodeLineGap;

  Vec2 drawStart =
      rotateAround(start, std::atan(distFromCentre / radius), arcCentre);
  Vec2 drawEnd = rotateAround(
      end, -std::atan((distFromCentre + cfg.arrowHeadSize) / radius),
      arcCentre);
  Vec2 drawArrowEnd =
      rotateAround(end, -std::atan(distFromCentre / radius), arcCentre);

  const float startAngle = std::atan2(drawStart.getY() - arcCentre.getY(),
                                      drawStart.getX() - arcCentre.getX());
  const float endAngle = std::atan2(drawEnd.getY() - arcCentre.getY(),
                                    drawEnd.getX() - arcCentre.getX());
  const float arrowAngleDeg = (kPi + endAngle) * 180.0f / kPi;

  return {arcCentre,    radius,     drawStart, drawEnd,
          drawArrowEnd, startAngle, endAngle,  arrowAngleDeg};
}

float getLineWidth(float intensity, const ConfigManager& cfg) {
  intensity = std::clamp(intensity, cfg.minLineWeight, cfg.maxLineWeight);
  const float normal =
      (intensity - cfg.minLineWeight) / (cfg.maxLineWeight - cfg.minLineWeight);
  return (normal * normal) * (cfg.maxLineThickness - cfg.minLineThickness) +
         cfg.minLineThickness;
}

float getNodeSpread(float intensity, const ConfigManager& cfg) {
  intensity = std::fabs(intensity);
  return std::pow(intensity, cfg.nodeFlashExponent) * cfg.nodeFlashSpread +
         cfg.nodeLineThickness;
}

float getColourScale(float width, const ConfigManager& cfg) {
  const float widthCutoff = cfg.connectionColourStartsAtLevel *
                                (cfg.maxLineThickness - cfg.minLineThickness) +
                            cfg.minLineThickness;
  if (width < widthCutoff)
    return width / widthCutoff;
  return 1.0f;
}

float getNodeBrightness(float intensity, const ConfigManager& cfg) {
  intensity = std::clamp(intensity, 0.0f, 1.0f);
  return cfg.nodeIntensityFloor + intensity * (1.0f - cfg.nodeIntensityFloor);
}

int makeConnectionID(int fromId, int toId) {
  return ((fromId + 1) * 1000) + toId;
}

ConnectionGeometry makeConnectionGeometry(Vec2 from,
                                          Vec2 to,
                                          bool isParent,
                                          float scaling,
                                          const ConfigManager& cfg) {
  ConnectionGeometry geo;
  geo.isParent = isParent;
  if (isParent) {
    const float distFromCentre = cfg.nodeRadius * cfg.node0HaloSize * scaling;
    geo.start = projectToCircumference(from, to, distFromCentre);
    geo.end =
        projectToCircumference(to, from, distFromCentre + cfg.arrowHeadSize);
    geo.arrowTip = projectToCircumference(to, from, distFromCentre);
    const Vec2 dv = geo.start - geo.arrowTip;
    geo.arrowAngleDeg = std::atan2(-dv.getX(), dv.getY()) * 180.0f / kPi;
  } else {
    const InputEdge ie = makeInputEdge(from, to, scaling, cfg);
    geo.start = ie.drawStart;
    geo.end = ie.drawEnd;
    geo.arcCentre = ie.arcCentre;
    geo.radius = ie.radius;
    geo.startAngle = ie.startAngle;
    geo.endAngle = ie.endAngle;
    geo.arrowTip = ie.drawArrowEnd;
    geo.arrowAngleDeg = ie.arrowAngleDeg;
  }
  return geo;
}

bool isStraightConnectionAtPoint(Vec2 point,
                                 Vec2 start,
                                 Vec2 end,
                                 float clickableWidth) {
  const Vec2 line = end - start;
  const float len = std::hypot(line.getX(), line.getY());
  if (len < 1e-9f)
    return point.getDistanceFrom(start) <= clickableWidth / 2.0f;

  // Perpendicular offset of magnitude clickableWidth/2 each side (matches the
  // legacy `lineVector.getScaled(clickableWidth/2)` + 90° rotation).
  const Vec2 perp =
      Vec2(-line.getY(), line.getX()) * (clickableWidth / (2.0f * len));

  const Vec2 v1 = start + perp;  // startL
  const Vec2 v2 = end + perp;    // endL
  const Vec2 v3 = end - perp;    // endR
  const Vec2 v4 = start - perp;  // startR

  // Point-in-quadrilateral raycast (four edges).
  const float px = point.getX();
  const float py = point.getY();
  const auto crosses = [&](const Vec2& a, const Vec2& b) {
    return (a.getY() >= py) != (b.getY() >= py) &&
           px <=
               (b.getX() - a.getX()) * (py - a.getY()) / (b.getY() - a.getY()) +
                   a.getX();
  };

  bool inside = false;
  if (crosses(v1, v2))
    inside = !inside;
  if (crosses(v2, v3))
    inside = !inside;
  if (crosses(v3, v4))
    inside = !inside;
  if (crosses(v4, v1))
    inside = !inside;
  return inside;
}

bool isCurvedConnectionAtPoint(Vec2 point,
                               Vec2 arcCentre,
                               float radius,
                               float startAngle,
                               float endAngle,
                               float clickableWidth) {
  const float dist = point.getDistanceFrom(arcCentre);
  const float maxDist = radius + clickableWidth;
  if (dist > maxDist)
    return false;
  const float minDist = radius - clickableWidth;
  if (dist < minDist)
    return false;

  const float anglePoint = std::atan2(point.getY() - arcCentre.getY(),
                                      point.getX() - arcCentre.getX());
  // Legacy `is_angle_between2`.
  const double d = anglePoint - startAngle;
  const double s = std::remainder(endAngle - startAngle - kPi, kTwoPi) + kPi;
  return std::remainder(d - kPi, kTwoPi) + kPi <= s;
}

}  // namespace neurythmic::GraphGeometry
