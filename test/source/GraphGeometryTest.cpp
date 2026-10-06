#include <gtest/gtest.h>
#include <juce_core/juce_core.h>

#include "Neurythmic/GraphGeometry.h"

namespace {

using neurythmic::GraphGeometry::Vec2;

constexpr float kPi = 3.14159265358979323846f;

const neurythmic::ConfigManager& cfg() {
  return neurythmic::ConfigManager::get();
}

float dist(Vec2 a, Vec2 b) { return a.getDistanceFrom(b); }

}  // namespace

// projectToCircumference -----------------------------------------------------

TEST(GraphGeometry, ProjectToCircumferencePlacesPointAtRadiusPlusOne) {
  Vec2 start(0.0f, 0.0f);
  Vec2 end(1.0f, 0.0f);
  Vec2 p = neurythmic::GraphGeometry::projectToCircumference(start, end, 4.0f);
  EXPECT_NEAR(p.getX(), 5.0f, 1e-4f);
  EXPECT_NEAR(p.getY(), 0.0f, 1e-4f);
}

TEST(GraphGeometry, ProjectToCircumferenceIsDirectional) {
  Vec2 start(0.0f, 0.0f);
  Vec2 end(0.0f, -1.0f);
  Vec2 p = neurythmic::GraphGeometry::projectToCircumference(start, end, 4.0f);
  EXPECT_NEAR(p.getX(), 0.0f, 1e-4f);
  EXPECT_NEAR(p.getY(), -5.0f, 1e-4f);
}

TEST(GraphGeometry, ProjectToCircumferenceDegenerateReturnsStart) {
  Vec2 start(2.0f, 3.0f);
  Vec2 p = neurythmic::GraphGeometry::projectToCircumference(start, start, 4.0f);
  EXPECT_NEAR(p.getX(), 2.0f, 1e-4f);
  EXPECT_NEAR(p.getY(), 3.0f, 1e-4f);
}

// makeCircle -----------------------------------------------------------------

TEST(GraphGeometry, MakeCircleProducesNPlusThreeVertices) {
  auto pts = neurythmic::GraphGeometry::makeCircle(Vec2(0, 0), 10.0f, 30);
  EXPECT_EQ(pts.size(), 33u);
  for (const auto& p : pts)
    EXPECT_NEAR(dist(p, Vec2(0, 0)), 10.0f, 1e-3f);
}

TEST(GraphGeometry, MakeCircleFirstVertexAtAngleZero) {
  auto pts = neurythmic::GraphGeometry::makeCircle(Vec2(5, 5), 10.0f, 30);
  EXPECT_NEAR(pts[0].getX(), 15.0f, 1e-3f);
  EXPECT_NEAR(pts[0].getY(), 5.0f, 1e-3f);
}

// makeLine -------------------------------------------------------------------

TEST(GraphGeometry, MakeLineProducesFourAdjacencyVertices) {
  auto pts = neurythmic::GraphGeometry::makeLine(Vec2(0, 0), Vec2(10, 0));
  ASSERT_EQ(pts.size(), 4u);
  // [pre, start, end, post]; pre/post lie just outside the [start, end] span.
  EXPECT_LT(pts[0].getX(), 0.0f);    // pre before start
  EXPECT_NEAR(pts[1].getX(), 0.0f, 1e-4f);   // start
  EXPECT_NEAR(pts[2].getX(), 10.0f, 1e-4f);  // end
  EXPECT_GT(pts[3].getX(), 10.0f);   // post past end
}

// makeArrowHead --------------------------------------------------------------

TEST(GraphGeometry, MakeArrowHeadClampsWidthAndBuildsTriangle) {
  // rawWidth 2 -> 2*2 = 4, clamped up to arrowHeadWidthMin (5).
  auto pts = neurythmic::GraphGeometry::makeArrowHead(Vec2(0, 0), 0.0f, 2.0f,
                                                      cfg());
  ASSERT_EQ(pts.size(), 3u);
  EXPECT_NEAR(pts[0].getX(), 0.0f, 1e-4f);   // tip
  EXPECT_NEAR(pts[0].getY(), 0.0f, 1e-4f);
  // angle 0 -> offsets (-5, 12) and (5, 12)
  EXPECT_NEAR(pts[1].getX(), -5.0f, 1e-3f);
  EXPECT_NEAR(pts[1].getY(), 12.0f, 1e-3f);
  EXPECT_NEAR(pts[2].getX(), 5.0f, 1e-3f);
  EXPECT_NEAR(pts[2].getY(), 12.0f, 1e-3f);
}

TEST(GraphGeometry, MakeArrowHeadClampsToMaxWidth) {
  auto pts = neurythmic::GraphGeometry::makeArrowHead(Vec2(0, 0), 0.0f, 100.0f,
                                                      cfg());
  // 100*2 = 200 -> clamped to arrowHeadWidthMax (13).
  EXPECT_NEAR(pts[1].getX(), -13.0f, 1e-3f);
  EXPECT_NEAR(pts[2].getX(), 13.0f, 1e-3f);
}

// makeArc --------------------------------------------------------------------

TEST(GraphGeometry, MakeArcProducesNPlusTwoVertices) {
  auto pts = neurythmic::GraphGeometry::makeArc(Vec2(0, 0), 100.0f, 0.0f,
                                                kPi / 2.0f, 30, cfg());
  EXPECT_EQ(pts.size(), 32u);
}

TEST(GraphGeometry, MakeArcEndpointsMatchAngles) {
  // radius 100 > maxArcAdjustRad (50) so no end-trimming is applied.
  auto pts = neurythmic::GraphGeometry::makeArc(Vec2(0, 0), 100.0f, 0.0f,
                                                kPi / 2.0f, 30, cfg());
  // vertex[1] is the first "real" point (startAngle).
  EXPECT_NEAR(pts[1].getX(), 100.0f, 1e-2f);
  EXPECT_NEAR(pts[1].getY(), 0.0f, 1e-2f);
  // vertex[nPts] (index 30) is at endAngle.
  EXPECT_NEAR(pts[30].getX(), 0.0f, 1e-2f);
  EXPECT_NEAR(pts[30].getY(), 100.0f, 1e-2f);
}

// getLineWidth ---------------------------------------------------------------

TEST(GraphGeometry, GetLineWidthMapsBoundsToThickness) {
  EXPECT_NEAR(neurythmic::GraphGeometry::getLineWidth(cfg().minLineWeight, cfg()),
              cfg().minLineThickness, 1e-4f);
  EXPECT_NEAR(neurythmic::GraphGeometry::getLineWidth(cfg().maxLineWeight, cfg()),
              cfg().maxLineThickness, 1e-4f);
}

TEST(GraphGeometry, GetLineWidthIsMonotonicAndBounded) {
  float lo = neurythmic::GraphGeometry::getLineWidth(0.1f, cfg());
  float hi = neurythmic::GraphGeometry::getLineWidth(7.0f, cfg());
  EXPECT_LT(lo, hi);
  EXPECT_GE(lo, cfg().minLineThickness);
  EXPECT_LE(hi, cfg().maxLineThickness);
}

// getNodeSpread --------------------------------------------------------------

TEST(GraphGeometry, GetNodeSpreadAtZeroIsBaseThickness) {
  EXPECT_NEAR(neurythmic::GraphGeometry::getNodeSpread(0.0f, cfg()),
              cfg().nodeLineThickness, 1e-4f);
}

TEST(GraphGeometry, GetNodeSpreadGrowsWithIntensity) {
  EXPECT_GT(neurythmic::GraphGeometry::getNodeSpread(1.0f, cfg()),
            neurythmic::GraphGeometry::getNodeSpread(0.0f, cfg()));
}

// getColourScale -------------------------------------------------------------

TEST(GraphGeometry, GetColourScaleBelowCutoffIsProportional) {
  float cutoff = cfg().connectionColourStartsAtLevel *
                     (cfg().maxLineThickness - cfg().minLineThickness) +
                 cfg().minLineThickness;
  EXPECT_NEAR(neurythmic::GraphGeometry::getColourScale(cutoff / 2.0f, cfg()),
              0.5f, 1e-4f);
}

TEST(GraphGeometry, GetColourScaleSaturatesAtOne) {
  EXPECT_NEAR(neurythmic::GraphGeometry::getColourScale(100.0f, cfg()), 1.0f,
              1e-4f);
}

// getNodeBrightness ----------------------------------------------------------

TEST(GraphGeometry, GetNodeBrightnessMapsIntensityFloorToOne) {
  EXPECT_NEAR(neurythmic::GraphGeometry::getNodeBrightness(0.0f, cfg()),
              cfg().nodeIntensityFloor, 1e-4f);
  EXPECT_NEAR(neurythmic::GraphGeometry::getNodeBrightness(1.0f, cfg()), 1.0f,
              1e-4f);
}

// makeConnectionID -----------------------------------------------------------

TEST(GraphGeometry, MakeConnectionIDRoundTrips) {
  int id = neurythmic::GraphGeometry::makeConnectionID(1, 2);
  EXPECT_EQ(id, 2002);
  EXPECT_EQ(id % 1000, 2);        // toId
  EXPECT_EQ(id / 1000 - 1, 1);    // fromId
}

// makeInputEdge --------------------------------------------------------------

TEST(GraphGeometry, MakeInputEdgeArcCentrePerpendicularAndRadius) {
  auto ie = neurythmic::GraphGeometry::makeInputEdge(Vec2(0, 0), Vec2(1, 0),
                                                     1.0f, cfg());
  float strghtDist = 1.0f;
  float curveAmt = cfg().defaultCurveAmount *
                   (strghtDist * strghtDist * 0.2f / 1200.0f);
  EXPECT_NEAR(ie.arcCentre.getX(), 0.5f, 1e-4f);       // midpoint x
  EXPECT_NEAR(ie.arcCentre.getY(), curveAmt, 1e-4f);   // perpendicular offset
  EXPECT_NEAR(ie.radius, 0.5f, 1e-3f);
}

TEST(GraphGeometry, MakeInputEdgeDrawPointsLieOnArc) {
  auto ie = neurythmic::GraphGeometry::makeInputEdge(Vec2(0, 0), Vec2(1, 0),
                                                     1.0f, cfg());
  EXPECT_NEAR(dist(ie.drawStart, ie.arcCentre), ie.radius, 1e-2f);
  EXPECT_NEAR(dist(ie.drawEnd, ie.arcCentre), ie.radius, 1e-2f);
  EXPECT_NEAR(dist(ie.drawArrowEnd, ie.arcCentre), ie.radius, 1e-2f);
}

TEST(GraphGeometry, MakeInputEdgeTrimsTowardEachNode) {
  auto ie = neurythmic::GraphGeometry::makeInputEdge(Vec2(0, 0), Vec2(1, 0),
                                                     1.0f, cfg());
  // drawStart sits on the source-node side, drawEnd on the target-node side.
  EXPECT_LT(dist(ie.drawStart, Vec2(0, 0)), dist(ie.drawEnd, Vec2(0, 0)));
  EXPECT_LT(dist(ie.drawEnd, Vec2(1, 0)), dist(ie.drawStart, Vec2(1, 0)));
}

TEST(GraphGeometry, MakeInputEdgePointsLieOnArcForLongEdges) {
  // Regression: for a non-unit chord the arc centre must still be at `radius`
  // from the draw points (the legacy code used a non-normalised perpendicular,
  // which pushed the centre far away and looped the arc).
  auto ie = neurythmic::GraphGeometry::makeInputEdge(Vec2(0, 0), Vec2(200, 0),
                                                     0.5f, cfg());
  EXPECT_NEAR(dist(ie.drawStart, ie.arcCentre), ie.radius, 1e-2f);
  EXPECT_NEAR(dist(ie.drawEnd, ie.arcCentre), ie.radius, 1e-2f);
  EXPECT_NEAR(dist(ie.drawArrowEnd, ie.arcCentre), ie.radius, 1e-2f);
}

// makeConnectionGeometry -----------------------------------------------------

TEST(GraphGeometry, MakeConnectionGeometryParentMatchesProjection) {
  const auto& c = cfg();
  const Vec2 from(0.0f, 0.0f);
  const Vec2 to(100.0f, 0.0f);
  const float scaling = 0.5f;
  const float distFromCentre = c.nodeRadius * c.node0HaloSize * scaling;

  auto geo = neurythmic::GraphGeometry::makeConnectionGeometry(from, to, true,
                                                               scaling, c);
  EXPECT_TRUE(geo.isParent);
  EXPECT_NEAR(geo.start.getX(), distFromCentre + 1.0f, 1e-3f);
  EXPECT_NEAR(geo.start.getY(), 0.0f, 1e-3f);
  EXPECT_NEAR(geo.end.getX(),
              to.getX() - (distFromCentre + c.arrowHeadSize + 1.0f), 1e-3f);
  EXPECT_NEAR(geo.arrowTip.getX(), to.getX() - (distFromCentre + 1.0f), 1e-3f);
}

TEST(GraphGeometry, MakeConnectionGeometryInputMatchesInputEdge) {
  const auto& c = cfg();
  const Vec2 from(0.0f, 0.0f);
  const Vec2 to(100.0f, 0.0f);
  const float scaling = 0.5f;

  auto ie = neurythmic::GraphGeometry::makeInputEdge(from, to, scaling, c);
  auto geo = neurythmic::GraphGeometry::makeConnectionGeometry(from, to, false,
                                                               scaling, c);
  EXPECT_FALSE(geo.isParent);
  EXPECT_NEAR(geo.arcCentre.getX(), ie.arcCentre.getX(), 1e-3f);
  EXPECT_NEAR(geo.arcCentre.getY(), ie.arcCentre.getY(), 1e-3f);
  EXPECT_NEAR(geo.radius, ie.radius, 1e-3f);
  EXPECT_NEAR(geo.startAngle, ie.startAngle, 1e-3f);
  EXPECT_NEAR(geo.endAngle, ie.endAngle, 1e-3f);
  EXPECT_NEAR(geo.arrowTip.getX(), ie.drawArrowEnd.getX(), 1e-3f);
  EXPECT_NEAR(geo.arrowTip.getY(), ie.drawArrowEnd.getY(), 1e-3f);
  EXPECT_NEAR(geo.arrowAngleDeg, ie.arrowAngleDeg, 1e-3f);
}

// isStraightConnectionAtPoint ------------------------------------------------

TEST(GraphGeometry, IsStraightConnectionAtPointHitsOnSegment) {
  using neurythmic::GraphGeometry::isStraightConnectionAtPoint;
  EXPECT_TRUE(isStraightConnectionAtPoint({5.0f, 0.0f}, {0.0f, 0.0f},
                                          {10.0f, 0.0f}, 4.0f));
  EXPECT_TRUE(isStraightConnectionAtPoint({5.0f, 1.0f}, {0.0f, 0.0f},
                                          {10.0f, 0.0f}, 4.0f));
}

TEST(GraphGeometry, IsStraightConnectionAtPointRespectsWidthAndEnds) {
  using neurythmic::GraphGeometry::isStraightConnectionAtPoint;
  EXPECT_TRUE(isStraightConnectionAtPoint({5.0f, 1.9f}, {0.0f, 0.0f},
                                          {10.0f, 0.0f}, 4.0f));
  EXPECT_FALSE(isStraightConnectionAtPoint({5.0f, 2.1f}, {0.0f, 0.0f},
                                           {10.0f, 0.0f}, 4.0f));
  EXPECT_FALSE(isStraightConnectionAtPoint({-1.0f, 0.0f}, {0.0f, 0.0f},
                                           {10.0f, 0.0f}, 4.0f));
  EXPECT_FALSE(isStraightConnectionAtPoint({11.0f, 0.0f}, {0.0f, 0.0f},
                                           {10.0f, 0.0f}, 4.0f));
}

TEST(GraphGeometry, IsStraightConnectionAtPointVertical) {
  using neurythmic::GraphGeometry::isStraightConnectionAtPoint;
  EXPECT_TRUE(isStraightConnectionAtPoint({0.0f, 5.0f}, {0.0f, 0.0f},
                                          {0.0f, 10.0f}, 4.0f));
  EXPECT_FALSE(isStraightConnectionAtPoint({3.0f, 5.0f}, {0.0f, 0.0f},
                                           {0.0f, 10.0f}, 4.0f));
}

// isCurvedConnectionAtPoint --------------------------------------------------

TEST(GraphGeometry, IsCurvedConnectionAtPointHitsOnArc) {
  using neurythmic::GraphGeometry::isCurvedConnectionAtPoint;
  const float a = kPi / 4.0f;
  EXPECT_TRUE(isCurvedConnectionAtPoint({10.0f * std::cos(a), 10.0f * std::sin(a)},
                                        {0.0f, 0.0f}, 10.0f, 0.0f, kPi / 2.0f,
                                        2.0f));
}

TEST(GraphGeometry, IsCurvedConnectionAtPointRespectsRadiusAndAngle) {
  using neurythmic::GraphGeometry::isCurvedConnectionAtPoint;
  const float a = kPi / 4.0f;
  // radius 11 is within radius ± 2.
  EXPECT_TRUE(isCurvedConnectionAtPoint(
      {11.0f * std::cos(a), 11.0f * std::sin(a)}, {0.0f, 0.0f}, 10.0f, 0.0f,
      kPi / 2.0f, 2.0f));
  // radius 13 exceeds radius + 2.
  EXPECT_FALSE(isCurvedConnectionAtPoint(
      {13.0f * std::cos(a), 13.0f * std::sin(a)}, {0.0f, 0.0f}, 10.0f, 0.0f,
      kPi / 2.0f, 2.0f));
  // on the arc radius but outside the [0, pi/2] angle span.
  const float neg = -kPi / 4.0f;
  EXPECT_FALSE(isCurvedConnectionAtPoint(
      {10.0f * std::cos(neg), 10.0f * std::sin(neg)}, {0.0f, 0.0f}, 10.0f, 0.0f,
      kPi / 2.0f, 2.0f));
}

