#include "sbmlsim/internal/util/MathUtil.h"
#include <cmath>

const unsigned long long FACTORIAL_TABLE[] = { // size 20
    0, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800, 39916800, 479001600, 6227020800,
    87178291200, 1307674368000, 20922789888000, 355687428096000, 6402373705728000, 121645100408832000
    // 2432902008176640000, 51090942171709440000, 1124000727777607680000, 25852016738884976640000,
    // 620448401733239439360000, 15511210043330985984000000, 403291461126605635584000000,
    // 10888869450418352160768000000, 304888344611713860501504000000, 8841761993739701954543616000000,
    // 265252859812191058636308480000000, 8222838654177922817725562880000000, 263130836933693530167218012160000000,
    // 8683317618811886495518194401280000000, 295232799039604140847618609643520000000,
    // 10333147966386144929666651337523200000000, 371993326789901217467999448150835200000000,
    // 13763753091226345046315979581580902400000000, 523022617466601111760007224100074291200000000
};

double MathUtil::factorial(unsigned long long n) {
  if (n <= 19) {
    return FACTORIAL_TABLE[n];
  }

  double ret = FACTORIAL_TABLE[19];
  for (unsigned long long i = 20; i <= n; i++) {
    ret *= (double) i;
  }
  return ret;
}

long long MathUtil::ceil(double f) {
  return ::ceil(f);
}

ASTNode* MathUtil::differentiate(const ASTNode *ast, std::string target) {
  ASTNode *rtn = new ASTNode();

  if (containsTarget(ast, target) == 0) {
    rtn->setType(AST_INTEGER);
    rtn->setValue(0);
    return rtn;
  }

  switch (ast->getType()) {
    case AST_PLUS:
      // d{u+v}/dx = du/dx + dv/dx
      rtn->setType(AST_PLUS);
      rtn->addChild(differentiate(ast->getLeftChild(), target));
      rtn->addChild(differentiate(ast->getRightChild(), target));
      break;
    case AST_MINUS:
      // d{u-v}/dx = du/dx - dv/dx
      rtn->setType(AST_MINUS);
      rtn->addChild(differentiate(ast->getLeftChild(), target));
      rtn->addChild(differentiate(ast->getRightChild(), target));
      break;
    case AST_TIMES: {
      // d{u*v}/dx = u * dv/dx + v * du/dx
      rtn->setType(AST_PLUS);
      ASTNode *left = new ASTNode(AST_TIMES);
      left->addChild(differentiate(ast->getLeftChild(), target));
      left->addChild(ast->getRightChild()->deepCopy());
      rtn->addChild(left);
      ASTNode *right = new ASTNode(AST_TIMES);
      right->addChild(ast->getLeftChild()->deepCopy());
      right->addChild(differentiate(ast->getRightChild(), target));
      rtn->addChild(right);
      break;
    }
    case AST_DIVIDE: {
      // d{u/v}/dx = (v * du/dx - u * dv/dx) / v^2
      rtn->setType(AST_DIVIDE);
      if (containsTarget(ast->getRightChild(), target) == 0) {
        rtn->addChild(differentiate(ast->getLeftChild(), target));
        rtn->addChild(ast->getRightChild()->deepCopy());
        break;
      }
      ASTNode *left = new ASTNode(AST_MINUS);
      ASTNode *ll = new ASTNode(AST_TIMES);
      ASTNode *lr = new ASTNode(AST_TIMES);
      ll->addChild(differentiate(ast->getLeftChild(), target));
      ll->addChild(ast->getRightChild()->deepCopy());
      lr->addChild(ast->getLeftChild()->deepCopy());
      lr->addChild(differentiate(ast->getRightChild(), target));
      left->addChild(ll);
      left->addChild(lr);
      ASTNode *right = new ASTNode(AST_FUNCTION_POWER);
      right->addChild(ast->getRightChild()->deepCopy());
      ASTNode *rr = new ASTNode(AST_INTEGER);
      rr->setValue(2);
      right->addChild(rr);
      rtn->addChild(left);
      rtn->addChild(right);
      break;
    }
    case AST_FUNCTION_POWER:
    case AST_POWER: {
      // d{u^v}/dx = v * u^(v-1) * du/dx + u^v * ln(u) * dv/dx
      rtn->setType(AST_PLUS);
      // LHS
      ASTNode *left = new ASTNode(AST_TIMES);
      ASTNode *minus = new ASTNode(AST_MINUS);
      ASTNode *one = new ASTNode(AST_INTEGER);
      one->setValue(1);
      minus->addChild(ast->getRightChild()->deepCopy()); // add v
      minus->addChild(one);
      ASTNode *power = new ASTNode(AST_POWER);
      power->addChild(ast->getLeftChild()->deepCopy()); // add u
      power->addChild(minus);
      ASTNode *ll = new ASTNode(AST_TIMES);
      ll->addChild(ast->getRightChild()->deepCopy());  // add v
      ll->addChild(power);
      ASTNode *lr = differentiate(ast->getLeftChild(), target); // du/dx
      left->addChild(ll);
      left->addChild(lr);
      // RHS
      ASTNode *right = new ASTNode(AST_TIMES);
      ASTNode *rl = new ASTNode(AST_POWER);
      rl->addChild(ast->getLeftChild()->deepCopy());  // add u
      rl->addChild(ast->getRightChild()->deepCopy()); // add v
      ASTNode *rr = new ASTNode(AST_TIMES);
      ASTNode *ln = new ASTNode(AST_FUNCTION_LN);
      ln->addChild(ast->getLeftChild()->deepCopy());  // add u
      rr->addChild(ln);
      rr->addChild(differentiate(ast->getRightChild()->deepCopy(), target)); // add dv/dx
      right->addChild(rl);
      right->addChild(rr);
      // add left and right
      rtn->addChild(left);
      rtn->addChild(right);
      break;
    }
    case AST_FUNCTION_ROOT: {
      rtn->setType(AST_TIMES);
      ASTNode *left = differentiate(ast->getLeftChild(), target);
      rtn->addChild(left);
      ASTNode *right = new ASTNode(AST_TIMES);
      ASTNode *tmp2 = new ASTNode(AST_REAL);
      tmp2->setValue(0.5);
      right->addChild(tmp2);
      tmp2 = new ASTNode(AST_FUNCTION_POWER);
      tmp2->addChild(ast->getLeftChild()->deepCopy());
      ASTNode *tmp3 = new ASTNode(AST_REAL);
      tmp3->setValue(-0.5);
      tmp2->addChild(tmp3);
      right->addChild(tmp2);
      break;
    }
    case AST_FUNCTION_SIN: {
      // d{sin(u)}/dx = du/dx * cos(u)
      rtn->setType(AST_TIMES);
      ASTNode *left = differentiate(ast->getLeftChild(), target);
      ASTNode *right = new ASTNode(AST_FUNCTION_COS);
      right->addChild(ast->getLeftChild()->deepCopy());
      rtn->addChild(left);
      rtn->addChild(right);
      break;
    }
    case AST_FUNCTION_COS: {
      // d{cos(u)}/dx = -1 * du/dx * sin(u)
      rtn->setType(AST_TIMES);
      ASTNode *left = new ASTNode(AST_TIMES);
      ASTNode *ll = new ASTNode(AST_INTEGER);
      ll->setValue(-1);
      left->addChild(ll);
      left->addChild(differentiate(ast->getLeftChild(), target));
      ASTNode *right = new ASTNode(AST_FUNCTION_SIN);
      right->addChild(ast->getLeftChild()->deepCopy());
      rtn->addChild(left);
      rtn->addChild(right);
      break;
    }
    case AST_FUNCTION_TAN: {
      // d{tan(u)}/dx = du/dx * sec(u)^2
      rtn->setType(AST_TIMES);
      ASTNode *left = differentiate(ast->getLeftChild(), target);
      ASTNode *right = new ASTNode(AST_POWER);
      ASTNode *rl = new ASTNode(AST_FUNCTION_SEC);
      ASTNode *tmpl = ast->getLeftChild()->deepCopy();
      rl->addChild(tmpl);
      ASTNode *rr = new ASTNode();
      rr->setValue(2);
      right->addChild(rl);
      right->addChild(rr);
      rtn->addChild(left);
      rtn->addChild(right);
      break;
    }
    case AST_FUNCTION_SINH: {
      rtn->setType(AST_TIMES);
      ASTNode *left = differentiate(ast->getLeftChild(), target);
      ASTNode *right = new ASTNode(AST_FUNCTION_COSH);
      right->addChild(ast->getLeftChild()->deepCopy());
      rtn->addChild(left);
      rtn->addChild(right);
      break;
    }
    case AST_FUNCTION_COSH: {
      rtn->setType(AST_TIMES);
      rtn->addChild(differentiate(ast->getLeftChild(), target));
      ASTNode *right = new ASTNode(AST_FUNCTION_SINH);
      right->addChild(ast->getLeftChild()->deepCopy());
      rtn->addChild(right);
      break;
    }
    case AST_FUNCTION_TANH: {
      rtn->setType(AST_TIMES);
      rtn->addChild(differentiate(ast->getLeftChild(), target));
      ASTNode *right = new ASTNode(AST_DIVIDE);
      ASTNode *tmp2 = new ASTNode(AST_INTEGER);
      tmp2->setValue(1);
      right->addChild(tmp2);
      ASTNode *tmp4 = new ASTNode(AST_FUNCTION_POWER);
      ASTNode *cosh = new ASTNode(AST_FUNCTION_COSH);
      cosh->addChild(ast->getLeftChild()->deepCopy());
      tmp4->addChild(cosh);
      ASTNode *two = new ASTNode(AST_INTEGER);
      two->setValue(2);
      tmp4->addChild(two);
      right->addChild(tmp4);
      break;
    }
    case AST_REAL:
    case AST_INTEGER:
    case AST_NAME_TIME:
      rtn->setType(AST_INTEGER);
      rtn->setValue(0);
      break;
    case AST_NAME:
      rtn->setType(AST_INTEGER);
      if (ast->getName() == target) {
        rtn->setValue(1);
      } else {
        rtn->setValue(0);
      }
      break;
    default:
      std::cout << "unknown" << std::endl;
      exit(1);
  }
  rtn->reduceToBinary();
  return rtn;
}

bool MathUtil::containsTarget(const ASTNode *ast, std::string target)
{
  bool found = false;
  if (ast->getLeftChild() != nullptr) {
    found |= containsTarget(ast->getLeftChild(), target);
  }
  if (ast->getRightChild() != nullptr) {
    found |= containsTarget(ast->getRightChild(), target);
  }
  if (ast->getType() == AST_NAME) {
    std::string name = ast->getName();
    if (name == target) {
      return true;
    }
  }
  return found;
}

ASTNode* MathUtil::simplify(const ASTNode *ast) {
  ASTNodeType_t type = ast->getType();
  ASTNode *left, *right, *simplifiedRoot, *tmpl, *tmpr;

  if ((!ast->isOperator())
    && (ast->getType() != AST_FUNCTION_POWER)
    && (ast->getType() != AST_POWER)
    && (ast->getType() != AST_FUNCTION_LN)
    ) {
    return ast->deepCopy();
  }

  left  = simplify(ast->getLeftChild());
  auto left_val = left->getValue();

  // AST_FUNCTION_LN only has 1 argument
  if (ast->getType() == AST_FUNCTION_LN) {
    if (ast->getLeftChild()->getType() == AST_CONSTANT_E) {
      simplifiedRoot = new ASTNode();
      simplifiedRoot->setValue(1);
      return simplifiedRoot;
    } else {
      return ast->deepCopy();
    }
  }

  // take care on 2nd argument
  right = simplify(ast->getRightChild());
  auto right_val = right->getValue();

  switch (type) {
    case AST_PLUS:
      if (left->isNumber()) {
        left_val = left->getValue();
        if (left_val == 0.0) {
          return right->deepCopy();
        } else if (right->isNumber()) {
          right_val   = right->getValue();
          simplifiedRoot = new ASTNode();
          simplifiedRoot->setValue(left_val+right_val);
          return simplifiedRoot;
        } else if (right->getType() != AST_PLUS){  // left is an integer, right is not AST_PLUS. (3 + x) => (x + 3)
          simplifiedRoot = new ASTNode(AST_PLUS);
          simplifiedRoot->addChild(right->deepCopy());
          simplifiedRoot->addChild(left->deepCopy());
          return simplifiedRoot;
        }
      }
      if (right->isNumber()) { // left is not an integer
        right_val = right->getValue();
        if (right_val == 0.0) {
          return left->deepCopy();
        }
      }
      // merge "2 + x + 3" to "x + 5"
      if (left->getType() == AST_PLUS) {
        if (left->getRightChild()->isNumber() && right->isNumber()) {
          simplifiedRoot = new ASTNode(AST_PLUS);
          tmpl = left->getLeftChild()->deepCopy();
          tmpr = new ASTNode(AST_PLUS);
          tmpr->addChild(right->deepCopy());
          tmpr->addChild(left->getRightChild()->deepCopy());
          simplifiedRoot->addChild(tmpl);
          simplifiedRoot->addChild(simplify(tmpr));
          return simplifiedRoot;
        }
      }
      if (right->getType() == AST_PLUS) {
        if (right->getRightChild()->isNumber() && left->isNumber()) {
          simplifiedRoot = new ASTNode(AST_PLUS);
          tmpl = right->getLeftChild()->deepCopy();
          tmpr = new ASTNode(AST_PLUS);
          tmpr->addChild(left->deepCopy());
          tmpr->addChild(right->getRightChild()->deepCopy());
          simplifiedRoot->addChild(tmpl);
          simplifiedRoot->addChild(simplify(tmpr));
          return simplifiedRoot;
        }
      }
      break;
    case AST_MINUS:
      if (right->isNumber()) {
        right_val = right->getValue();
        if (right_val == 0.0) {
          return left->deepCopy();
        } else if (left->isNumber()) {
          left_val   = left->getValue();
          simplifiedRoot = new ASTNode();
          simplifiedRoot->setValue(left_val-right_val);
          return simplifiedRoot;
        }
      }
      break;
    case AST_TIMES:
      if (left->isNumber()) {
        left_val = left->getValue();
        if (left_val == 0.0) {
          simplifiedRoot = new ASTNode();
          simplifiedRoot->setValue(0);
          return simplifiedRoot;
        } else if (left_val == 1.0) {
          return right->deepCopy();
        } else if (right->isNumber()) {
          right_val   = right->getValue();
          simplifiedRoot = new ASTNode();
          simplifiedRoot->setValue(left_val*right_val);
          return simplifiedRoot;
        }
      }
      if (right->isNumber()) { // left is not an integer
        right_val = right->getValue();
        if (right_val == 0.0) {
          simplifiedRoot = new ASTNode();
          simplifiedRoot->setValue(0);
          return simplifiedRoot;
        } else if (right_val == 1.0) {
          return left->deepCopy();
        } else if (left->getType() != AST_TIMES){   // left is not AST_TIMES, right is an integer. (x * 2) => (2 * x)
          simplifiedRoot = new ASTNode(AST_TIMES);
          simplifiedRoot->addChild(right->deepCopy());
          simplifiedRoot->addChild(left->deepCopy());
          return simplifiedRoot;
        }
      }
      // merge "2 * x * 3" to "6 * x"
      if (left->getType() == AST_TIMES) {
        if (left->getLeftChild()->isNumber() && right->isNumber()) {
          simplifiedRoot = new ASTNode(AST_TIMES);
          tmpl = new ASTNode(AST_TIMES);
          tmpl->addChild(left->getLeftChild()->deepCopy());
          tmpl->addChild(right->deepCopy());
          tmpr = left->getRightChild()->deepCopy();
          simplifiedRoot->addChild(simplify(tmpl));
          simplifiedRoot->addChild(tmpr);
          return simplifiedRoot;
        }
      }
      if (right->getType() == AST_TIMES) {
        if (right->getLeftChild()->isNumber() && left->isNumber()) {
          simplifiedRoot = new ASTNode(AST_TIMES);
          tmpl = new ASTNode(AST_TIMES);
          tmpl->addChild(right->getLeftChild()->deepCopy());
          tmpl->addChild(left->deepCopy());
          tmpr = right->getRightChild()->deepCopy();
          simplifiedRoot->addChild(simplify(tmpl));
          simplifiedRoot->addChild(tmpr);
          return simplifiedRoot;
        }
      }
      break; // can't simplify
    case AST_DIVIDE:
      if (left->isNumber()) {
        left_val = left->getValue();
        if (left_val == 0.0) {
          simplifiedRoot = new ASTNode();
          simplifiedRoot->setValue(0);
          return simplifiedRoot;
        } else if (right->isNumber()) {
          right_val   = right->getValue();
          simplifiedRoot = new ASTNode();
          simplifiedRoot->setValue((double)left_val/right_val);
          return simplifiedRoot;
        }
      }
      if (right->isNumber()) { // left is not an integer
        right_val = right->getValue();
        if (right_val == 1.0) {
          return left->deepCopy();
        }
      }
      break; // can't simplify
    case AST_POWER:
    case AST_FUNCTION_POWER:
      if (right->isNumber()) {
        right_val = right->getValue();
        if (right_val == 0.0) {
          simplifiedRoot = new ASTNode();
          simplifiedRoot->setValue(1);
          return simplifiedRoot;
        } else if (right_val == 1.0) {
          return left->deepCopy();
        }
      }
      if (left->getType() == AST_FUNCTION_POWER || left->getType() == AST_POWER) {  // pow(pow(x, 2), 3) => pow(x, 2*3)
        simplifiedRoot = new ASTNode(AST_POWER);
        simplifiedRoot->addChild(left->getLeftChild()->deepCopy());
        tmpr = new ASTNode(AST_TIMES);
        tmpr->addChild(left->getRightChild());
        tmpr->addChild(right->deepCopy());
        simplifiedRoot->addChild(tmpr);
        return simplify(simplifiedRoot);
      }
      if (type == AST_FUNCTION_POWER) { // convert pow(x, y) to x ^ y
        simplifiedRoot = new ASTNode(AST_POWER);
        simplifiedRoot->addChild(left->deepCopy());
        simplifiedRoot->addChild(right->deepCopy());
        return simplifiedRoot;
      }
      break; // can't simplify
    default:
      break;
  }
  simplifiedRoot = new ASTNode(type);
  simplifiedRoot->addChild(left);
  simplifiedRoot->addChild(right);
  return simplifiedRoot;
}
