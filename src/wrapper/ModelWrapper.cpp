#include "sbmlsim/internal/wrapper/ModelWrapper.h"

ModelWrapper::ModelWrapper(const Model *model) {
  // species
  for (auto i = 0; i < model->getNumSpecies(); i++) {
    auto species = model->getSpecies(i);
    this->specieses.push_back(SpeciesWrapper(species));
  }

  // global parameters
  for (auto i = 0; i < model->getNumParameters(); i++) {
    auto parameter = model->getParameter(i);
    this->parameters.push_back(ParameterWrapper::asGlobalParameter(parameter));
  }

  // local parameters
  for (auto i = 0; i < model->getNumReactions(); i++) {
    auto reaction = model->getReaction(i);
    auto kineticLaw = reaction->getKineticLaw();
    for (auto j = 0; j < kineticLaw->getNumLocalParameters(); j++) {
      auto parameter = kineticLaw->getLocalParameter(i);
      this->parameters.push_back(ParameterWrapper::asLocalParameter(parameter, reaction));
    }
  }

  // compartments
  for (auto i = 0; i < model->getNumCompartments(); i++) {
    auto compartment = model->getCompartment(i);
    this->compartments.push_back(CompartmentWrapper(compartment));
  }

  // reactions
  for (auto i = 0; i < model->getNumReactions(); i++) {
    auto reaction = model->getReaction(i);
    this->reactions.push_back(ReactionWrapper(reaction));
  }

  // function definitions
  for (auto i = 0; i < model->getNumFunctionDefinitions(); i++) {
    auto functionDefinition = model->getFunctionDefinition(i);
    this->functionDefinitions.push_back(FunctionDefinitionWrapper(functionDefinition));
  }

  // events
  for (auto i = 0; i < model->getNumEvents(); i++) {
    auto event = model->getEvent(i);
    this->events.push_back(new EventWrapper(event));
  }
}

ModelWrapper::ModelWrapper(const ModelWrapper &model) {
  this->specieses = model.specieses;
  this->parameters = model.parameters;
  this->compartments = model.compartments;
  this->reactions = model.reactions;
  this->functionDefinitions = model.functionDefinitions;
  this->events = model.events;
}

ModelWrapper::~ModelWrapper() {
  this->specieses.clear();
  this->parameters.clear();
  this->compartments.clear();
  this->reactions.clear();
  this->functionDefinitions.clear();

  for (auto event : this->events) {
    delete event;
  }
  this->events.clear();
}

const std::vector<SpeciesWrapper> &ModelWrapper::getSpecieses() const {
  return this->specieses;
}

const std::vector<ParameterWrapper> &ModelWrapper::getParameters() const {
  return this->parameters;
}

const std::vector<CompartmentWrapper> &ModelWrapper::getCompartments() const {
  return this->compartments;
}

const std::vector<ReactionWrapper> &ModelWrapper::getReactions() const {
  return this->reactions;
}

const std::vector<FunctionDefinitionWrapper> &ModelWrapper::getFunctionDefinitions() const {
  return this->functionDefinitions;
}

std::vector<EventWrapper *> &ModelWrapper::getEvents() {
  return this->events;
}