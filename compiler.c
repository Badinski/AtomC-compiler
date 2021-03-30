#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define SAFEALLOC(var,Type) if((var=(Type*)malloc(sizeof(Type)))==NULL)perror("not enough memory");

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

Token *tokens = NULL, *lastToken = NULL;

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
     printf("starea %d: %c(%d)\n",state, *pch, *pch);

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

int main(int argc, char **argv)
{
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

  printf("\n\n");
  for(Token *index = tokens; index != NULL; index = index->next)
    if(index->code == CT_CHAR) printf("%d CT_CHAR: %c (%ld)\n", index->line, index->i, index->i);
    else if(index->code == CT_STRING) printf("%d CT_STRING: %s\n", index->line, index->text);
    else if(index->code == ID) printf("%d ID: %s\n", index->line, index->text);
    else if(index->code == CT_REAL) printf("%d CT_REAL: %.4lf\n", index->line, index->r);
    else if(index->code == CT_INT) printf("%d CT_INT: %ld\n", index->line, index->i);
    else printf("%d %s\n", index->line, enumNames[index->code]);

  return 0;
}
