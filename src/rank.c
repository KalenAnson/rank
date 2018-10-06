/*\
|*| rankConfig
\*/
/*\
|*| Rank System Includes
\*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <limits.h>
/*\
|*| Rank Project Includes
\*/
#include "rank.h"
/*\
|*| Rank options variable
\*/
int rk_opts = 0;
/*\
|*| Rank Implimentation
\*/
int
rankOptions(int opts)
{
	/*\
	|*| Set the internal options variable
	|*| For now there is little validation of options settings.
	\*/
	rk_opts = opts;
	return 0;
}
int
rankOpen(char *filename, FILE **cfh) {
	/*\
	|*| Open the rank config file
	\*/
	if (*cfh != NULL) {
		if (rk_opts & RK_DEBUG)
			printf("R_C\tInvalid config file pointer\n");
		return 1;
	}
	if ((*cfh = fopen(filename, "r")) == NULL) {
		return 1;
	}
	return 0;
}
int
rankParse(FILE *cfh, struct rankC **cfg)
{
	int error = 0;
	int rankLines = 0;
	int activeSection = 0;
	char *line = NULL;
	size_t linecap = 0;
	ssize_t bytes = 0;
	char sectionStart = 0x5B;
	char *initial = NULL;
	struct rankS *thisSection = NULL;
	struct rankS *newSection = NULL;
	/*\
	|*| Check the file handle and struct pointer
	\*/
	if (cfh == NULL) {
		if (rk_opts & RK_DEBUG)
			printf("R_C\tInvalid file handle\n");
		return 1;
	} else if (*cfg != NULL) {
		if (rk_opts & RK_DEBUG)
			printf("R_C\tInvalid config pointer\n");
		return 1;
	}
	/*\
	|*| Allocate the rank configuration struct
	\*/
	*cfg = malloc(sizeof (struct rankC) );
	if (*cfg == NULL) {
		return 1;
	}
	memset(*cfg, 0, sizeof (struct rankC) );
	/*\
	|*| Begin to pull rank lines of config and parse them into structs
	\*/
	while ((bytes = getline(&line, &linecap, cfh) ) > 0) {
		if (bytes > RK_MAX_LINE) {
			if (rk_opts & RK_DEBUG)
				printf("R_C\tRK_MAX_LINE exceeded\n");
			error = 1;
			goto loopClean;
		}
		rankLines++;
		/*\
		|*| If no rank sections have been located, then look for the beginning
		|*| of a section heading
		\*/
		if (activeSection == 0) {
			initial = findInitialChar(line, bytes, &sectionStart);
			/*\
			|*| If blank lines, comment or errors are encountered, we will keep
			|*| parsing, ignoring this line
			\*/
			if (initial == NULL)
				goto loopClean;
			/*\
			|*| Try parsing this line as a section heading
			\*/
			newSection = rankParseSection(initial, *cfg);
			if (newSection == NULL) {
				if (rk_opts & RK_DEBUG)
					printf("R_C\tInvalid section\n");
			} else {
				if (rk_opts & RK_DEBUG)
					printf("R_C\tNew section located [%s]\n", newSection->name);
				thisSection = newSection;
				activeSection = 1;
			}
		} else {
			/*\
			|*| Continue parsing the section until EOF or another seaction
			|*| header is reached
			|*|
			|*| Search for the first valid initial character, then evaluate it
			|*| to see if it is a section headding or a property name
			\*/
			initial = findInitialChar(line, bytes, NULL);
			if (initial == NULL) {
				/*\
				|*| If blank lines, comment or errors are encountered, we will keep
				|*| parsing, ignoring this line
				\*/
				goto loopClean;
			} else if (*initial == sectionStart) {
				/*\
				|*| This is a new section
				\*/
				newSection = rankParseSection(initial, *cfg);
				if (newSection == NULL) {
					if (rk_opts & RK_DEBUG)
						printf("R_C\tInvalid section\n");
				} else {
					if (rk_opts & RK_DEBUG)
						printf("R_C\tNew section located [%s]\n", newSection->name);
					thisSection = newSection;
				}
			} else {
				/*\
				|*| Treat this line as a property name
				\*/
				if (rankParseProperty(initial, thisSection) ) {
					if (rk_opts & RK_DEBUG)
						printf("R_C\tInvalid property\n");
				}
			}
		}
		/**********************************************************************/
		loopClean: /*************************************************loopClean*/
		/**********************************************************************/
		/*\
		|*| Release the rank line
		\*/
		if (line != NULL) {
			free(line);
			line = NULL;
			linecap = 0;
		}
		if (error) {
			break;
		}
	}
	/*\
	|*| Free the line pointer again here to avoid a leak
	\*/
	if (line != NULL) {
		free(line);
		line = NULL;
	}
	if (rk_opts & RK_DEBUG)
		printf("R_C\tLines parsed: %i\n", rankLines);
	return 0;
}
uint64_t rankHash(char *buff, int len)
{
	/*\
	|*| Modified djb2 hashing algorithm
	|*| Edited to consume a length specified buffer rather than a c string
	\*/
    uint64_t hash = 5381;
	for (int i = 0; i < len; i++) {
		hash = ((hash << 5) + hash) + buff[i];
	}
    return hash;
}
struct rankS
*rankSectionE(struct rankC *cfg, char *name)
{
	struct rankS *section = NULL;
	struct rankS *thisS = NULL;
	uint64_t nameHash = 0;
	int len = 0;
	/*\
	|*| Get the length of the requested section name
	\*/
	len = strnlen(name, RK_MAX_STRING);
	if (len) {
		/*\
		|*| Hash the name
		\*/
		nameHash = rankHash(name, len);
		/*\
		|*| Walk the sections
		\*/
		thisS = cfg->sect;
		while (thisS) {
			if (thisS->id == nameHash) {
				/*\
				|*| Found the section
				\*/
				if (rk_opts & RK_DEBUG)
					printf("R_C\tSection Located: [%s]\n", name);
				section = thisS;
				break;
			}
			thisS = thisS->next;
		}
	}
	return section;
}
struct rankP
*rankPropE(struct rankS *sect, char *name)
{
	struct rankP *property = NULL;
	struct rankP *thisP = NULL;
	uint64_t nameHash = 0;
	int len = 0;
	/*\
	|*| Get the length of the requested section name
	\*/
	len = strnlen(name, RK_MAX_STRING);
	if (len) {
		/*\
		|*| Hash the name
		\*/
		nameHash = rankHash(name, len);
		/*\
		|*| Walk the properties
		\*/
		thisP = sect->prop;
		while (thisP) {
			if (thisP->id == nameHash) {
				/*\
				|*| Found the property
				\*/
				if (rk_opts & RK_DEBUG)
					printf("R_C\tProperty Located: [%s]\n", name);
				property = thisP;
				break;
			}
			thisP = thisP->next;
		}
	}
	return property;
}
int
rankFree(struct rankC *cfg)
{
	struct rankS *thisS = NULL;
	struct rankP *thisP = NULL;
	struct rankS *nextS = NULL;
	struct rankP *nextP = NULL;
	if (cfg == NULL) {
		return 1;
	}
	thisS = cfg->sect;
	/*\
	|*| Iterate over the sections linked list
	\*/
	while (thisS) {
		/*\
		|*| Iterate over the properties linked list
		\*/
		thisP = thisS->prop;
		while (thisP) {
			if (thisP->val != NULL) {
				/*\
				|*| Free the value string
				\*/
				if (thisP->val->str != NULL) {
					free(thisP->val->str);
					thisP->val->str = NULL;
				}
				/*\
				|*| Free the value union
				\*/
				if (thisP->val->data != NULL) {
					free(thisP->val->data);
					thisP->val->data = NULL;
				}
				free(thisP->val);
				thisP->val = NULL;
			}
			nextP = thisP->next;
			if (thisP->name != NULL) {
				free(thisP->name);
				thisP->name = NULL;
			}
			free(thisP);
			thisP = NULL;
			thisP = nextP;
		}
		nextS = thisS->next;
		if (thisS->name != NULL) {
			free(thisS->name);
			thisS->name = NULL;
		}
		free(thisS);
		thisS = NULL;
		thisS = nextS;
	}
	/*\
	|*| @NOTE Caller must free the actual config struct
	\*/
	return 0;
}
/*\
|*| Helper function to find the first valid character
|*| Returns NULL if this line is a comment, contains only whitespace or is
|*| otherwise an invalid configuration line.
\*/
char *
findInitialChar(char *line, ssize_t length, char *symbol)
{
	char *thisChar = NULL;
	int error = 0;
	for (ssize_t i = 0; i < length; i++) {
		/*\
		|*| Optionally check for semicolon style comments
		\*/
		if (rk_opts & RK_SEMI) {
			if (line[i] == 0x3B)
				break;
		}
		/*\
		|*| Optionally check for pound style comments
		\*/
		if (rk_opts & RK_LBS) {
			if (line[i] == 0x23)
				break;
		}
		/*\
		|*| Symbol search mode
		|*| If this is a strict search, we expect the symbol to be the first
		|*| character on the line. Otherwise, valid whitespace is allowed.
		\*/
		if (symbol != NULL) {
			if (line[i] == *symbol) {
				thisChar = &line[i];
				break;
			} else if (line[i] == 0x0A ) {
				/*\
				|*| Reached the end of the line, so this was an empty line with
				|*| either no characters or valid whitespace.
				\*/
				error = RK_E_EMPTY;
				break;
			} else if (rk_opts & RK_STRICT) {
				/*\
				|*| This is a strict search, so bail
				\*/
				error = RK_E_STRICT;
				break;
			} else if (line[i] == 0x20|| line[i] == 0x09) {
				/*\
				|*| Non-strict, so pass on valid whitespace
				|*| 0x20 = Space
				|*| 0x09 = Horrizontal Tab
				\*/
				continue;
			} else {
				/*\
				|*| Found a character we were not expecting
				\*/
				error = RK_E_NOTSYMBOL;
				break;
			}
		} else {
			/*\
			|*| Character search mode
			|*| Strict seaching here will allow for leading whitespace, but will
			|*| fail on certain invalid characters.
			|*| Non-strict will let more valid ascii characters through.
			|*| Strict = [A-Z], [a-z], [, _, "
			|*| Non-strict = [A-Z], [a-z], [, _, ", [0-9.-], [spaces, tabs]
			|*|
			\*/
			if (line[i] == 0x0A ) {
				/*\
				|*| Reached the end of the line, so this was an empty line with
				|*| either no characters or valid whitespace.
				\*/
				error = RK_E_EMPTY;
				break;
			} else if (
				(line[i] >= 0x41 && line[i] <= 0x5A) ||
				(line[i] >= 0x61 && line[i] <= 0x7A) ||
				(line[i] == 0x5F) || (line[i] == 0x5B) || (line[i] == 0x22) ) {
				/*\
				|*| Found an ascii letter, number or valid character
				\*/
				thisChar = &line[i];
				break;
			} else if (rk_opts & RK_STRICT) {
				/*\
				|*| This is a strict search, so bail
				\*/
				error = RK_E_STRICT;
				break;
			} else if ((line[i] >= 0x30 && line[i] <= 0x39 ) ||
				line[i] == 0x2D ||
				line[i] == 0x2E) {
				/*\
				|*| Found a numeral, decimal or negative sign
				\*/
				thisChar = &line[i];
				break;
			} else if (line[i] == 0x20|| line[i] == 0x09) {
				/*\
				|*| Non-strict, so pass on valid whitespace
				|*| 0x20 = Space
				|*| 0x09 = Horrizontal Tab
				\*/
				continue;
			} else {
				/*\
				|*| Found a character we were not expecting
				\*/
				error = RK_E_INVALID;
				break;
			}
		}
	}
	if (error && rk_opts & RK_DEBUG) {
		printf("R_C\tfindInitialChar error [%i]\n",
			error);
	}
	return thisChar;
}
struct rankS *
rankParseSection(char *line, struct rankC *cfg)
{
	struct rankS *sect = NULL;
	struct rankS *next = NULL;
	char *ptr = NULL;
	char name[RK_MAX_STRING];
	int nameLen = 0;
	if (line == NULL || cfg == NULL) {
		if (rk_opts & RK_DEBUG)
			printf("R_C\tInvalid arguments for rankParseSection\n");
		goto cleanup;
	}
	/*\
	|*| Zero out the name buffer
	\*/
	memset(&name, 0, RK_MAX_STRING);
	/*\
	|*| Parse the name into the name buffer
	\*/
	if (rankParseName(line, &name[0], &ptr, RK_NAME_SECT) ) {
		if (rk_opts & RK_DEBUG)
			printf("R_C\tErrors parsing section\n");
		goto cleanup;
	} else {
		/*\
		|*| Create the section
		\*/
		nameLen = strlen(name);
		if (nameLen <= 0) {
			if (rk_opts & RK_DEBUG)
				printf("R_C\tInvalid name length\n");
			goto cleanup;
		}
		sect = malloc(sizeof (struct rankS) );
		if (sect == NULL) {
			if (rk_opts & RK_DEBUG)
				printf("R_C\tErrors allocating new section\n");
			goto cleanup;
		}
		memset(sect, 0, sizeof (struct rankS) );
		/*\
		|*| Allocate the name string
		\*/
		sect->name = malloc(sizeof (char) * (nameLen + 1) );
		if (sect->name == NULL) {
			if (rk_opts & RK_DEBUG)
				printf("R_C\tErrors allocating new section name\n");
			goto cleanup;
		}
		for (int i = 0; i < nameLen; i++) {
			sect->name[i] = name[i];
		}
		sect->name[nameLen] = 0x00;
		sect->id = rankHash(sect->name, nameLen);
		if (sect->id == 0) {
			if (rk_opts & RK_DEBUG)
				printf("R_C\tErrors hashing section name\n");
			goto cleanup;
		}
	}
	/*\
	|*| Attach the section to the config
	\*/
	if (cfg->sect == NULL) {
		/*\
		|*| This is the first section, so initalize both the section and the
		|*| overall config.
		\*/
		cfg->sect = sect;
		cfg->sect->prop = NULL;
		cfg->sect->next = NULL;
		cfg->valid = 1;
	} else {
		/*\
		|*| Find the end of the section list and insert this section
		\*/
		for (next = cfg->sect; next->next != NULL; next = next->next);
		next->next = sect;
	}
	return sect;
	/**************************************************************************/
	cleanup: /*********************************************************cleanup*/
	/**************************************************************************/
	if (sect != NULL) {													 /* * */
		if (sect->name != NULL) {										 /* * */
			free(sect->name);											 /* * */
			sect->name = NULL;											 /* * */
		}																 /* * */
		free(sect);														 /* * */
		sect = NULL;													 /* * */
	}																	 /* * */
	return NULL;														 /* * */
}
int
rankParseProperty(char *line, struct rankS *sect)
{
	int error = 0;
	struct rankP *prop = NULL;
	struct rankP *next = NULL;
	struct rankV *val = NULL;
	char *ptr = NULL;
	char name[RK_MAX_STRING];
	int nameLen = 0;
	if (line == NULL || sect == NULL) {
		if (rk_opts & RK_DEBUG)
			printf("R_C\tInvalid arguments for rankParseProperty\n");
		goto cleanup;
	}
	/*\
	|*| Zero out the name buffer
	\*/
	memset(&name, 0, RK_MAX_STRING);
	/*\
	|*| Parse the name into the name buffer
	\*/
	if (rankParseName(line, &name[0], &ptr, RK_NAME_PROP) ) {
		if (rk_opts & RK_DEBUG)
			printf("R_C\tErrors parsing property\n");
		goto cleanup;
	} else {
		/*\
		|*| Create the property
		\*/
		nameLen = strlen(name);
		if (nameLen <= 0) {
			if (rk_opts & RK_DEBUG)
				printf("R_C\tInvalid name length\n");
			goto cleanup;
		}
		prop = malloc(sizeof (struct rankP) );
		if (prop == NULL) {
			if (rk_opts & RK_DEBUG)
				printf("R_C\tErrors allocating new property\n");
			goto cleanup;
		}
		memset(prop, 0, sizeof (struct rankP) );
		/*\
		|*| Allocate the name string
		\*/
		prop->name = malloc(sizeof (char) * (nameLen + 1) );
		if (prop->name == NULL) {
			if (rk_opts & RK_DEBUG)
				printf("R_C\tErrors allocating new property name\n");
			goto cleanup;
		}
		for (int i = 0; i < nameLen; i++) {
			prop->name[i] = name[i];
		}
		prop->name[nameLen] = 0x00;
		if (rk_opts & RK_DEBUG)
			printf("R_C\tFound property [%s]\n",prop->name);
		prop->id = rankHash(prop->name, nameLen);
		if (sect->id == 0) {
			if (rk_opts & RK_DEBUG)
				printf("R_C\tErrors hashing property name\n");
			goto cleanup;
		}
	}
	/*\
	|*| Allocate the new value struct
	\*/
	val = malloc(sizeof (struct rankV) );
	if (val == NULL) {
		if (rk_opts & RK_DEBUG)
			printf("R_C\tErrors allocating new value\n");
		goto cleanup;
	}
	memset(val, 0, sizeof (struct rankV) );
	/*\
	|*| Now parse the value from the property
	\*/
	if (rankParseValue(ptr, val) ) {
		if (rk_opts & RK_DEBUG)
			printf("R_C\tErrors parsing property value\n");
		goto cleanup;
	} else {
		if (rk_opts & RK_DEBUG)
			printf("R_C\tParsed property value type [%i]\n", val->type);
		/*\
		|*| Attach the value struct to the property
		\*/
		prop->val = val;
	}
	/*\
	|*| Attach the property to the section
	\*/
	if (sect->prop == NULL) {
		/*\
		|*| This is the first property
		\*/
		sect->prop = prop;
		sect->prop->next = NULL;
	} else {
		/*\
		|*| Find the end of the property list and insert this property
		\*/
		for (next = sect->prop; next->next != NULL; next = next->next);
		next->next = prop;
	}
	return error;
	/**************************************************************************/
	cleanup: /*********************************************************cleanup*/
	/**************************************************************************/
	if (prop != NULL) {													 /* * */
		if (prop->name != NULL) {										 /* * */
			free(prop->name);											 /* * */
			prop->name = NULL;											 /* * */
		}																 /* * */
		free(prop);														 /* * */
		prop = NULL;													 /* * */
	}																	 /* * */
	if (val != NULL) {													 /* * */
		if (val->str != NULL) {											 /* * */
			free(val->str);												 /* * */
			val->str = NULL;											 /* * */
		}																 /* * */
		if (val->data != NULL) {										 /* * */
			free(val->data);											 /* * */
			val->data = NULL;											 /* * */
		}																 /* * */
		free(val);														 /* * */
		val = NULL;													 	 /* * */
	}																	 /* * */
	return error;														 /* * */
}
int
rankParseValue(char *line, struct rankV *val)
{
	int error = 0;
	ssize_t len = 0;
	char *start = NULL;
	/*\
	|*| Get the remaining string length
	\*/
	len = strlen(line);
	if (len < 1) {
		if (rk_opts & RK_DEBUG)
			printf("R_C\tInvalid value length\n");
		error = RK_E_INVALID;
		goto cleanup;
	}
	/*\
	|*| Get the first character in the value string
	\*/
	start = findInitialChar(line, len, NULL);
	if (start == NULL) {
		if (rk_opts & RK_DEBUG)
			printf("R_C\tValue string contained no valid data\n");
		error = RK_E_INVALID;
		goto cleanup;
	} else if (start[0] == 0x22) {
		/*\
		|*| Start of string value
		\*/
		if (rk_opts & RK_DEBUG)
			printf("R_C\tValue string quote start\n");
		if (rankParseString(start, val) ) {
			if (rk_opts & RK_DEBUG)
				printf("R_C\tUnable to parse rank string\n");
			error = RK_E_INVALID;
			goto cleanup;
		} else {
			if (rk_opts & RK_DEBUG)
				printf("R_C\tParsed string [%s] \n", val->str);
		}
	} else {
		/*\
		|*| Start of unquoted value
		|*| Try reading the value into increasingly larger value types, prefer-
		|*| ing integers over floats.
		\*/
		if (rk_opts & RK_DEBUG)
			printf("R_C\tValue string unquoted start\n");
		/*\
		|*| Now parse the number from the string
		\*/
		if (rankParseNumber(start, val) ) {
			if (rk_opts & RK_DEBUG)
				printf("R_C\tUnable to parse rank number\n");
			error = RK_E_INVALID;
			goto cleanup;
		} else {
			if (rk_opts & RK_DEBUG)
				printf("R_C\tParsed number\n");
		}
	}
	return error;
	/**************************************************************************/
	cleanup: /*********************************************************cleanup*/
	/**************************************************************************/
	// if (data != NULL) {
	// 	free(data);
	// 	data = NULL;
	// }
	return error;
}
int
rankParseName(char *line, char *name, char **ptr, int type)
{
	int error = 0;
	int pos = 0;
	int i = 0;
	/*\
	|*| Only try for a string name upto the RK_MAX_STRING - 1, otherwise this is
	|*| a bad string
	\*/
	for (i = 0; i < (RK_MAX_STRING - 1); i++) {
		if (line[i] == 0x0A || line[i] == 0x0D || line[i] == 0x00) {
			/*\
			|*| 'NL, CR, NULL'
			|*| End of name string, not good here
			\*/
			error = 1;
			if (rk_opts & RK_DEBUG)
				printf("R_C\tPremature line ending or null byte\n");
		} else if (line[i] == 0x5B) {
			/*\
			|*| '['
			|*| Start of section name
			\*/
			if (type == RK_NAME_SECT) {
				continue;
			} else {
				error = 1;
				if (rk_opts & RK_DEBUG)
					printf("R_C\tInvalid opening brace in property name\n");
			}
		}else if (line[i] == 0x5D) {
			/*\
			|*| ']'
			|*| End of section name, check if the name is greater than 0
			\*/
			if (type == RK_NAME_SECT) {
				if (pos > 0) {
					i++;
					name[i] = 0x00;
					break;
				} else {
					error = 1;
					if (rk_opts & RK_DEBUG)
						printf("R_C\tZero length section found\n");
				}
			} else {
				error = 1;
				if (rk_opts & RK_DEBUG)
					printf("R_C\tInvalid closing brace in property name\n");
			}
		} else if (line[i] == 0x3D) {
			/*\
			|*| '='
			|*| End of a property name, check if the name is greater than 0
			\*/
			if (type == RK_NAME_PROP) {
				if (pos > 0) {
					i++;
					name[i] = 0x00;
					*ptr = &line[i];
					break;
				} else {
					if (rk_opts & RK_DEBUG)
						printf("R_C\tZero length propery found\n");
					error = 1;
				}
			} else {
				error = 1;
				if (rk_opts & RK_DEBUG)
					printf("R_C\tInvalid equality operator in section name\n");
			}
		} else if ((line[i] >= 0x30 && line[i] <= 0x39) ||
			(line[i] >= 0x41 && line[i] <= 0x5A) ||
			(line[i] >= 0x61 && line[i] <= 0x7A) ||
			line[i] == 0x5F
		) {
			/*\
			|*| '[A-Z], [a-z], [0-9], _'
			\*/
			name[pos] = line[i];
			pos++;
		} else if (line[i] == 0x20|| line[i] == 0x09) {
			/*\
			|*| 'SPACE, TAB'
			|*| Only a problem on strict parsing
			\*/
			if (rk_opts & RK_STRICT) {
				error = 1;
				if (rk_opts & RK_DEBUG)
					printf("R_C\tInvalid white space in name\n");
			}
		} else {
			/*\
			|*| Some other binary cruft
			\*/
			error = 1;
			if (rk_opts & RK_DEBUG)
				printf("R_C\tRank cruft [0x%x] in name\n", line[i]);
		}
		if (error == 1) {
			break;
		}
	}
	/*\
	|*| See if we have located a valid string
	\*/
	if (error == 0 || pos > 0) {
		if (type == RK_NAME_SECT) {
			error = rankIsEmpty(&line[i]);
			if (error == 1) {
				if (rk_opts & RK_DEBUG)
					printf("R_C\tInvalid characters located after name\n");
			}
		}
	}
	return error;
}
int
rankParseString(char *line, struct rankV *val)
{
	int error = 0;
	int j = 0;
	char *string = NULL;
	short quoted = 0;
    short escaped = 0;
	/*\
	|*| Sanitize
	\*/
	if (line == NULL || val == NULL) {
		if (rk_opts & RK_DEBUG)
			printf("R_C\tInvalid parameters for parsing value string\n");
		error = RK_E_INVALID;
		goto cleanup;
	}
	/*\
	|*| Check the first character for a valid opening quote
	\*/
	if (*line != 0x22) {
		if (rk_opts & RK_DEBUG)
			printf("R_C\tInvalid rank value string\n");
		error = RK_E_INVALID;
		goto cleanup;
	} else {
		quoted = 1;
	}
	/*\
	|*| Begin copying the string into the rank struct
	\*/
	string = malloc(RK_MAX_STRING + 1);
	if (string == NULL) {
		if (rk_opts & RK_DEBUG)
			printf("R_C\tUnable to allocate new rank string\n");
		error = RK_E_MALLOC;
		goto cleanup;
	}
	memset(string, 0, sizeof (RK_MAX_STRING + 1) );
	/*\
	|*| Walk through the string and copy the contents into the rank struct
	\*/
	for (int i = 0; i < RK_MAX_STRING; i++) {
		j++;
		if (line[j] == 0x22) {
			/*\
			|*| Quote character, terminate and break
			\*/
			if (escaped == 1) {
				escaped = 0;
				string[i] = line[j];
			} else {
				if (quoted == 1) {
					quoted = 0;
					string[i] = 0x00;
				} else {
					if (rk_opts & RK_DEBUG)
						printf("R_C\tInvalid quote character in string\n");
					error = RK_E_INVALID;
				}
				break;
			}
		} else if (line[j] == 0x5C) {
			/*\
			|*| Backslash
			\*/
			if (escaped == 0) {
				escaped = 1;
				i = i - 1;
				continue;
			} else {
				string[i] = line[j];
				escaped = 0;
			}
		} else if (line[j] == 0x0A) {
			/*\
			|*| New lines are not permissible, even escaped
			\*/
			if (rk_opts & RK_DEBUG)
				printf("R_C\tInvalid new line in string\n");
			error = RK_E_INVALID;
			break;
		} else if (
			(line[j] >= 0x21 && line[j] <= 0x7E && line[j] != 0x5C) ) {
			/*\
			|*| Found an ascii letter, number or valid character, see ASCII
			|*| table for ranges.
			|*| [!-~] not inclusive of the backslash '\'
			\*/
			if (escaped)
				escaped = 0;
			string[i] = line[j];
		} else if (line[j] == 0x20|| line[j] == 0x09) {
			/*\
			|*| Spaces or tabs are fine
			\*/
			if (escaped)
				escaped = 0;
			string[i] = line[j];
		} else {
			if (rk_opts & RK_DEBUG)
				printf("R_C\tUnexpected character in string\n");
			error = RK_E_INVALID;
			break;
		}
	}
	/*\
	|*| Check for errors
	\*/
	if (error) {
		if (rk_opts & RK_DEBUG)
			printf("R_C\tString parsing encountered errors\n");
		goto cleanup;
	} else if (quoted || escaped) {
		if (rk_opts & RK_DEBUG)
			printf("R_C\tRank string parsing failed\n");
		error = RK_E_INVALID;
		goto cleanup;
	} else {
		/*\
		|*| See if entire string was parsed
		\*/
		if (line[j] != 0x00) {
			j++;
			if (rankIsEmpty(&line[j]) ) {
				if (rk_opts & RK_DEBUG)
					printf("R_C\tRank string parsing left behind rank cruft\n");
				error = RK_E_INVALID;
				goto cleanup;
			}
		}
	}
	/*\
	|*| Now update the rank struct
	\*/
	val->str = string;
	/*\
	|*| Set the value type
	\*/
	val->type = RK_STRING;
	return error;
	/**************************************************************************/
	cleanup: /*********************************************************cleanup*/
	/**************************************************************************/
	if (string != NULL) {												 /* * */
		free(string);													 /* * */
		string = NULL;													 /* * */
	}																	 /* * */
	return error;														 /* * */
}
int
rankParseNumber(char *line, struct rankV *val)
{
	int error = 0;
	union rankD *data = NULL;
	size_t len = 0;
	int decimal = -1;
	int negative = 0;
	int last = -1;
	int j = 0;
	char clean[RK_MAX_STRING];
	char *endptr = NULL;
	memset(&clean, 0, RK_MAX_STRING);
	/*\
	|*| Sanitize
	\*/
	if (line == NULL || val == NULL) {
		if (rk_opts & RK_DEBUG)
			printf("R_C\tInvalid parameters for parsing numbers\n");
		error = RK_E_INVALID;
		goto cleanup;
	}
	/*\
	|*| Decide how to handle this value
	|*| Parse the number in the following order of preference:
	|*| short, long, float, unsigned long
	|*|
	|*| Get the length of the string (may still include new lines or npcs
	\*/
	len = strnlen(line, RK_MAX_STRING);
	/*\
	|*| Walk the string and look for the last numeric character [0-9.] and the
	|*| presence of a decimal point]. Looking for contigious characters
	\*/
	for (size_t i = 0; i < len; i++) {
		/*\
		|*| Evaluate
		\*/
		if (line[i] == 0x2D) {
			/*\
			|*| Negative
			\*/
			if (last == -1 && negative == 0) {
				/*\
				|*| No numbers yet, life is good
				\*/
				negative = 1;
				clean[j++] = line[i];
				last = i;
			} else {
				error = RK_E_INVALID;
				break;
			}
		} else if (line[i] == 0x2E) {
			/*\
			|*| Decimals
			\*/
			if (decimal == -1) {
				decimal = i;
				clean[j++] = line[i];
				last = i;
			} else {
				error = RK_E_INVALID;
				break;
			}
		} else if (line[i] >= 0x30 && line[i] <= 0x39) {
			/*\
			|*| Valid numeric
			\*/
			clean[j++] = line[i];
			last = i;
		} else if (line[i] == 0x09 || line[i] == 0x20) {
			/*\
			|*| Valid npcs
			\*/
			if (last == -1) {
				/*\
				|*| No numbers yet, just skip the npcs
				\*/
				continue;
			} else {
				/*\
				|*| Hit whitespace after a number, we are done
				\*/
				last = i;
				break;
			}
		} else if (line[i] == 0x0A || line[i] == 0x0D) {
			/*\
			|*| End of line
			\*/
			if (last == -1) {
				/*\
				|*| No numbers in this string
				\*/
				error = RK_E_INVALID;
				break;
			} else {
				/*\
				|*| End of the line
				\*/
				last = i;
				break;
			}
		} else {
			/*\
			|*| Invalid cruft
			\*/
			error = RK_E_INVALID;
			break;
		}
	}
	/*\
	|*| Evaluate
	\*/
	if (error) {
		if (rk_opts & RK_DEBUG)
			printf("R_C\tParsing errors evaluating numeric data\n");
		goto cleanup;
	} else if (last == -1) {
		error = RK_E_INVALID;
		if (rk_opts & RK_DEBUG)
			printf("R_C\tEmpty string passed to numeric parser\n");
		goto cleanup;
	} else {
		/*\
		|*| Check for invalid cruft
		\*/
		if (rankIsEmpty(&line[last]) ) {
			if (rk_opts & RK_DEBUG)
				printf("R_C\tRank numeric parsing left behind rank cruft\n");
			error = RK_E_INVALID;
			goto cleanup;
		}
	}
	/*\
	|*| Allocate the data union
	\*/
	data = malloc(sizeof (union rankD) );
	if (data == NULL) {
		if (rk_opts & RK_DEBUG)
			printf("R_C\tUnable to allocate rank data\n");
		error = RK_E_MALLOC;
		goto cleanup;
	}
	memset(data, 0, sizeof (union rankD) );
	/*\
	|*| Now evaluate the number
	\*/
	errno = 0;
	len = strnlen(clean, RK_MAX_STRING);
	if (rk_opts & RK_DEBUG)
		printf("R_C\tClean number string length [%zu] [%s]\n", len, clean);
	if (decimal != -1) {
		/*\
		|*| There was a decimal point so parse this as a float
		\*/
		data->flt = strtof(clean, &endptr);
		if (data->flt == HUGE_VALF) {
			if (rk_opts & RK_DEBUG)
				printf("R_C\tRank numeric parsing failed\n");
			error = RK_E_INVALID;
			goto cleanup;
		} else if (data->flt == 0 && errno == ERANGE) {
			if (rk_opts & RK_DEBUG)
				printf("R_C\tRank numeric parsing failed miseraby\n");
			error = RK_E_INVALID;
			goto cleanup;
		} else if (*endptr != 0x00) {
			if (rk_opts & RK_DEBUG)
				printf("R_C\tRank numeric parsing failed badly\n");
			error = RK_E_INVALID;
			goto cleanup;
		} else {
			val->type = RK_FLOAT;
			val->data = data;
		}
	} else {
		/*\
		|*| Lets play find the rank number type!
		\*/
		if (len == 1 && (clean[0] == 0x30 || clean[0] == 0x31) ) {
			/*\
			|*| Will a bool work
			\*/
			if (clean[0] == 0x30) {
				data->boolean = 0;
				val->type = RK_BOOL;
				val->data = data;
			} else {
				data->boolean = 1;
				val->type = RK_BOOL;
				val->data = data;
			}
		} else if (clean[0] == 0x2D) {
			/*\
			|*| Signed it is!
			\*/
			data->numb = strtol(clean, &endptr, 10);
			if ((data->numb == LONG_MIN || data->numb == LONG_MAX) &&
				errno == ERANGE) {
				if (rk_opts & RK_DEBUG)
					printf("R_C\tRank numeric parsing failed\n");
				error = RK_E_INVALID;
				goto cleanup;
			} else if (data->numb == 0 && errno == ERANGE) {
				if (rk_opts & RK_DEBUG)
					printf("R_C\tRank numeric parsing failed miseraby\n");
				error = RK_E_INVALID;
				goto cleanup;
			} else if (*endptr != 0x00) {
				if (rk_opts & RK_DEBUG)
					printf("R_C\tRank numeric parsing failed badly\n");
				error = RK_E_INVALID;
				goto cleanup;
			} else {
				val->type = RK_INTEGER;
				val->data = data;
			}
		} else {
			/*\
			|*| Going for unsigned
			\*/
			data->ul = strtoul(clean, &endptr, 10);
			if (data->ul == ULONG_MAX && errno == ERANGE) {
				if (rk_opts & RK_DEBUG)
					printf("R_C\tRank numeric parsing failed\n");
				error = RK_E_INVALID;
				goto cleanup;
			} else if (data->ul == 0 && errno == ERANGE) {
				if (rk_opts & RK_DEBUG)
					printf("R_C\tRank numeric parsing failed miseraby\n");
				error = RK_E_INVALID;
				goto cleanup;
			} else if (*endptr != 0x00) {
				if (rk_opts & RK_DEBUG)
					printf("R_C\tRank numeric parsing failed badly\n");
				error = RK_E_INVALID;
				goto cleanup;
			} else {
				val->type = RK_ULONG;
				val->data = data;
			}
		}
	}
	return error;
	/**************************************************************************/
	cleanup: /*********************************************************cleanup*/
	/**************************************************************************/
	if (data != NULL) {													 /* * */
		free(data);														 /* * */
		data = NULL;													 /* * */
	}																	 /* * */
	return error;														 /* * */
}
int
rankIsEmpty(char *line) {
	int error = 0;
	int i = -1;
	/*\
	|*| Examine the string and search for invalid data
	\*/
	while (i++ >= -1) {
		if (line[i] == 0x0A) {
			if (! (rk_opts & RK_NEW_LINES) ) {
				error = 1;
			}
		} else if (line[i] == 0x0D) {
			if (! (rk_opts & RK_CAR_RET) ) {
				error = 1;
			}
		} else if (line[i] == 0x20 || line[i] == 0x09) {
			/*\
			|*| 'SPACE, TAB'
			|*| Not a problem, even for strict parsing
			\*/
			continue;
		} else if (line[i] == 0x23) {
			/*\
			|*| '#'
			|*| Inline comment
			\*/
			if (rk_opts & RK_LBS) {
				break;
			} else {
				error = 1;
			}
		} else if (line[i] == 0x3B) {
			/*\
			|*| ';'
			|*| Inline comment
			\*/
			if (rk_opts & RK_SEMI) {
				break;
			} else {
				error = 1;
			}
		} else if (line[i] == 0x00) {
			/*\
			|*| Reached the end of the line with no problems
			\*/
			break;
		} else {
			/*\
			|*| Hit another character after we were done with the parsing
			|*| Signal an error
			\*/
			error = 1;
		}
		/*\
		|*| Check for errors
		\*/
		if (error == 1) {
			break;
		}
	}
	return error;
}
int
rankEnumerate(struct rankC *cfg)
{
	int error = 0;
	struct rankS *ss = NULL;
	struct rankP *ps = NULL;
	if (cfg == NULL || cfg->valid != 1) {
		if (rk_opts & RK_DEBUG)
			printf("R_C\tInvalid configuration enumeration\n");
		return 1;
	}
	/*\
	|*| Print the configuration to stdout in a human readable fashion
	\*/
	printf("Rank Config ***************************************************\n");
	ss = cfg->sect;
	do {
		printf("->\t[%s]\n", ss->name);
		ps = ss->prop;
		while (ps) {
			if (ps->val) {
				switch (ps->val->type) {
					case RK_STRING:
						printf("---->\tS\t[%s] : [%s]\n", ps->name, ps->val->str);
						break;
					case RK_BOOL:
						printf("---->\tB\t[%s] : [%i]\n", ps->name,
							ps->val->data->boolean);
						break;
					case RK_INTEGER:
						printf("---->\tI\t[%s] : [%li]\n", ps->name,
							ps->val->data->numb);
						break;
					case RK_FLOAT:
						printf("---->\tF\t[%s] : [%f]\n", ps->name,
							ps->val->data->flt);
						break;
					case RK_ULONG:
						printf("---->\tUL\t[%s] : [%lu]\n", ps->name,
							ps->val->data->ul);
						break;
					default:
						printf("---->\tNULL\t[%s] : [NULL]\n", ps->name);
				}
			}
			/*\
			|*| Increment the property pointer
			\*/
			ps = ps->next;
		}
		/*\
		|*| Increment the section pointer
		\*/
		ss = ss->next;
	} while (ss);
	printf("End Rank Config ***********************************************\n");
	return error;
}
