#include "token.h"

const char* Token_str(TokenType type) {
  switch (type) {
    case AUTO:
      return "auto";
    case BREAK:
      return "break";
    case CASE:
      return "case";
    case CHAR:
      return "char";
    case CONST:
      return "const";
    case CONTINUE:
      return "continue";
    case DEFAULT:
      return "default";
    case DO:
      return "do";
    case DOUBLE:
      return "double";
    case ELSE:
      return "else";
    case ENUM:
      return "enum";
    case EXTERN:
      return "extern";
    case FLOAT:
      return "float";
    case FOR:
      return "for";
    case GOTO:
      return "goto";
    case IF:
      return "if";
    case INLINE:
      return "inline";
    case INT:
      return "int";
    case LONG:
      return "long";
    case REGISTER:
      return "register";
    case RESTRICT:
      return "restrict";
    case RETURN:
      return "return";
    case SHORT:
      return "short";
    case SIGNED:
      return "signed";
    case SIZEOF:
      return "sizeof";
    case STATIC:
      return "static";
    case STRUCT:
      return "struct";
    case SWITCH:
      return "switch";
    case TYPEDEF:
      return "typedef";
    case UNION:
      return "union";
    case UNSIGNED:
      return "unsigned";
    case VOID:
      return "void";
    case VOLATILE:
      return "volatile";
    case WHILE:
      return "while";
    case ELLIPSIS:
      return "...";
    case RIGHT_ASSIGN:
      return ">>=";
    case LEFT_ASSIGN:
      return "<<=";
    case ADD_ASSIGN:
      return "+=";
    case SUB_ASSIGN:
      return "-=";
    case MUL_ASSIGN:
      return "*=";
    case DIV_ASSIGN:
      return "/=";
    case MOD_ASSIGN:
      return "%=";
    case AND_ASSIGN:
      return "&=";
    case XOR_ASSIGN:
      return "^=";
    case OR_ASSIGN:
      return "|=";
    case RIGHT_OP:
      return ">>";
    case LEFT_OP:
      return "<<";
    case INC_OP:
      return "++";
    case DEC_OP:
      return "--";
    case PTR_OP:
      return "->";
    case AND_OP:
      return "&&";
    case OR_OP:
      return "||";
    case LE_OP:
      return "<=";
    case GE_OP:
      return ">=";
    case EQ_OP:
      return "==";
    case NE_OP:
      return "!=";
    case SEMICOLON:
      return ";";
    case LEFT_BRACE:
      return "{";
    case RIGHT_BRACE:
      return "}";
    case COMMA:
      return ",";
    case COLON:
      return ":";
    case EQUAL:
      return "=";
    case LEFT_PAREN:
      return "(";
    case RIGHT_PAREN:
      return ")";
    case LEFT_SQUARE:
      return "[";
    case RIGHT_SQUARE:
      return "]";
    case DOT:
      return ".";
    case AMPERSAND:
      return "&";
    case BANG:
      return "!";
    case TILDE:
      return "~";
    case MINUS:
      return "-";
    case PLUS:
      return "+";
    case STAR:
      return "*";
    case SLASH:
      return "/";
    case PERCENT:
      return "%";
    case LESS_THAN:
      return "<";
    case GREATER_THAN:
      return ">";
    case CARET:
      return "^";
    case BAR:
      return "|";
    case QUESTION:
      return "?";
    case IDENTIFIER:
      return "identifier";
    case CONSTANT:
      return "constant";
    case STRING_LITERAL:
      return "string literal";
    case END_OF_FILE:
      return "End of File";
    default:
      return "(unknown)";
  }
}