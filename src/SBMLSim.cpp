#include "sbmlsim/SBMLSim.h"

#include <iostream>
#include <boost/numeric/odeint.hpp>
#include "sbmlsim/internal/system/SBMLSystem.h"
#include "sbmlsim/internal/system/SBMLSystemJacobi.h"
#include "sbmlsim/internal/integrate/IntegrateConst.h"
#include "sbmlsim/internal/observer/StdoutCsvObserver.h"

using namespace boost::numeric;
using state = SBMLSystem::state;

void SBMLSim::simulate(const std::string &filepath, const RunConfiguration &conf) {
  SBMLReader reader;
  SBMLDocument *document = reader.readSBMLFromFile(filepath);
  simulate(document, conf);
  delete document;
}

void SBMLSim::simulate(const SBMLDocument *document, const RunConfiguration &conf) {
  const Model *model = document->getModel();
  unsigned int level = document->getLevel();
  unsigned int version = document->getVersion();
  simulate(model, level, version, conf);
}

void SBMLSim::simulate(const Model *model, unsigned int level, unsigned int version, const RunConfiguration &conf) {
  Model *clonedModel = model->clone();
  SBMLDocument *dummyDocument = new SBMLDocument(level, version);
  clonedModel->setSBMLDocument(dummyDocument);
  dummyDocument->setModel(clonedModel);

  ModelWrapper *modelWrapper = new ModelWrapper(clonedModel);

  // simulateRungeKutta4(modelWrapper, conf);
  simulateRungeKuttaDopri5(modelWrapper, conf);
  // simulateRungeKuttaFehlberg78(modelWrapper, conf);
  // simulateRosenbrock4(modelWrapper, conf);

  delete modelWrapper;
  delete dummyDocument;
}

void SBMLSim::simulateRungeKutta4(const ModelWrapper *model, const RunConfiguration &conf) {
  SBMLSystem system(model);
  odeint::runge_kutta4<state> stepper;
  auto initialState = system.getInitialState();
  StdoutCsvObserver observer(system.createOutputTargetsFromOutputFields(conf.getOutputFields()));

  // print header
  observer.outputHeader();

  // integrate
  sbmlsim::integrate_const(
      stepper, system, initialState, conf.getStart(), conf.getDuration(), conf.getStepInterval(), std::ref(observer));
}

void SBMLSim::simulateRungeKuttaDopri5(const ModelWrapper *model, const RunConfiguration &conf) {
  SBMLSystem system(model);
  auto stepper = odeint::make_controlled<odeint::runge_kutta_dopri5<state> >(
      conf.getAbsoluteTolerance(), conf.getRelativeTolerance());
  auto initialState = system.getInitialState();
  StdoutCsvObserver observer(system.createOutputTargetsFromOutputFields(conf.getOutputFields()));

  // print header
  observer.outputHeader();

  // integrate
  sbmlsim::integrate_const(
      stepper, system, initialState, conf.getStart(), conf.getDuration(), conf.getStepInterval(), std::ref(observer));
}

void SBMLSim::simulateRungeKuttaFehlberg78(const ModelWrapper *model, const RunConfiguration &conf) {
  SBMLSystem system(model);
  auto stepper = odeint::make_controlled<odeint::runge_kutta_fehlberg78<state> >(
      conf.getAbsoluteTolerance(), conf.getRelativeTolerance());
  auto initialState = system.getInitialState();
  StdoutCsvObserver observer(system.createOutputTargetsFromOutputFields(conf.getOutputFields()));

  // print header
  observer.outputHeader();

  // integrate
  sbmlsim::integrate_const(
      stepper, system, initialState, conf.getStart(), conf.getDuration(), conf.getStepInterval(), std::ref(observer));
}

void SBMLSim::simulateRosenbrock4(const ModelWrapper *model, const RunConfiguration &conf) {
  SBMLSystem system(model);
  SBMLSystemJacobi systemJacobi;
  auto initialState = system.getInitialState();
  auto stepper = odeint::make_dense_output(conf.getAbsoluteTolerance(), conf.getRelativeTolerance(),
                                           odeint::rosenbrock4<double>());
  auto implicitSystem = std::make_pair(system, systemJacobi);
  StdoutCsvObserver observer(system.createOutputTargetsFromOutputFields(conf.getOutputFields()));

  // print header
  observer.outputHeader();

  // integrate
  integrate_const(stepper, implicitSystem, initialState, conf.getStart(), conf.getDuration(), conf.getStepInterval(),
                  std::ref(observer));
}
