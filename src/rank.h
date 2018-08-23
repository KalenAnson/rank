/*\
|*| rankConfig
|*| because
\*/
#ifndef RANK_H
#define RANK_H
/*\
|*| Rank Includes
\*/
#include <stdio.h>
/*\
|*| Rank macros
\*/
/*\
|*| Read lines upto x rank characters long
\*/
#define RK_MAX_LINE 1024
/*\
|*| Read upto x rank lines from a file
\*/
#define RK_MAX_LINES 1024
/*\
|*| Maximum rank value string length
\*/
#define RK_MAX_STRING 512
/*\
|*| Rank comments
\*/
#define RK_COMMENT_SC ';'
#define RK_COMMENT_LBS '#'
/*\
|*| Rank flags
\*/
#define RK_STRICT		0x1		/* Rank strictness, white space matters		*/
#define RK_NEW_LINES	0x2		/* Rank new line endings					*/
#define RK_CAR_RET		0x4		/* Rank carrage return endings				*/
#define RK_MULTILINE	0x8		/* Rank multiline values					*/
#define RK_BINARY		0x10	/* Rank binary values						*/
#define RK_SLOPPY		0x20	/* Rank slopy assignment parsing			*/
#define RK_SEMI			0x40	/* Rank semicolon comments					*/
#define RK_LBS			0x80	/* Rank pound comments						*/
#define RK_TYPES		0x100	/* Rank type strictness						*/
#define RK_MANDITORY	0x200	/* Rank manditory sections or values		*/
#define RK_DEBUG		0x400	/* Rank debug output requested				*/
/*\
|*| Rank errors
\*/
#define RK_E_STRICT		0x1		/* Rank strict parse failures				*/
#define RK_E_INVALID	0x2		/* Rank invalid characters					*/
#define RK_E_EMPTY		0x4		/* Rank empty lines							*/
#define RK_E_NOTSYMBOL	0x8		/* Rank other symbols (on a symbol search) 	*/
#define RK_E_MALLOC		0x10	/* Rank memory allocation errors 			*/
/*\
|*| Rank types
\*/
#define RK_STRING		0x1		/* Rank string type							*/
#define RK_BOOL			0x2		/* Rank boolean type						*/
#define RK_INTEGER		0x4		/* Rank integer type (int long)				*/
#define RK_FLOAT		0x8		/* Rank float type							*/
#define RK_ULONG		0x10	/* Rank unsigned long						*/
#define RK_NAME_SECT	0x20	/* Rank section name						*/
#define RK_NAME_PROP	0x40	/* Rank property name						*/
/*\
|*| Rank Structs
|*|
|*| Rank config struct, a top level configuration struct
\*/
struct rankC {
	short			valid;		/* Rank validity							*/
	struct rankS	*sect;		/* Rank sections linked list				*/
};
/*\
|*| Rank section struct
\*/
struct rankS {
	uint64_t 		id;			/* Rank section name hash					*/
	char 			*name;		/* The rank section name (malloc'd)			*/
	struct rankP	*prop;		/* The rank properties linked list			*/
	struct rankS	*next;		/* The next section in the linked list		*/
};
/*\
|*| Rank property struct
\*/
struct rankP {
	uint64_t 		id;			/* Rank section name hash					*/
	char 			*name;		/* The rank property name (malloc'd)		*/
	struct rankV	*val;		/* The rank value 							*/
	struct rankP	*next;		/* The next section in the linked list		*/
};
/*\
|*| Rank value struct
\*/
struct rankV {
	short			type;		/* The rank value type						*/
	char 			*str;		/* A pointer to a rank string (malloc'd)	*/
	union rankD		*data;		/* The rank union							*/
};
/*\
|*| Rank data union
\*/
union rankD {
	unsigned short	boolean;	/* A rank boolean							*/
	long			numb;		/* A rank long integer						*/
	float			flt;		/* A rank float								*/
	unsigned long	ul;			/* A rank unsigned long						*/
};
/*\
|*| Function used for setting the rank options
\*/
int rankOptions(int);
/*\
|*| Function to open the rank config file
\*/
int rankOpen(char *, FILE **);
/*\
|*| Function to parse the rank options
\*/
int rankParse(FILE *, struct rankC **);
/*\
|*| Function to hash a string to a rank ID
\*/
uint64_t rankHash(char *, int);
/*\
|*| Function to probe for a rank section, returns the pointer to the section
|*| if it exists or NULL if the section does not exist.
\*/
struct rankS *rankSectionE(struct rankC *, char *);
/*\
|*| Function to probe for a rank property
\*/
struct rankP *rankPropE(struct rankS *, char *);
/*\
|*| Function to retrieve a rank value
|*| Passes in a pointer for the expected value.
\*/
int rankGetValue(struct rankP *, void *);
/*\
|*| Function to free rank configuration data
\*/
int rankFree(struct rankC *);
/*\
|*| Function to find an initial character
\*/
char *findInitialChar(char *, ssize_t, char *);
/*\
|*| Function to parse a section heading
|*| Returns a pointer to the current section, if the pointer is NULL, then the
|*| attempt to parse the section failed.
\*/
struct rankS *rankParseSection(char *, struct rankC *);
/*\
|*| Function to parse a propterty declaration
|*| Returns non-zero on error
\*/
int rankParseProperty(char *, struct rankS *);
/*\
|*| Function to parse a property value
|*| Returns non-zero on error
\*/
int rankParseValue(char *, struct rankV *);
/*\
|*| Function to parse a valid rank section or property name from a line
|*| Returns non-zero on error
\*/
int rankParseName(char *, char *, char **, int);
/*\
|*| Function to parse a valid rank value string (must be quoted)
|*| Returns non-zero on error
\*/
int rankParseString(char *, struct rankV *);
/*\
|*| Function to parse a valid rank value string (must be quoted)
|*| Returns non-zero on error
\*/
int rankParseNumber(char *, struct rankV *);
/*\
|*| Function to search a string to the end and look for invalid data
\*/
int rankIsEmpty(char *);
/*\
|*| Function to walk a rank config and enumerate the configuration
\*/
int rankEnumerate(struct rankC *);
#endif /* RANK_H */
