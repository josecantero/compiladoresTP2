/* analizador lexico */
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<ctype.h>
/***************** MACROS **********************/


#define VAR			256  
#define PR_IF		257  
#define THEN		258  
#define PR_ELSE		259  
#define PR_FOR		260  
#define TO			261  
#define STEP		262  
#define PR_DO		263  
#define END			264  
#define PR_ALERT    265  
#define NUM			266  
#define ID			267  
#define LITERAL		268  
#define OP_ASIGNA	269  
#define OP_REL		270  
#define OP_SUMA		271  
#define OP_MULT		272  
#define OP_LOGIC   273  
#define ST_WRITELN  274 

#define TAMBUFF 	5
#define TAMLEX 		50
#define TAMHASH 	101

/*******************Definiciones***********************/
typedef enum {TRUE, FALSE} Error;

typedef struct entrada{
	
	int compLex;
	char lexema[TAMLEX];	
	struct entrada *tipoDato; 
	
} entrada;

typedef struct {
	int compLex;
	entrada *pe; 
} token;


typedef enum 
    
   {ENDFILE,ERROR,
    
    VAR_,IF,THEN_,ELSE_,END_,FOR_,TO_,STEP_,DO_,WRITE,

    ID_,NUM_,

    ASSIGN,EQ,LT,PLUS,MINUS,TIMES,OVER,LPAREN,RPAREN,SEMI
   } TokenType;
   
typedef enum {Stmtk,ExpK} NodeKind; 
typedef enum {IfK, RepeatK, AssignK, WriteK} StmtKind;
typedef enum {OpK, ConstK, IdK} ExpKind;
typedef enum {Void, Integer, Boolean} ExpType;
#define MAXCHILDREN 3

typedef struct treeNode
{
    struct treeNode * child [MAXCHILDREN];
    struct treeNode * sibling;
    
    int lineno;
    NodeKind nodekind;
    union {StmtKind stmt; ExpKind exp;}kind;
    union {TokenType op; 
          int val;
          char * name; }attr;
    ExpType type; 
} TreeNode;

/************* Variables globales **************/
int consumir;			
						
char cad[5*TAMLEX];		
token t;				

// variables para el analizador lexico
FILE *archivo;			
char buff[2*TAMBUFF];	
char id[TAMLEX];		
int delantero=-1;		
int fin=0;				
int numLinea=1;			
Error HayError= FALSE;

/************** Prototipos *********************/
void sigLex();		

/*********************HASH************************/
entrada *tabla;				
int tamTabla=TAMHASH;		
int elems=0;				

int h(char* k, int m)
{
	unsigned h=0,g;
	int i;
	for (i=0;i<strlen(k);i++)
	{
		h=(h << 4) + k[i];
		if (g=h&0xf0000000){
			h=h^(g>>24);
			h=h^g;
		}
	}
	return h%m;
}
void insertar(entrada e);

void initTabla()
{	
	int i=0;
	
	tabla=(entrada*)malloc(tamTabla*sizeof(entrada));
	for(i=0;i<tamTabla;i++)
	{
		tabla[i].compLex=-1;
	}
}

int esprimo(int n)
{
	int i;
	for(i=3;i*i<=n;i+=2)
		if (n%i==0)
			return 0;
	return 1;
}

int siguiente_primo(int n)
{
	if (n%2==0)
		n++;
	for (;!esprimo(n);n+=2);

	return n;
}


void rehash()
{
	entrada *vieja;
	int i;
	vieja=tabla;
	tamTabla=siguiente_primo(2*tamTabla);
	initTabla();
	for (i=0;i<tamTabla/2;i++)
	{
		if(vieja[i].compLex!=-1)
			insertar(vieja[i]);
	}		
	free(vieja);
}


void insertar(entrada e)
{
	int pos;
	if (++elems>=tamTabla/2)
		rehash();
	pos=h(e.lexema,tamTabla);
	while (tabla[pos].compLex!=-1)
	{
		pos++;
		if (pos==tamTabla)
			pos=0;
	}
	tabla[pos]=e;

}

entrada* buscar(char *clave)
{
	int pos;
	entrada *e;
	pos=h(clave,tamTabla);
	while(tabla[pos].compLex!=-1 && stricmp(tabla[pos].lexema,clave)!=0 )
	{
		pos++;
		if (pos==tamTabla)
			pos=0;
	}
	return &tabla[pos];
}

void insertTablaSimbolos(char *s, int n)
{
	entrada e;
	sprintf(e.lexema,s);
	e.compLex=n;
	insertar(e);
}

void initTablaSimbolos()
{
	int i;
	entrada pr,*e;
	char *vector[]={
	"var",
    "if", 
    "then", 
    "else",
	"for", 
    "to", 
    "step", 
    "do",
    "end",
    "alert"	
  	};
 	for (i=0;i<10;i++)
	{
		insertTablaSimbolos(vector[i],i+256);
	}
    insertTablaSimbolos(",",',');
	insertTablaSimbolos(".",'.');
	insertTablaSimbolos(";",';');
	insertTablaSimbolos("(",'(');
	insertTablaSimbolos(")",')');
	insertTablaSimbolos("[",'[');
	insertTablaSimbolos("]",']');
	insertTablaSimbolos("<",OP_REL);
	insertTablaSimbolos("<=",OP_REL);
	insertTablaSimbolos("<>",OP_REL);
	insertTablaSimbolos("==",OP_REL);
	insertTablaSimbolos(">",OP_REL);
	insertTablaSimbolos(">=",OP_REL);
	insertTablaSimbolos("=",OP_ASIGNA);
	insertTablaSimbolos("+",OP_SUMA);
	insertTablaSimbolos("-",OP_SUMA);
	insertTablaSimbolos("or",OP_SUMA);
	insertTablaSimbolos("*",OP_MULT);
	insertTablaSimbolos("/",OP_MULT);
	insertTablaSimbolos("and",OP_LOGIC);
	insertTablaSimbolos("or",OP_LOGIC);
 	insertTablaSimbolos("writeln",ST_WRITELN);
 

}


void error(char* mensaje)
{
	printf("Lin %d: %s==>'%s'\n",numLinea,mensaje,t.pe->lexema);
    HayError=TRUE;	
}

void sigLex()
{
	int i=0, longid=0;
	char c=0;
	int acepto=0;
	int estado=0;
	char msg[41];
	entrada e;

	while((c=fgetc(archivo))!=EOF)
	{
		
		if (c==' ' || c=='\t')
			continue;
			
		else if(c=='\n')
		{
			
			numLinea++;
			continue;
		}
		else if (isalpha(c))
		{
			
			i=0;
			do{
				id[i]=c;
				i++;
				c=fgetc(archivo);
				if (i>=TAMLEX)
					error("La longitud de Identificador excede el tamaño de buffer");
			}while(isalpha(c) || isdigit(c));
			id[i]='\0';
			if (c!=EOF)
				ungetc(c,archivo);
			else
				c=0;
			if(id[0]=='i' && id[1]=='f' && id[2]=='\0'){
				t.pe=buscar(id);
				t.compLex=t.pe->compLex;
				if (t.pe->compLex==-1)
				{
					sprintf(e.lexema,id);
					e.compLex=PR_IF;
					insertar(e);
					t.pe=buscar(id);
					//sprintf(
					//t.compLex,"PR_IF"
					t.compLex=PR_IF;
					//);
				}
			}
			else if(id[0]=='a' && id[1]=='l' && id[2]=='e' && id[3]=='r' && id[4]=='t' && id[5]=='\0'){
				t.pe=buscar(id);
				t.compLex=t.pe->compLex;
				if (t.pe->compLex==-1)
				{
					sprintf(e.lexema,id);
					e.compLex=PR_ALERT;
					insertar(e);
					t.pe=buscar(id);
					//sprintf(
					//t.compLex,"PR_ALERT"
					t.compLex=PR_ALERT;
					//);
				}
			}
			else{
				t.pe=buscar(id);
				t.compLex=t.pe->compLex;
				if (t.pe->compLex==-1)
				{
					sprintf(e.lexema,id);
					e.compLex=ID;
					insertar(e);
					t.pe=buscar(id);
					t.compLex=ID;
				}
				/*sprintf(e.lexema,id);
				e.compLex=ID;
				insertar(e);
				t.pe=buscar(id);
				sprintf(t.compLex,"ID");*/
			}
			
			/*t.pe=buscar(id);
			t.compLex=t.pe->compLex;
			if (t.pe->compLex==-1)
			{
				sprintf(e.lexema,id);
				e.compLex=ID;
				insertar(e);
				t.pe=buscar(id);
				t.compLex=ID;
			}*/
			break;
		}
		else if (isdigit(c))
		{
				
				i=0;
				estado=0;
				acepto=0;
				id[i]=c;
				
				while(!acepto)
				{
					switch(estado){
					case 0: 
						c=fgetc(archivo);
						if (isdigit(c))
						{
							id[++i]=c;
							estado=0;
						}
						else if(c=='.'){
							id[++i]=c;
							estado=1;
						}
						else if(tolower(c)=='e'){
							id[++i]=c;
							estado=3;
						}
						else{ 
							estado=6;
						}
						break;
					
					case 1:
						c=fgetc(archivo);						
						if (isdigit(c))
						{
							id[++i]=c;
							estado=2;
						}
						else if(c=='.') 
						{
							i--;
							fseek(archivo,-1,SEEK_CUR);
							estado=6;
						}
						else{
							sprintf(msg,"No se esperaba '%c'",c);
							estado=-1;
						}
						break;
					case 2:
						c=fgetc(archivo);
						if (isdigit(c))
						{
							id[++i]=c;
							estado=2;
						}
						else if(tolower(c)=='e')
						{
							id[++i]=c;
							estado=3;
						}
						else
							estado=6;
						break;
					case 3:
						c=fgetc(archivo);
						if (c=='+' || c=='-')
						{
							id[++i]=c;
							estado=4;
						}
						else if(isdigit(c))
						{
							id[++i]=c;
							estado=5;
						}
						else{
							sprintf(msg,"No se esperaba '%c'",c);
							estado=-1;
						}
						break;
					case 4:
						c=fgetc(archivo);
						if (isdigit(c))
						{
							id[++i]=c;
							estado=5;
						}
						else{
							sprintf(msg,"No se esperaba '%c'",c);
							estado=-1;
						}
						break;
					case 5:
						c=fgetc(archivo);
						if (isdigit(c))
						{
							id[++i]=c;
							estado=5;
						}
						else{
							estado=6;
						}break;
					case 6:
						if (c!=EOF)
							ungetc(c,archivo);
						else
							c=0;
						id[++i]='\0';
						acepto=1;
						t.pe=buscar(id);
						if (t.pe->compLex==-1)
						{
							sprintf(e.lexema,id);
							e.compLex=NUM;
							insertar(e);
							t.pe=buscar(id);
						}
						t.compLex=NUM;
						break;
					case -1:
						if (c==EOF)
							error("No se esperaba el fin de archivo");
						else
							error(msg);
						exit(1);
					}
				}
			break;
		}
		else if (c=='<') 
		{
			
			c=fgetc(archivo);
			if (c=='>'){
				t.compLex=OP_REL;
				t.pe=buscar("<>");
			}
			else if (c=='='){
				t.compLex=OP_REL;
				t.pe=buscar("<=");
			}
			else{
				ungetc(c,archivo);
				t.compLex=OP_REL;
				t.pe=buscar("<");
			}
			break;
		}
		else if (c=='>')
		{
			
				c=fgetc(archivo);
			if (c=='='){
				t.compLex=OP_REL;
				t.pe=buscar(">=");
			}
			else{
				ungetc(c,archivo);
				t.compLex=OP_REL;
				t.pe=buscar(">");
			}
			break;
		}
		else if (c=='=')
		{
		
			c=fgetc(archivo);
			if (c=='='){ 
				t.compLex=OP_REL;
				t.pe=buscar("==");
			}
			else{
				ungetc(c,archivo);
				t.compLex=OP_ASIGNA;
	     		t.pe=buscar("=");
			}
			break;
		}
		else if (c=='+')
		{
			t.compLex=OP_SUMA;
			t.pe=buscar("+");
			break;
		}
		else if (c=='-')
		{
			t.compLex=OP_SUMA;
			t.pe=buscar("-");
			break;
		}
		else if (c=='*')
		{
			t.compLex=OP_MULT;
			t.pe=buscar("*");
			break;
		}
		else if (c=='/')
		{
		  		   
          c=fgetc(archivo);	
            if (c=='/') 
            {
               while(c!=EOF)
			   {
                     c=fgetc(archivo); 
                     if(c=='\n'){
                       ungetc(c,archivo);    
                       break; 
                     }        
                }    
                            
            }
            else{
               ungetc(c,archivo); 	
               t.compLex=OP_MULT;
			   t.pe=buscar("/");
               break;
			   
			}
		}
		else if (c=='=')
		{
			t.compLex=OP_ASIGNA;
			t.pe=buscar("=");
			break;
		}
		else if (c==',')
		{
			t.compLex=',';
			t.pe=buscar(",");
			break;
		}
		else if (c==';')
		{
			t.compLex=';';
			t.pe=buscar(";");
			break;
		}
		else if (c=='.')
		{
			t.compLex='.';
			t.pe=buscar(".");
			break;
		}
		else if (c=='(')
		{
			if ((c=fgetc(archivo))=='*')
			{
				while(c!=EOF)
				{
					c=fgetc(archivo);
					if (c=='*')
					{
						if ((c=fgetc(archivo))==')')
						{
							break;
						}
						ungetc(c,archivo);
					}
				}
				if (c==EOF)
					error("Se llego al fin de archivo sin finalizar un comentario");
				continue;
			}
			else
			{
				ungetc(c,archivo);
				t.compLex='(';
				t.pe=buscar("(");
			}
			break;
		}
		else if (c==')')
		{
			t.compLex=')';
			t.pe=buscar(")");
			break;
		}
		else if (c=='[')
		{
			t.compLex='[';
			t.pe=buscar("[");
			break;
		}
		else if (c==']')
		{
			t.compLex=']';
			t.pe=buscar("]");
			break;
		}
		else if (c=='"')
		{
			i=0;
			id[i]=c;
			i++;
			do{
				c=fgetc(archivo);
				if (c=='"')
				{
					c=fgetc(archivo);
					if (c=='"')
					{
						id[i]=c;
						i++;
						id[i]=c;
						i++;
					}
					else
					{
						id[i]='"';
						i++;
						break;
					}
				}
				else if(c==EOF)
				{
					error("Se llego al fin de archivo sin finalizar un literal");
				}
				else{
					id[i]=c;
					i++;
				}
			}while(isascii(c));
			id[i]='\0';
			if (c!=EOF)
				ungetc(c,archivo);
			else
				c=0;
			t.pe=buscar(id);
			t.compLex=t.pe->compLex;
			if (t.pe->compLex==-1)
			{
				sprintf(e.lexema,id);
				e.compLex=LITERAL;
				insertar(e);
				t.pe=buscar(id);
				t.compLex=e.compLex;
			}
			break;
		}
	   	
       else if (c=='/')
		{
         			
           c=fgetc(archivo);	
            if (c=='/') break;
             
			if (c==EOF)
				error("Se llego al fin de archivo sin finalizar un comentario");
		}
		else if (c=='#')
		{
			//elimina el comentario
			//sprintf(t.compLex,"");
			while(c!=EOF)
			{
				c=fgetc(archivo);
				if (c=='\n'){
					ungetc(c,archivo);
					break;
                }
					
			}
			if (c==EOF)
				error("Se llego al fin de archivo sin finalizar un comentario");
			break;
		}
		else if (c!=EOF)
		{
			sprintf(msg,"%c no esperado",c);
			error(msg);
		}
	}
	if (c==EOF)
	{
		t.compLex=EOF;
		sprintf(e.lexema,"EOF");
		t.pe=&e;
	}
	
}

