#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#define SAFEALLOC(var,Type) if((var=(Type*)malloc(sizeof(Type)))==NULL)perror("not enough memory");

int expr();
int typeName();
int arrayDecl();
int exprUnary();
int typeBase();
int declVar();
int stmCompound();

// codurile AL
enum{
  ID,  // identificatori
  BREAK, CHAR, DOUBLE, ELSE, FOR, IF, INT, RETURN, STRUCT, VOID, WHILE, // key words
  CT_INT, CT_REAL, CT_CHAR, CT_STRING,  // constante
  COMMA, SEMICOLON, LPAR, RPAR, LBRACKET, RBRACKET, LACC, RACC, // delimitatori
  ADD, SUB, MUL, DIV, DOT, AND, OR, NOT, ASSIGN, EQUAL, NOTEQ, LESS, LESSEQ, GREATER, GREATEREQ,  // operatori
  END // EOF
};

int line = 1; // linia curenta din fisierul de intrare
char *pch;  // pointer la pozitia curenta din buffer-ul unde se citeste din fisier

char *enumNames[] = {
  "ID",
  "BREAK", "CHAR", "DOUBLE", "ELSE", "FOR", "IF", "INT", "RETURN", "STRUCT", "VOID", "WHILE",
  "CT_INT", "CT_REAL", "CT_CHAR", "CT_STRING",
  "COMMA", "SEMICOLON", "LPAR", "RPAR", "LBRACKET", "RBRACKET", "LACC", "RACC",
  "ADD", "SUB", "MUL", "DIV", "DOT", "AND","OR", "NOT", "ASSIGN", "EQUAL", "NOTEQ", "LESS", "LESSEQ", "GREATER", "GREATEREQ",
  "END"};

typedef struct _Token
{
  int code; // codul (numele)
  union
  {
    char *text; // folosit pentru ID, CT_STRING (alocat dinamic)
    long int i; // folosit pentru CT_INT, CT_CHAR
    double r; // folosit pentru CT_REAL
  };
  int line; // linia din fisierul de intrare
  struct _Token *next; // inlantuire la urmatorul AL
}Token;

  /*  */

struct _Symbol;
typedef struct _Symbol Symbol;

enum{ TB_INT, TB_DOUBLE, TB_CHAR, TB_STRUCT, TB_VOID};

typedef struct{
  int typeBase; // TB_*
  Symbol *s; // struct definition for TB_STRUCT
  int nElements; // >0 array of given size, 0=array without size, <0 non array
}Type;

enum{ CLS_VAR, CLS_FUNC, CLS_EXTFUNC, CLS_STRUCT };
enum{ MEM_GLOBAL, MEM_ARG, MEM_LOCAL };

typedef struct
{
  Symbol **begin; // the beginning of the symbols, or NULL
  Symbol **end; // the position after the last symbol
  Symbol **after; // the position after the allocated space
}Symbols;

typedef struct _Symbol
{
  const char *name; // a reference to the name stored in a token
  int cls; // CLS_*
  int mem; // MEM_*
  Type type;
  int depth; // 0-global, 1-in function, 2... - nested blocks in function
  union
  {
    Symbols args; // used only of functions
    Symbols members; // used only for structs
  };
}Symbol;

Symbols symbols;
int crtDepth = 0;
Symbol *crtStruct, *crtFunc;

void initSymbols(Symbols *symbols)
{
symbols->begin=NULL;
symbols->end=NULL;
symbols->after=NULL;
}

Symbol *addSymbol(Symbols *symbols,const char *name,int cls)
{
  Symbol *s;

  if(symbols->end==symbols->after)
  { 
    // create more room
    int count=symbols->after-symbols->begin;
    int n=count*2; // double the room
    if(n==0)n=1; // needed for the initial case
    symbols->begin=(Symbol**)realloc(symbols->begin, n*sizeof(Symbol*));
    if(symbols->begin==NULL)perror("not enough memory");
    symbols->end=symbols->begin+count;
    symbols->after=symbols->begin+n;
  }
  SAFEALLOC(s,Symbol);
  *symbols->end++=s;
  s->name=name;
  s->cls=cls;
  s->depth=crtDepth;

  return s;
}

Symbol *findSymbol(Symbols *symbols,const char *name)
{
  Symbol **p, *s;
  int foundSymbol = 0;

  for(p = symbols->begin; p != symbols->end; p++)
  {
    if(strcmp((*p)->name, name) == 0 && (*p)->depth == crtDepth )
    {
      foundSymbol = 1;
      s = *p;
    }
  }

  if(foundSymbol == 1) return s;
  
  return NULL;
}

void deleteSymbolsAfter(Symbols *symbols, Symbol *crtSymbol)
{
  Symbol **p, **aux;

  for(p = symbols->begin; p != symbols->end; p++)
  {
    if(strcmp((*p)->name, crtSymbol->name) == 0 && (*p)->depth == crtSymbol->depth) break;
  }

  while(p != symbols->end)
  {
    aux = p;
    p++;

    free(aux);
  }
}

Token *tokens = NULL, *lastToken = NULL;
Token *crtToken, *consumedToken;

Token *symbolToken;
Type *symbolType;

Token *addTk(int code)
{
  Token *tk;
  SAFEALLOC(tk, Token);

  tk->code=code;
  tk->line=line;
  tk->next=NULL;

  if(lastToken != NULL) lastToken->next = tk;
    else tokens = tk;

  lastToken = tk;
  return tk;
}

char *createString(char *begin_ch, char *end_ch)
{
  int i = 0;
  static char *str;
  char ch;

  str = (char *)malloc(sizeof(char));

  for(char *p = begin_ch; p < end_ch; p++)
  {
    if(*p == '\\')
    {
      if(*(p+1) == 'a') ch = '\a';
      else if(*(p+1) == 'b') ch = '\b';
      else if(*(p+1) == 'f') ch = '\f';
      else if(*(p+1) == 'n') ch = '\n';
      else if(*(p+1) == 'r') ch = '\r';
      else if(*(p+1) == 't') ch = '\t';
      else if(*(p+1) == 'v') ch = '\v';
      else if(*(p+1) == '\'') ch = '\'';
      else if(*(p+1) == '?') ch = '?';
      else if(*(p+1) == '\"') ch = '\"';
      else if(*(p+1) == '\\') ch = '\\';
      else if(*(p+1) == '\0') ch = '\0';

      p++;
    }
    else ch = *p;

    str[i++] = ch;
    realloc(str, i+1);
  }

  realloc(str, i+1);
  str[i] = '\0';

  return str;
}

 int getNextToken()
 {
   int state = 0, aux_line = 0;
   int base;
   // char ch;
   char *begin, *end, *aux; // begin si end -> pointeri catre inceputul si sfarsitul cuvantului/caracterului
                            //care va fi memorat in CT_STRING/CT_CHAR
   Token *tk;

   while (1)
   {
     // ch = *pch;
     //printf("starea %d: %c(%d)\n",state, *pch, *pch);

     switch(state)
     {
       case 0:
            if(*pch == '\0') { tk = addTk(END); return END;}

            if(isalpha(*pch) || *pch == '_') { state = 51; begin = pch++; break;}
            else if(*pch == '/') { state = 52; pch++; break; }
            else if(*pch == ' ' || *pch == '\r' || *pch == '\t') { pch++; break; }
            else if(*pch == '\n'){ line++; pch++; break; }

            if(*pch == '\'') { state = 14; begin = ++pch; break; }
            else if(*pch == '\"') { state = 18; begin = ++pch; break; }

            if(isdigit(*pch) && *pch != '0') { state = 1; begin = pch++; base = 10; break; }
            else if(*pch == '0') { state = 2; begin = pch++; base = 8; break; }

            if(*pch == ',') { state = 23; pch++; break; }
            else if(*pch == ';') { state = 24; pch++; break; }
            else if(*pch == '(') { state = 25; pch++; break; }
            else if(*pch == ')') { state = 26; pch++; break; }
            else if(*pch == '[') { state = 27; pch++; break; }
            else if(*pch == ']') { state = 28; pch++; break; }
            else if(*pch == '{') { state = 29; pch++; break; }
            else if(*pch == '}') { state = 30; pch++; break; }

            if(*pch == '+') { state = 31; pch++; break; }
            else if(*pch == '-') { state = 32; pch++; break; }
            else if(*pch == '*') { state = 33; pch++; break; }
            else if(*pch == '.') { state = 34; pch++; break; }
            else if(*pch == '!') { state = 39; pch++; break; }
            else if(*pch == '|') { state = 40; pch++; break; }
            else if(*pch == '&') { state = 41; pch++; break; }

            if(*pch == '=') { state = 42; pch++; break; }
            else if(*pch == '<') { state = 43; pch++; break; }
            else if(*pch == '>') { state = 44; pch++; break; }

            //daca se ajunge pana aici inseamna ca in starea 0 apare un caracter invalid ce ar cauza o bucla infinita
            printf("Invalid character at line %d: \'%c\'", line, *pch);
            exit(1);

            break;

        case 1:
            if(isdigit(*pch)) { state = 1; pch++; }
            else if(*pch == '.') { state = 8; pch++; }
            else if(*pch == 'e' || *pch == 'E') { state = 10; pch++; }
            else { state = 7; end = pch; }

            break;

        case 2:
            if(*pch >= '0' && *pch <= '7') { state = 3; pch++; }
            else if(*pch == '8' || *pch == '9') { state = 6; pch++; }
            else if(*pch == '.') { state = 8; pch++; }
            else if(*pch == 'e' || *pch == 'E') { state = 10; pch++; }
            else if(*pch == 'x') { state = 4; pch++; base = 16; }
            else { state = 7; end = pch; }

            break;

        case 3:
            if(*pch >= '0' && *pch <= '7') { state = 3; pch++; }
            else if(*pch == '8' || *pch == '9') { state = 6; pch++; }
            else if(*pch == '.') { state = 8; pch++; }
            else if(*pch == 'e' || *pch == 'E') { state = 10; pch++; }
            else { state = 7; end = pch; }

            break;

        case 4:
            if(( *pch >= '0' && *pch <= '9' ) || ( toupper(*pch) >= 'A' && toupper(*pch) <= 'F' )) { state = 5; pch++; }
            else { printf("Invalid character at line %d", line); exit(1); }

            break;

        case 5:
            if(( *pch >= '0' && *pch <= '9' ) || ( toupper(*pch) >= 'A' && toupper(*pch) <= 'F' )) { state = 5; pch++; }
            else { state = 7; end = pch; }

            break;

        case 6:
            if(isdigit(*pch)) { state = 6; pch++; }
            else if(*pch == '.') { state = 8; pch++; }
            else if(*pch == 'e' || *pch == 'E') { state = 10; pch++; }
            else { printf("Invalid character at line %d\n", line); }
            break;

        case 7:
            tk = addTk(CT_INT);

            aux = createString(begin, end);
            printf("%s\n", aux);
            tk->i = strtol(aux, NULL, base);

            return CT_INT;

        case 8:
            if(isdigit(*pch)) { state = 9; pch++; }
            else { printf("Invalid character at line %d\n", line); exit(1); }

            break;

        case 9:
            if(isdigit(*pch)) { state = 9; pch++; }
            else if(*pch == 'e' || *pch == 'E') { state = 10; pch++; }
            else { state = 13; end = pch; }

            break;

        case 10:
            if(isdigit(*pch)) { state = 12; pch++; }
            else if(*pch == '-' || *pch == '+') { state = 11; pch++; }
            else { printf("Invalid character at line %d", line); exit(1); }

            break;

        case 11:
            if(isdigit(*pch)) { state = 12; pch++; }
            else { printf("Invalid character at line %d", line); exit(1); }

            break;

        case 12:
            if(isdigit(*pch)) { state = 12; pch++; }
            else { state = 13; end = pch; }

            break;

        case 13:
            tk = addTk(CT_REAL);

            aux = createString(begin, end);
            printf("%s\n", aux);
            tk->r = atof(aux);

            return CT_REAL;

            break;

        case 14:
            if(*pch == '\\') { state = 15; pch++; }
            else { state = 16; pch++; }
            break;

        case 15:
            if(strchr("abfnrtv\'?\"\\", *pch)) { state = 16; pch++; }
            else { printf("Invalid character at line %d", line); exit(1); }
            break;

        case 16:
            if(*pch == '\''){ state = 17; end = pch++; }
            else { printf("Invalid character at line %d", line); exit(1); }
            break;

        case 17:
            tk = addTk(CT_CHAR);

            // printf("\nstart = %c --- pch = %d\n", *begin, *end);
            // printf("%s\n", createString(begin, end));
            // //strcpy(aux, createString(begin, end));

            aux = createString(begin, end);
            //tk->text = (char *)malloc(strlen(aux) * sizeof(char));
            //strcpy(tk->text, aux);
            tk->i = *aux;

            return CT_CHAR;

        case 18:
            if(*pch == '\"') { state = 22; end = pch++; }
            else if(*pch == '\\') { state = 19; pch++; }
            else if(*pch == '\n') { state = 21; pch++; line++; aux_line++; }
            else if(*pch != '\0'){ state = 21; pch++; }
            else { printf("Invalid string at line %d", line); exit(1); }

            break;

        case 19:
            if(strchr("abfnrtv\'?\"\\", *pch)) { state = 20; pch++; }
            else { printf("Invalid character at line %d", line); exit(1); }

            break;

        case 20:
            if(*pch == '\\') { state = 19; pch++; }
            else if(*pch == '\"') { state = 22; end = pch++; }
            else if(*pch == '\n') { state = 21; pch++; line++; aux_line++; }
            else if(*pch != '\0') { state = 21; pch++; }
            else { printf("Invalid string at line %d", line); exit(1); }

            break;

        case 21:
            if(*pch == '\\') { state = 19; pch++; }
            else if(*pch == '\"') { state = 22; end = pch++; }
            else if(*pch == '\n') { state = 18; pch++; line++; aux_line++; }
            else if(*pch != '\0') { state = 18; pch++; }
            else { printf("Invalid string at line %d", line); exit(1); }

            break;

        case 22:
            tk = addTk(CT_STRING);

            aux = createString(begin, end);
            //printf("----   %s  -  %c  -  %c\n", aux, *begin, *end);
            tk->text = (char *)malloc((strlen(aux)+1) * sizeof(char));
            strcpy(tk->text, aux);
            tk->line -= aux_line;

            return CT_STRING;

        case 23:
            tk = addTk(COMMA);
            return COMMA;

        case 24:
            tk = addTk(SEMICOLON);
            return SEMICOLON;

        case 25:
            tk = addTk(LPAR);
            return LPAR;

        case 26:
            tk = addTk(RPAR);
            return RPAR;

        case 27:
            tk = addTk(LBRACKET);
            return LBRACKET;

        case 28:
            tk = addTk(RBRACKET);
            return RBRACKET;

        case 29:
            tk = addTk(LACC);
            return LACC;

        case 30:
            tk = addTk(RACC);
            return RACC;

        case 31:
            tk = addTk(ADD);
            return ADD;

        case 32:
            tk = addTk(SUB);
            return SUB;

        case 33:
            tk = addTk(MUL);
            return MUL;

        case 34:
            tk = addTk(DOT);
            return DOT;

        case 35:
            tk = addTk(AND);
            return AND;

        case 36:
            tk = addTk(OR);
            return OR;

        case 37:
            tk = addTk(NOT);
            return NOT;

        case 38:
            tk = addTk(NOTEQ);
            return NOTEQ;

        case 39:
            if(*pch == '=') { state = 38; pch++; }
            else state = 37;

            break;

        case 40:
            if(*pch == '|') { state = 36; pch++; }
            else { printf("Invalid character at line %d\n", line); exit(1); }

            break;

        case 41:
            if(*pch == '&') { state = 35; pch++; }
            else { printf("Invalid character at line %d\n", line); exit(1); }

            break;

        case 42:
            if(*pch == '=') { state = 46; pch++; }
            else state = 45;

            break;

        case 43:
            if(*pch == '=') { state = 47; pch++; }
            else state = 48;

            break;

        case 44:
            if(*pch == '=') { state = 50; pch++; }
            else state = 49;

            break;

        case 45:
            tk = addTk(ASSIGN);
            return ASSIGN;

        case 46:
            tk = addTk(EQUAL);
            return EQUAL;

        case 47:
            tk = addTk(LESSEQ);
            return LESSEQ;

        case 48:
            tk = addTk(LESS);
            return LESS;

        case 49:
            tk = addTk(GREATER);
            return GREATER;

        case 50:
            tk = addTk(GREATEREQ);
            return GREATEREQ;

        case 51:
            if(isalnum(*pch) || *pch == '_') { state = 51; pch++; }
            else { state = 56; end = pch;}

            break;


        case 52:
            if(*pch == '/') { state = 53; pch++; }
            else if(*pch == '*') { state = 54; pch++; }
            else state = 57;

            break;

        case 53:
            if(*pch == '\n' || *pch == '\r' || *pch == '\0') state = 0;
            //else if(*pch == '\n') { state = 0; line++; }
            else { state = 53; pch++; }

            break;

        case 54:
            if(*pch == '*') { state = 55; pch++; }
            else if(*pch == '\n'){ state = 54; pch++; line++; }
            else { state = 54; pch++; }

            break;

        case 55:
            if(*pch == '*') { state = 55; pch++; }
            else if(*pch == '/') { state = 0; pch++; }
            else if(*pch == '\n'){ state = 54; pch++; line++; }
            else { state = 54; pch++; }

            break;

        case 56:
            tk = addTk(ID);

            aux = createString(begin, end);
            //printf("----   %s  -  %c  -  %c\n", aux, *begin, *end);

            tk->text = (char *)malloc((strlen(aux)+1) * sizeof(char));
            strcpy(tk->text, aux);
            tk->line -= aux_line;

            if(!strcmp(tk->text, "break")) tk->code = BREAK;
            else if(!strcmp(tk->text, "char")) tk->code = CHAR;
            else if(!strcmp(tk->text, "double")) tk->code = DOUBLE;
            else if(!strcmp(tk->text, "else")) tk->code = ELSE;
            else if(!strcmp(tk->text, "for")) tk->code = FOR;
            else if(!strcmp(tk->text, "if")) tk->code = IF;
            else if(!strcmp(tk->text, "int")) tk->code = INT;
            else if(!strcmp(tk->text, "return")) tk->code = RETURN;
            else if(!strcmp(tk->text, "struct")) tk->code = STRUCT;
            else if(!strcmp(tk->text, "void")) tk->code = VOID;
            else if(!strcmp(tk->text, "while")) tk->code = WHILE;

            return ID;

        case 57:
            tk = addTk(DIV);
            return DIV;


        default:
            perror("Invalid state");
            break;
     }
   }
 }

 void tkerr(const Token *tk,const char *fmt,...)
 {
   va_list va;
   va_start(va,fmt);
   fprintf(stderr,"error in line %d:  ",tk->line);
   vfprintf(stderr,fmt,va);
   fputc('\n',stderr);
   va_end(va);
   exit(-1);
 }

  void addVar(Token *tkName,Type *t)
  {
    Symbol *s;
    if(crtStruct)
    {
      if(findSymbol(&crtStruct->members,tkName->text))
        tkerr(crtToken,"symbol redefinition: %s",tkName->text);
      
      s=addSymbol(&crtStruct->members,tkName->text,CLS_VAR);
    }
    else if(crtFunc)
    {
      s=findSymbol(&symbols,tkName->text);
      if(s&&s->depth==crtDepth)
         tkerr(crtToken,"symbol redefinition: %s",tkName->text);
      s=addSymbol(&symbols,tkName->text,CLS_VAR);
      s->mem=MEM_LOCAL;
    }
    else
    {
      if(findSymbol(&symbols,tkName->text))
         tkerr(crtToken,"symbol redefinition: %s",tkName->text);
      s=addSymbol(&symbols,tkName->text,CLS_VAR);   
      s->mem=MEM_GLOBAL;
    }

    s->type=*t;
  }

  char *getTokenName(int code)
  {
    return enumNames[code];
  }

  int consume(int code)
  {
    if(crtToken->code == code)
    {
      if(crtToken->code == ID) symbolToken = crtToken;

      consumedToken = crtToken;
      printf("%d. consume: %s\n", crtToken->line, getTokenName(crtToken->code));

      crtToken = crtToken->next;
      return 1;
    }

    return 0;
  }

  int stm()
  {
    if(stmCompound()) return 1;

    if(consume(IF))
    {
      if(consume(LPAR))
      {
        if(expr())
        {
          if(consume(RPAR))
          {
            if(stm())
            {
              if(consume(ELSE))
              {
                if(!stm()) tkerr(crtToken, "incorrect statement");
              }

              return 1;
            }
            else tkerr(crtToken, "missing statement");
          }
          else tkerr(crtToken, "missing )");
        }
        else tkerr(crtToken, "wrong expression");
      }
      else tkerr(crtToken, "missing (");
    }

    if(consume(WHILE))
    {
      if(consume(LPAR))
      {
        if(expr())
        {
          if(consume(RPAR))
          {
            if(stm())
            {
              return 1;
            }
            else tkerr(crtToken, "wrong while body declaration");
          }
          else tkerr(crtToken, "missing )");
        }
        else tkerr(crtToken, "wrong expression");
      }
      else tkerr(crtToken, "missing (");
    }

    if(consume(FOR))
    {
      if(consume(LPAR))
      {
        if(expr())
        {
        }

        if(consume(SEMICOLON))
        {
          if(expr())
          {
          }

          if(consume(SEMICOLON))
          {
            if(expr())
            {
            }

            if(consume(RPAR))
            {
              if(stm()) return 1;
              else tkerr(crtToken, "missing statement");
            }
            else tkerr(crtToken, "missing )");
          }
          tkerr(crtToken, "missing ; inside for expression");
        }
        else tkerr(crtToken, "missing ; inside for expression");
      }
      else tkerr(crtToken, "missing (");
    }

    if(consume(BREAK))
    {
      if(consume(SEMICOLON)) return 1;
      else tkerr(crtToken, "missing ;");
    }

    if(consume(RETURN))
    {
      if(expr())
      {
      }

      if(consume(SEMICOLON)) return 1;
      else tkerr(crtToken, "missing ;");
    }

    if(expr())
    {
      if(consume(SEMICOLON)) return 1;
      else tkerr(crtToken, "missing ;");
    }

    if(consume(SEMICOLON)) return 1;

    return 0;
  }

  int stmCompound()
  {
    printf("%d stmCompound %s\n", crtToken->line, getTokenName(crtToken->code));

    Symbol *start=symbols.end[-1];

    if(consume(LACC))
    {
      crtDepth++;
      while(declVar() || stm());

      if(consume(RACC))
      {
        crtDepth--;
        deleteSymbolsAfter(&symbols,start);

        return 1;
      }
      else tkerr(crtToken, "missing }");
    }

    return 0;
  }

  int funcArg()
  {
    printf("%d funcArg %s\n", crtToken->line, getTokenName(crtToken->code));

    if(typeBase())
    {
      if(consume(ID))
      {
        if(arrayDecl())
        {
        }
        else symbolType->nElements = -1;

        Symbol  *s=addSymbol(&symbols, symbolToken->text,CLS_VAR);
        s->mem = MEM_ARG;
        s->type = *symbolType;
        s = addSymbol(&crtFunc->args, symbolToken->text,CLS_VAR);
        s->mem = MEM_ARG;
        s->type = *symbolType;

        return 1;
      }
      else tkerr(crtToken, "missing argument name");
    }

    return 0;
  }

  int declFunc()
  {
    printf("%d declFunc %s\n", crtToken->line, getTokenName(crtToken->code));
    Token *startToken = crtToken;

    if(typeBase())
    {
      if(consume(MUL))
      {
        symbolType->nElements = 0;
      }
      else symbolType->nElements = -1;

      if(consume(ID))
      {
        if(consume(LPAR))
        {
          if(findSymbol(&symbols,symbolToken->text))
            tkerr(crtToken,"symbol redefinition: %s",symbolToken->text);

          crtFunc=addSymbol(&symbols,symbolToken->text,CLS_FUNC);
          initSymbols(&crtFunc->args);
          crtFunc->type = *symbolType;
          crtDepth++;

          if(funcArg())
          {
            while(consume(COMMA))
            {
              if(!funcArg()) tkerr(crtToken, "wrong argument");
            }
          }

          if(consume(RPAR))
          {
            crtDepth--;

            if(stmCompound())
            {
              deleteSymbolsAfter(&symbols,crtFunc);
              crtFunc=NULL;

              return 1;
            }
            else tkerr(crtToken, "Invalid function body");
          }
          else tkerr(crtToken, "missing )");
        }
        else
        {
          crtToken = startToken;
          return 0;
        }
        //else tkerr(crtToken, "missing (");
      }
      else tkerr(crtToken, "function name not found");
    }

    if(consume(VOID))
    {
      symbolType->typeBase = TB_VOID;

      if(consume(ID))
      {
        if(consume(LPAR))
        {
          if(findSymbol(&symbols,symbolToken->text))
            tkerr(crtToken,"symbol redefinition: %s",symbolToken->text);
            
          crtFunc=addSymbol(&symbols,symbolToken->text,CLS_FUNC);
          initSymbols(&crtFunc->args);
          crtFunc->type = *symbolType;
          crtDepth++;

          if(funcArg())
          {
            while(consume(COMMA))
            {
              if(!funcArg()) tkerr(crtToken, "wrong argument");
            }
          }

          if(consume(RPAR))
          {
            crtDepth--;

            if(stmCompound())
            {
              deleteSymbolsAfter(&symbols,crtFunc);
              crtFunc=NULL;

              return 1;
            }
          }
          else tkerr(crtToken, "missing )");
        }
        else tkerr(crtToken, "missing (");
      }
      else tkerr(crtToken, "function name not found");
    }

    crtToken = startToken;
    return 0;
  }

  int typeName()
  {
    if(typeBase())
    {
      if(!arrayDecl()) symbolType->nElements = -1;

      return 1;
    }

    return 0;
  }

  int exprCast()
  {
    printf("%d exprCast %s\n", crtToken->line, getTokenName(crtToken->code));
    Token *startToken = crtToken;

    if(consume(LPAR))
    {
      if(typeName())
      {
        if(consume(RPAR))
        {
          if(exprCast()) return 1;
        }
      }

      crtToken = startToken;
    }


    if(exprUnary()) return 1;

    crtToken = startToken;
    return 0;
  }

  int exprMulPrim()
  {
    printf("%d exprMulPrim %s\n", crtToken->line, getTokenName(crtToken->code));
    Token *startToken = crtToken;
    if(consume(MUL) || consume(DIV))
    {
      if(exprCast())
      {
        if(exprMulPrim()) return 1;
      }
    }

    crtToken = startToken;
    return 1;
  }

  int exprMul()
  {
    printf("%d exprMul %s\n", crtToken->line, getTokenName(crtToken->code));

    if(exprCast())
    {
      if(exprMulPrim()) return 1;
    }

    return 0;
  }

  int exprAddPrim()
  {
    printf("%d exprAddPrim %s\n", crtToken->line, getTokenName(crtToken->code));
    Token *startToken = crtToken;
    if(consume(ADD) || consume(SUB))
    {
      if(exprMul())
      {
        if(exprAddPrim()) return 1;
      }
      else tkerr(crtToken, "Missing right argument after +/-");
    }

    crtToken = startToken;
    return 1;
  }

  int exprAdd()
  {
    printf("%d exprAdd %s\n", crtToken->line, getTokenName(crtToken->code));

    if(exprMul())
    {
      if(exprAddPrim()) return 1;
    }

    return 0;
  }

  int exprRelPrim()
  {
    printf("%d exprRelPrim %s\n", crtToken->line, getTokenName(crtToken->code));
    Token *startToken = crtToken;
    if(consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ))
    {
      if(exprAdd())
      {
        if(exprRelPrim()) return 1;
      }
    }

    crtToken = startToken;
    return 1;
  }

  int exprRel()
  {
    printf("%d exprRel %s\n", crtToken->line, getTokenName(crtToken->code));

    if(exprAdd())
    {
      if(exprRelPrim()) return 1;
    }

    return 0;
  }

  int exprEqPrim()
  {
    printf("%d exprEqPrim %s\n", crtToken->line, getTokenName(crtToken->code));
    Token *startToken = crtToken;
    if(consume(EQUAL) || consume(NOTEQ))
    {
      if(exprRel())
      {
        if(exprEqPrim()) return 1;
      }
    }

    crtToken = startToken;
    return 1;
  }

  int exprEq()
  {
    printf("%d exprEq %s\n", crtToken->line, getTokenName(crtToken->code));

    if(exprRel())
    {
      if(exprEqPrim()) return 1;
    }

    return 0;
  }

  int exprAndPrim()
  {
    printf("%d exprAndPrim %s\n", crtToken->line, getTokenName(crtToken->code));
    Token *startToken = crtToken;
    if(consume(AND))
    {
      if(exprEq())
      {
        if(exprAndPrim()) return 1;
      }
    }

    crtToken = startToken;
    return 1;
  }

  int exprAnd()
  {
    printf("%d exprAnd %s\n", crtToken->line, getTokenName(crtToken->code));

    if(exprEq())
    {
      if(exprAndPrim()) return 1;
    }

    return 0;
  }

  int exprOrPrim()
  {
    printf("%d exprOrPrim %s\n", crtToken->line, getTokenName(crtToken->code));
    Token *startToken = crtToken;
    if(consume(OR))
    {
      if(exprAnd())
      {
        if(exprOrPrim()) return 1;
      }
    }

    crtToken = startToken;
    return 1;
  }

  int exprOr()
  {
    printf("%d exprOr %s\n", crtToken->line, getTokenName(crtToken->code));

    if(exprAnd())
    {
      if(exprOrPrim()) return 1;
    }

    return 0;
  }

  int typeBase()
  {
    printf("%d typeBase %s\n", crtToken->line, getTokenName(crtToken->code));
    if(consume(INT)) 
    {
      symbolType->typeBase = TB_INT;
      return 1;
    }

    if(consume(DOUBLE))
    {
      symbolType->typeBase = TB_DOUBLE;
      return 1;
    } 

    if(consume(CHAR))
    {
      symbolType->typeBase = TB_CHAR;
      return 1;
    } 

    if(consume(STRUCT))
      if(consume(ID))
      {
        Symbol *s = findSymbol(&symbols, symbolToken->text);
        if(s==NULL)tkerr(crtToken,"undefined symbol: %s", symbolToken->text);
        if(s->cls!=CLS_STRUCT)tkerr(crtToken,"%s is not a struct", symbolToken->text);
        symbolType->typeBase=TB_STRUCT;
        symbolType->s=s;

        return 1;
      } 

    return 0;
  }

  int exprPrimary()
  {
    printf("%d exprPrimary %s\n", crtToken->line, getTokenName(crtToken->code));
    Token *startToken = crtToken;

    if(consume(ID))
    {
      if(consume(LPAR))
      {
        if(expr())
        {
          while(consume(COMMA))
          {
            if(expr())
            {

            }
            else tkerr(crtToken, "expression not found");
          }
        }

        if(consume(RPAR)) return 1;
        else tkerr(crtToken, "missing )");
      }

      //crtToken = startToken;
      return 1;
    }

    if(consume(CT_INT)) return 1;
    if(consume(CT_REAL)) return 1;
    if(consume(CT_CHAR)) return 1;
    if(consume(CT_STRING)) return 1;

    if(consume(LPAR))
    {
      if(expr())
      {
        if(consume(RPAR)) return 1;
        else tkerr(crtToken, "missing )");
      }
      else tkerr(crtToken, "no expression was found");
    }

    crtToken = startToken;
    return 0;
  }

  int exprPostfixPrim()
  {
    printf("%d exprPostfixPrim %s\n", crtToken->line, getTokenName(crtToken->code));
    Token *startToken = crtToken;

    if(consume(LBRACKET))
    {
      if(expr())
      {
        if(consume(RBRACKET))
        {
          if(exprPostfixPrim()) return 1;
        }
        else tkerr(crtToken, "Missing ]");
      }
      else tkerr(crtToken, "exprPostfixPrim error");
    }

    if(consume(DOT))
    {
      if(consume(ID))
      {
        if(exprPostfixPrim()) return 1;
      }
      else tkerr(crtToken, "Missing variable name");
    }

    crtToken = startToken;
    return 1;
  }

  int exprPostfix()
  {
    printf("%d exprPostfix %s\n", crtToken->line, getTokenName(crtToken->code));
    Token *startToken = crtToken;

    if(exprPrimary())
    {
      if(exprPostfixPrim()) return 1;
    }

    crtToken = startToken;
    return 0;
  }

  int exprUnary()
  {
    printf("%d exprUnary %s\n", crtToken->line, getTokenName(crtToken->code));
    Token *startToken = crtToken;

    if(consume(SUB) || consume(NOT))
    {
      if(exprUnary()) return 1;
      else tkerr(crtToken, "exprUnary error");
    }

    if(exprPostfix()) return 1;

    crtToken = startToken;
    return 0;
  }

  int exprAssign()
  {
    printf("%d exprAssign %s\n", crtToken->line, getTokenName(crtToken->code));
    Token *startToken = crtToken;

    //if(exprOr()) return 1;

    if(exprUnary())
    {
      if(consume(ASSIGN))
      {
        if(exprAssign()) return 1;
      }
      //else tkerr(crtToken, "Missing =");
      crtToken = startToken;
    }

    if(exprOr()) return 1;

    crtToken = startToken;
    return 0;
  }

  int expr()
  {
    if(exprAssign()) return 1;

    return 0;
  }

  int arrayDecl()
  {
    printf("%d arrayDecl %s\n", crtToken->line, getTokenName(crtToken->code));
    Token *startToken = crtToken;

    if(consume(LBRACKET))
    {
      if(expr())
      {
      }

      symbolType->nElements = 0;

      if(consume(RBRACKET)) return 1;
      else tkerr(crtToken, "Wrong array declaration; missing ]");
    }

    crtToken = startToken;
    return 0;
  }

  int declVar()
  {
    printf("%d declVar %s\n", crtToken->line, getTokenName(crtToken->code));
    Token *startToken = crtToken;

    if(typeBase())
    {
      if(consume(ID))
        {
          if(!arrayDecl()) symbolType->nElements = -1;
          addVar(symbolToken, symbolType);

          while(1)
          {
            if(consume(COMMA))
            {
              if(consume(ID))
              {
                if(!arrayDecl()) symbolType->nElements = -1;
                addVar(symbolToken, symbolType);

                continue;
              }
              else tkerr(crtToken, "Missing variable name after ,");
            }
            break;
          }

          if(consume(SEMICOLON)) return 1;
          else tkerr(crtToken, "Missing ;");
        }
        else tkerr(crtToken, "Wrong variable name");
    }

    crtToken = startToken;
    return 0;
  }

  int declStruct()
  {
    printf("%d declStruct %s\n", crtToken->line, getTokenName(crtToken->code));

    Token *startToken = crtToken;

    if(consume(STRUCT))
    {
      if(consume(ID))
      {
        if(consume(LACC))
        {
          if(findSymbol(&symbols, symbolToken->text)) 
            tkerr(crtToken,"symbol redefinition: %s", symbolToken->text);

          crtStruct=addSymbol(&symbols, symbolToken->text,CLS_STRUCT);
          initSymbols(&crtStruct->members);

          while(declVar());

          if(consume(RACC))
          {
            if(consume(SEMICOLON)) 
            {
              crtStruct = NULL;
              return 1;
            }
            else tkerr(crtToken, "Missing ;");
          }
          else tkerr(crtToken, "Missing }");
        }
        //else tkerr(crtToken, "Missing {");
      }
      else tkerr(crtToken, "Missing structure name");
    }

    crtToken = startToken;
    return 0;
  }

  // int declFunc()
  // {
  //   printf("%d declFunc %s\n", crtToken->line, getTokenName(crtToken->code));
  //
  //   return 0;
  // }

  int unit()
  {
    printf("%d unit %s\n", crtToken->line, getTokenName(crtToken->code));

    // Token *startToken = crtToken;

    while(1)
    {
      if(declStruct())
      {
      }
      else if(declFunc())
      {
      }
      else if(declVar())
      {
      }
      else break;
    }

    //printf("%s\n", getTokenName(crtToken->code));

    if(consume(END)) return 1;

    //crtToken = startToken;
    return 0;
  }

int main(int argc, char **argv)
{
  SAFEALLOC(symbolType, Type);
  SAFEALLOC(symbolToken, Token);

  char buffer[30001];

  if(argc != 2)
  {
    perror("Wrong argument(s)");
    exit(1);
  }

  FILE *input_file;
  if((input_file = fopen(argv[1], "r")) == NULL)
  {
    perror("File open error");
    exit(1);
  }

  int n = fread(buffer,1,30000,input_file);
  buffer[n] = '\0';
  pch = buffer;
  fclose(input_file);

  while(getNextToken() != END);

  crtToken = tokens;

  // if(unit()) printf("ok\n");
  // else printf("nu ok\n");

  printf("\n\n");
  for(Token *index = tokens; index != NULL; index = index->next)
    if(index->code == CT_CHAR) printf("%d CT_CHAR: %c (%ld)\n", index->line, index->i, index->i);
    else if(index->code == CT_STRING) printf("%d CT_STRING: %s\n", index->line, index->text);
    else if(index->code == ID) printf("%d ID: %s\n", index->line, index->text);
    else if(index->code == CT_REAL) printf("%d CT_REAL: %.4lf\n", index->line, index->r);
    else if(index->code == CT_INT) printf("%d CT_INT: %ld\n", index->line, index->i);
    else printf("%d %s\n", index->line, getTokenName(index->code));

  printf("\n\n-----------------------------------------------------------------------\n\n\n");

  if(unit()) printf("Sintaxa OK!\n");
  else tkerr(crtToken, "invalid syntax");

  return 0;
}
