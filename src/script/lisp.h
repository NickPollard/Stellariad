// lisp.h
#pragma once

#define DEBUG_PARSE 0
#define DEBUG_CONTEXT 0
//#define DEBUG_LISP_STACK
//#define DEBUG_LISP_VERBOSE

#if DEBUG_PARSE
#define PARSE_PRINT( ... ) printf( __VA_ARGS__ )
#else
#define PARSE_PRINT( ... ) 
#endif

#if DEBUG_CONTEXT
#define CONTEXT_PRINT( ... ) printf( __VA_ARGS__ )
#else
#define CONTEXT_PRINT( ... ) 
#endif



/*
   types of lisp term
   all terms aside from LIST and ATOM are considered to be value terms
   */
enum termType {
	typeList,
	typeAtom,
	typeString,
	typeFloat,
	typeInt,
	typeVector,
	typeIntrinsic,
	typeMacro,
	typeObject,
	typeFalse,		// Special FALSE datatype
	typeTrue		// Special TRUE datatype
	};

/*
	A lisp term. Has a type to define what variable is stored in HEAD;
	If it's a list, then TAIL stores the next item in the list
	If it's not a list, then TAIL is undefined (should not be used; likely NULL)
   */
struct term_s;
typedef struct term_s term;

struct term_s {
	enum termType type;
	union {
		term* head;
		void* data;
		char* string;
		float* number;
		int*	integer;
	};
	term* tail;
	int refcount;
	};

/*
   Execution context
   */
typedef struct context_s {
	struct context_s* parent;
	map* lookup;
	} context;

extern context* lisp_global_context;

// Evaluate a lisp expression
term* eval( term* expr, void* _context );

// Return a new lisp execution context
context* lisp_newContext();
void context_delete( context* c );

// Parse the contents of [filename] as lisp code
term* lisp_parse_file( const char* filename );
term* lisp_eval_file( context* c, const char* filename );

// Init the lisp subsystem
void lisp_init();

// Take and hold a reference to term [t]
void term_takeRef( term* t );

// Release a held reference to term [t]
void term_deref( term* t );

term* head( term* list );
term* tail( term* list );

// Return the length of the given list T
int list_length( term* t );
#ifdef UNIT_TEST
void test_lisp();
#endif
