/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/** 
 * @file    testNonlinearOptimizer.cpp
 * @brief   Unit tests for NonlinearOptimizer class
 * @author  Frank Dellaert
 */

#include <iostream>
using namespace std;

#include <boost/assign/std/list.hpp> // for operator +=
using namespace boost::assign;

#include <CppUnitLite/TestHarness.h>

#include <boost/shared_ptr.hpp>
using namespace boost;

#include <gtsam/base/Matrix.h>
#include <gtsam/slam/smallExample.h>
#include <gtsam/slam/pose2SLAM.h>
#include <gtsam/linear/GaussianFactorGraph.h>
#include <gtsam/linear/NoiseModel.h>

// template definitions
#include <gtsam/nonlinear/NonlinearFactorGraph.h>
#include <gtsam/nonlinear/NonlinearOptimizer.h>

using namespace gtsam;

const double tol = 1e-5;

Key kx(size_t i) { return Symbol('x',i); }
Key kl(size_t i) { return Symbol('l',i); }

/* ************************************************************************* */
TEST( NonlinearOptimizer, iterateLM )
{
	// really non-linear factor graph
  shared_ptr<example::Graph> fg(new example::Graph(
  		example::createReallyNonlinearFactorGraph()));

	// config far from minimum
	Point2 x0(3,0);
	boost::shared_ptr<Values> config(new Values);
	config->insert(simulated2D::PoseKey(1), x0);

	// ordering
	shared_ptr<Ordering> ord(new Ordering());
	ord->push_back(kx(1));

	// create initial optimization state, with lambda=0
	NonlinearOptimizer::auto_ptr optimizer = LevenbergMarquardtOptimizer(fg, config, LevenbergMarquardtParams(), ord).update(0.0);

	// normal iterate
	NonlinearOptimizer::auto_ptr iterated1 = GaussNewtonOptimizer(fg, config, GaussNewtonParams(), ord).iterate();

	// LM iterate with lambda 0 should be the same
	NonlinearOptimizer::auto_ptr iterated2 = LevenbergMarquardtOptimizer(fg, config, LevenbergMarquardtParams(), ord).update(0.0)->iterate();

	CHECK(assert_equal(*iterated1.values(), *iterated2.values(), 1e-9));
}

/* ************************************************************************* */
TEST( NonlinearOptimizer, optimize )
{
  shared_ptr<example::Graph> fg(new example::Graph(
  		example::createReallyNonlinearFactorGraph()));

	// test error at minimum
	Point2 xstar(0,0);
	Values cstar;
	cstar.insert(simulated2D::PoseKey(1), xstar);
	DOUBLES_EQUAL(0.0,fg->error(cstar),0.0);

	// test error at initial = [(1-cos(3))^2 + (sin(3))^2]*50 =
	Point2 x0(3,3);
	boost::shared_ptr<Values> c0(new Values);
	c0->insert(simulated2D::PoseKey(1), x0);
	DOUBLES_EQUAL(199.0,fg->error(*c0),1e-3);

	// optimize parameters
	shared_ptr<Ordering> ord(new Ordering());
	ord->push_back(kx(1));

	// initial optimization state is the same in both cases tested
	boost::shared_ptr<NonlinearOptimizationParameters> params = boost::make_shared<NonlinearOptimizationParameters>();
	params->relDecrease_ = 1e-5;
	params->absDecrease_ = 1e-5;
	Optimizer optimizer(fg, c0, ord, params);

	// Gauss-Newton
	NonlinearOptimizer::auto_ptr actual1 = *GaussNewtonOptimizer(fg, c0, GaussNewtonParams(), ord).optimize();
	DOUBLES_EQUAL(0,fg->error(*(actual1->values())),tol);

	// Levenberg-Marquardt
	Optimizer actual2 = *LevenbergMarquardtOptimizer(fg, c0, LevenbergMarquardtParams(), ord).optimizer();
	DOUBLES_EQUAL(0,fg->error(*(actual2->values())),tol);
}

/* ************************************************************************* */
TEST( NonlinearOptimizer, SimpleLMOptimizer )
{
	shared_ptr<example::Graph> fg(new example::Graph(
			example::createReallyNonlinearFactorGraph()));

	Point2 x0(3,3);
	boost::shared_ptr<Values> c0(new Values);
	c0->insert(simulated2D::PoseKey(1), x0);

	Optimizer::shared_values actual = Optimizer::optimizeLM(fg, c0);
	DOUBLES_EQUAL(0,fg->error(*actual),tol);
}

/* ************************************************************************* */
TEST( NonlinearOptimizer, SimpleLMOptimizer_noshared )
{
	example::Graph fg = example::createReallyNonlinearFactorGraph();

	Point2 x0(3,3);
	Values c0;
	c0.insert(simulated2D::PoseKey(1), x0);

	Optimizer::shared_values actual = Optimizer::optimizeLM(fg, c0);
	DOUBLES_EQUAL(0,fg.error(*actual),tol);
}

/* ************************************************************************* */
TEST( NonlinearOptimizer, SimpleGNOptimizer )
{
	shared_ptr<example::Graph> fg(new example::Graph(
			example::createReallyNonlinearFactorGraph()));

	Point2 x0(3,3);
	boost::shared_ptr<Values> c0(new Values);
	c0->insert(simulated2D::PoseKey(1), x0);

	Optimizer::shared_values actual = Optimizer::optimizeGN(fg, c0);
	DOUBLES_EQUAL(0,fg->error(*actual),tol);
}

/* ************************************************************************* */
TEST( NonlinearOptimizer, SimpleGNOptimizer_noshared )
{
	example::Graph fg = example::createReallyNonlinearFactorGraph();

	Point2 x0(3,3);
	Values c0;
	c0.insert(simulated2D::PoseKey(1), x0);

	Optimizer::shared_values actual = Optimizer::optimizeGN(fg, c0);
	DOUBLES_EQUAL(0,fg.error(*actual),tol);
}

/* ************************************************************************* */
TEST( NonlinearOptimizer, optimization_method )
{
  GaussNewtonParams params;
  params.elimination = GaussNewtonParams::QR;
  EXPECT(false);

	example::Graph fg = example::createReallyNonlinearFactorGraph();

	Point2 x0(3,3);
	Values c0;
	c0.insert(simulated2D::PoseKey(1), x0);

	Values actualMFQR = optimize<example::Graph>(
			fg, c0, *NonlinearOptimizationParameters().newFactorization(true), MULTIFRONTAL, LM);
	DOUBLES_EQUAL(0,fg.error(actualMFQR),tol);

	Values actualMFLDL = optimize<example::Graph>(
			fg, c0, *NonlinearOptimizationParameters().newFactorization(false), MULTIFRONTAL, LM);
	DOUBLES_EQUAL(0,fg.error(actualMFLDL),tol);
}

/* ************************************************************************* */
TEST( NonlinearOptimizer, Factorization )
{
	typedef NonlinearOptimizer<pose2SLAM::Graph, GaussianFactorGraph, GaussianSequentialSolver > Optimizer;

	boost::shared_ptr<Values> config(new Values);
	config->insert(pose2SLAM::PoseKey(1), Pose2(0.,0.,0.));
	config->insert(pose2SLAM::PoseKey(2), Pose2(1.5,0.,0.));

	boost::shared_ptr<pose2SLAM::Graph> graph(new pose2SLAM::Graph);
	graph->addPrior(1, Pose2(0.,0.,0.), noiseModel::Isotropic::Sigma(3, 1e-10));
	graph->addOdometry(1,2, Pose2(1.,0.,0.), noiseModel::Isotropic::Sigma(3, 1));

	boost::shared_ptr<Ordering> ordering(new Ordering);
	ordering->push_back(pose2SLAM::PoseKey(1));
	ordering->push_back(pose2SLAM::PoseKey(2));

	Optimizer optimizer(graph, config, ordering);
	Optimizer optimized = optimizer.iterateLM();

	Values expected;
	expected.insert(pose2SLAM::PoseKey(1), Pose2(0.,0.,0.));
	expected.insert(pose2SLAM::PoseKey(2), Pose2(1.,0.,0.));
	CHECK(assert_equal(expected, *optimized.values(), 1e-5));
}

/* ************************************************************************* */
TEST_UNSAFE(NonlinearOptimizer, NullFactor) {

  shared_ptr<example::Graph> fg(new example::Graph(
      example::createReallyNonlinearFactorGraph()));

  // Add null factor
  fg->push_back(example::Graph::sharedFactor());

  // test error at minimum
  Point2 xstar(0,0);
  Values cstar;
  cstar.insert(simulated2D::PoseKey(1), xstar);
  DOUBLES_EQUAL(0.0,fg->error(cstar),0.0);

  // test error at initial = [(1-cos(3))^2 + (sin(3))^2]*50 =
  Point2 x0(3,3);
  boost::shared_ptr<Values> c0(new Values);
  c0->insert(simulated2D::PoseKey(1), x0);
  DOUBLES_EQUAL(199.0,fg->error(*c0),1e-3);

  // optimize parameters
  shared_ptr<Ordering> ord(new Ordering());
  ord->push_back(kx(1));

  // initial optimization state is the same in both cases tested
  boost::shared_ptr<NonlinearOptimizationParameters> params = boost::make_shared<NonlinearOptimizationParameters>();
  params->relDecrease_ = 1e-5;
  params->absDecrease_ = 1e-5;
  Optimizer optimizer(fg, c0, ord, params);

  // Gauss-Newton
  Optimizer actual1 = optimizer.gaussNewton();
  DOUBLES_EQUAL(0,fg->error(*(actual1.values())),tol);

  // Levenberg-Marquardt
  Optimizer actual2 = optimizer.levenbergMarquardt();
  DOUBLES_EQUAL(0,fg->error(*(actual2.values())),tol);
}

///* ************************************************************************* */
// SL-FIX TEST( NonlinearOptimizer, SubgraphSolver )
//{
//	using namespace pose2SLAM;
//	typedef SubgraphSolver<Graph, Values> Solver;
//	typedef NonlinearOptimizer<Graph, Values, SubgraphPreconditioner, Solver> Optimizer;
//
//	// Create a graph
//	boost::shared_ptr<Graph> graph(new Graph);
//	graph->addPrior(1, Pose2(0., 0., 0.), noiseModel::Isotropic::Sigma(3, 1e-10));
//	graph->addConstraint(1, 2, Pose2(1., 0., 0.), noiseModel::Isotropic::Sigma(3, 1));
//
//	// Create an initial config
//	boost::shared_ptr<Values> config(new Values);
//	config->insert(1, Pose2(0., 0., 0.));
//	config->insert(2, Pose2(1.5, 0., 0.));
//
//	// Create solver and optimizer
//	Optimizer::shared_solver solver
//		(new SubgraphSolver<Graph, Values> (*graph, *config));
//	Optimizer optimizer(graph, config, solver);
//
//	// Optimize !!!!
//	double relativeThreshold = 1e-5;
//	double absoluteThreshold = 1e-5;
//	Optimizer optimized = optimizer.gaussNewton(relativeThreshold,
//			absoluteThreshold, Optimizer::SILENT);
//
//	// Check solution
//	Values expected;
//	expected.insert(1, Pose2(0., 0., 0.));
//	expected.insert(2, Pose2(1., 0., 0.));
//	CHECK(assert_equal(expected, *optimized.values(), 1e-5));
//}

/* ************************************************************************* */
// SL-FIX TEST( NonlinearOptimizer, MultiFrontalSolver )
//{
//	shared_ptr<example::Graph> fg(new example::Graph(
//			example::createNonlinearFactorGraph()));
//	Optimizer::shared_values initial = example::sharedNoisyValues();
//
//	Values expected;
//	expected.insert(simulated2D::PoseKey(1), Point2(0.0, 0.0));
//	expected.insert(simulated2D::PoseKey(2), Point2(1.5, 0.0));
//	expected.insert(simulated2D::PointKey(1), Point2(0.0, -1.0));
//
//	Optimizer::shared_solver solver;
//
//	// Check one ordering
//	shared_ptr<Ordering> ord1(new Ordering());
//	*ord1 += kx(2),kl(1),kx(1);
//	solver = Optimizer::shared_solver(new Optimizer::solver(ord1));
//	Optimizer optimizer1(fg, initial, solver);
//
//	Values actual = optimizer1.levenbergMarquardt();
//	CHECK(assert_equal(actual,expected));
//}


/* ************************************************************************* */
int main() {
	TestResult tr;
	return TestRegistry::runAllTests(tr);
}
/* ************************************************************************* */
