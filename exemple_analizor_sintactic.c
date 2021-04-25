int tmp;

INT ID:tmp SEMICOLON END

a=b/2;

#include <stdbool.h>		// bool, true, false

Token *crtTk;			// iteratorul pentru analiza sintactica
Token *consumedTk;		// atomul consumat de consume

// principiul: TOTUL SAU NIMIC!!!
// - daca regula se indeplineste (returneaza true), atunci trebuie sa-si consume toti atomii
// daca regula nu se indeplineste (return false), atunci nu trebuie sa consume niciun atom

char *getName(int cod){
	// ID -> "ID"
	....
	}

// unit: ( declStruct | declFunc | declVar )* END ;
bool unit(){
	printf("%d: unit %s\n",crtTk->line,getName(crtTk->code));
	Token *startTk=crtTk;		// salvare iterator initial
	for(;;){
		if(declStruct()){
			}
		else if(declFunc()){
			}
		else if(declVar()){
			}
		else break;
		}
	if(consume(END))return true;
	crtTk=startTk;		// restaurare iterator la pozitia de start inainte de return false
	return false;
	}

//exprOr: exprOr OR exprAnd | exprAnd ;
bool exprOr(){
	Token *startTk=crtTk;
	if(exprOr()){			// RECURSIVITATE INFINITA!!!
		if(consume(OR)){
//exprOr: exprOr OR exprAnd | exprAnd ;
bool exprOr(){
	Token *startTk=crtTk;
	if(exprOr()){			// RECURSIVITATE INFINITA!!!
		if(consume(OR)){
			if(exprAnd()){
				return true;
				}
			}
		crtTk=startTk;
		}
	if(exprAnd()){
		return true;
		}
	return false;
	}
From Razvan Aciu to Everyone:  03:01 PM
int main(){
	// analizorul lexical + afisare atomi
	crtTk=tokens;		// initializare iterator
	if(unit()){
		puts("sintaxa ok");
		}else{
		puts("eroare de sintaxa");
		}
	return 0;
	}

A ::= A α1 | … | A αm | β1 | … | βn
=> A ::= β1 A’ | … | βn A’
=> A’ ::= α1 A’ | … | αm A’ | ε

A = exprOr
α1 = OR exprAnd
β1 = exprAnd

exprOr = exprAnd exprOrPrim
exprOrPrim = OR exprAnd exprOrPrim | ε
bool exprOr(){
	printf("%d: exprOr %s\n",crtTk->line,getName(crtTk->code));
	Token *startTk=crtTk;
	if(exprAnd()){
		if(exprOrPrim()){
			return true;
			}
		}
	crtTk=startTk;
	return false;
	}

bool exprOrPrim(){
	printf("%d: exprOrPrim %s\n",crtTk->line,getName(crtTk->code));
	Token *startTk=crtTk;
	if(consume(OR)){
		if(exprAnd()){
			if(exprOrPrim()){
				return true;
				}
			}
		crtTk=startTk;
		}
	return true;		// de la epsilon
	}
