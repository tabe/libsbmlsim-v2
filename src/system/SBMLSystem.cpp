#include "sbmlsim/internal/system/SBMLSystem.h"
#include <algorithm>
#include <cmath>

#define UNDEFINED_SPECIES_INDEX -1
#define UNDEFINED_REACTION_INDEX -1

SBMLSystem::SBMLSystem(const ModelWrapper *model) : model(const_cast<ModelWrapper *>(model)) {
  // nothing to do
}

SBMLSystem::SBMLSystem(const SBMLSystem &system) :model(system.model) {
  // nothing to do
}

SBMLSystem::~SBMLSystem() {
  // nothing to do
}

void SBMLSystem::operator()(const state &x, state &dxdt, double t) {
  handleReaction(x, dxdt, t);
}

void SBMLSystem::handleReaction(const state& x, state& dxdt, double t) {
  // initialize
  for (auto i = 0; i < dxdt.size(); i++) {
    dxdt[i] = 0.0;
  }

  auto reactions = model->getReactions();
  for (auto i = 0; i < reactions.size(); i++) {
    auto reaction = reactions[i];
    auto node = reaction.getMath();
    auto clonedNode = node->deepCopy();
    clonedNode->reduceToBinary();

    auto value = evaluateASTNode(clonedNode, i, x);

    // reactants
    for (auto reactant : reaction.getReactants()) {
      auto index = getIndexForSpecies(reactant.getSpeciesId());
      dxdt[index] -= value * reactant.getStoichiometry();
    }

    // products
    for (auto product : reaction.getProducts()) {
      auto index = getIndexForSpecies(product.getSpeciesId());
      dxdt[index] += value * product.getStoichiometry();
    }
  }

  // boundaryCondition and constant
  auto &specieses = model->getSpecieses();
  for (auto i = 0; i < specieses.size(); i++) {
    if (specieses[i].hasBoundaryCondition() || specieses[i].isConstant()) {
      dxdt[i] = 0.0;
    }
  }
}

void SBMLSystem::handleEvent(state &x, double t) {
  for (auto event : model->getEvents()) {
    bool fire = evaluateTriggerNode(event->getTrigger(), x);
    if (fire && event->getTriggerState() == false) {
      for (auto eventAssignment : event->getEventAssignments()) {
        auto variable = eventAssignment.getVariable();
        auto speciesIndex = getIndexForSpecies(variable);
        if (speciesIndex != UNDEFINED_SPECIES_INDEX) {
          double value = evaluateASTNode(eventAssignment.getMath(), UNDEFINED_REACTION_INDEX, x);
          x[speciesIndex] = value;
          event->setTriggerState(true);
        }
      }
    } else if (!fire) {
      event->setTriggerState(false);
    }
  }
}

void SBMLSystem::handleInitialAssignment(state &x, double t) {
  for (auto initialAssignment : model->getInitialAssignments()) {
    auto symbol = initialAssignment->getSymbol();
    auto value = evaluateASTNode(initialAssignment->getMath(), UNDEFINED_REACTION_INDEX, x);

    // species
    auto specieses = model->getSpecieses();
    for (auto i = 0; i < specieses.size(); i++) {
      if (symbol == specieses[i].getId()) {
        x[i] = value;
      }
    }

    // compartment
    auto compartments = model->getCompartments();
    for (auto i = 0; i < compartments.size(); i++) {
      if (symbol == compartments[i].getId()) {
        compartments[i].setValue(value);
      }
    }

    // global parameter
    auto parameters = model->getParameters();
    for (auto i = 0; i < parameters.size(); i++) {
      if (parameters[i].isGlobalParameter() && symbol == parameters[i].getId()) {
        parameters[i].setValue(value);
      }
    }
  }
}

double SBMLSystem::evaluateASTNode(const ASTNode *node, int reactionIndex, const state& x) {
  double left, right;

  ASTNodeType_t type = node->getType();
  switch (type) {
    case AST_NAME:
      return evaluateNameNode(node, reactionIndex, x);
    case AST_PLUS:
      left = evaluateASTNode(node->getLeftChild(), reactionIndex, x);
      right = evaluateASTNode(node->getRightChild(), reactionIndex, x);
      return left + right;
    case AST_MINUS:
      left = evaluateASTNode(node->getLeftChild(), reactionIndex, x);
      right = evaluateASTNode(node->getRightChild(), reactionIndex, x);
      return left - right;
    case AST_TIMES:
      left = evaluateASTNode(node->getLeftChild(), reactionIndex, x);
      right = evaluateASTNode(node->getRightChild(), reactionIndex, x);
      return left * right;
    case AST_DIVIDE:
      left = evaluateASTNode(node->getLeftChild(), reactionIndex, x);
      right = evaluateASTNode(node->getRightChild(), reactionIndex, x);
      return left / right;
    case AST_POWER:
    case AST_FUNCTION_POWER:
      left = evaluateASTNode(node->getLeftChild(), reactionIndex, x);
      right = evaluateASTNode(node->getRightChild(), reactionIndex, x);
      return std::pow(left, right);
    case AST_FUNCTION:
      return evaluateFunctionNode(node, reactionIndex, x);
    case AST_REAL:
      return node->getReal();
    case AST_INTEGER:
      return node->getInteger();
    default:
      std::cout << "type = " << type << std::endl;
      break;
  }
  return 0;
}

double SBMLSystem::evaluateNameNode(const ASTNode *node, int reactionIndex, const state &x) {
  auto name = node->getName();

  // species
  auto specieses = model->getSpecieses();
  for (auto i = 0; i < specieses.size(); i++) {
    if (name == specieses[i].getId()) {
      if (specieses[i].shouldDivideByCompartmentSize()) {
        auto compartments = model->getCompartments();
        for (auto j = 0; j < compartments.size(); j++) {
          if (specieses[i].getCompartmentId() == compartments[j].getId()) {
            return x[i] / compartments[j].getValue();
          }
        }
      } else {
        return x[i];
      }
    }
  }

  // compartment
  auto compartments = model->getCompartments();
  for (auto i = 0; i < compartments.size(); i++) {
    if (name == compartments[i].getId()) {
      return compartments[i].getValue();
    }
  }

  // parameter
  if (reactionIndex != UNDEFINED_REACTION_INDEX) {
    auto reactionId = model->getReactions().at(reactionIndex).getId();
    auto parameters = model->getParameters();
    for (auto i = 0; i < parameters.size(); i++) {
      if (parameters[i].isLocalParameter()
          && name == parameters[i].getId()
          && reactionId == parameters[i].getReactionId()) {
        return parameters[i].getValue();
      }
    }
    for (auto i = 0; i < parameters.size(); i++) {
      if (parameters[i].isGlobalParameter() && name == parameters[i].getId()) {
        return parameters[i].getValue();
      }
    }
  }

  return 0.0;
}

double SBMLSystem::evaluateFunctionNode(const ASTNode *node, int reactionIndex, const state &x) {
  auto name = node->getName();
  for (auto functionDefinition : model->getFunctionDefinitions()) {
    if (name == functionDefinition.getName()) {
      auto body = functionDefinition.getBody()->deepCopy();
      for (auto i = 0; i < body->getNumChildren(); i++) {
        auto bvar = body->getChild(i)->getName();
        body->replaceArgument(bvar, node->getChild(i));
      }
      auto value = evaluateASTNode(body, reactionIndex, x);
      delete body;
      return value;
    }
  }

  // not reachable
  return 0.0;
}

bool SBMLSystem::evaluateTriggerNode(const ASTNode *trigger, const state &x) {
  double left, right;

  switch (trigger->getType()) {
    case AST_RELATIONAL_LT:
      left = evaluateASTNode(trigger->getLeftChild(), UNDEFINED_REACTION_INDEX, x);
      right = evaluateASTNode(trigger->getRightChild(), UNDEFINED_REACTION_INDEX, x);
      return left < right;
    default:
      std::cout << "trigger root node type = " << trigger->getType() << std::endl;
      return false;
  }
}

int SBMLSystem::getIndexForSpecies(const std::string &speciesId) {
  auto specieses = model->getSpecieses();
  for (auto i = 0; i < specieses.size(); i++) {
    if (speciesId == specieses[i].getId()) {
      return i;
    }
  }
  return UNDEFINED_SPECIES_INDEX;
}
