#ifndef __JSMN_H_
#define __JSMN_H_

#include <stddef.h>
#include "project_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_JSMN_PARENT_LINKS
#ifndef JSMN_PARENT_LINKS
#define JSMN_PARENT_LINKS
#endif /* JSMN_PARENT_LINKS */
#endif /* CONFIG_JSMN_PARENT_LINKS */

#ifdef CONFIG_JSMN_STRICT
#ifndef JSMN_STRICT
#define JSMN_STRICT
#endif /* JSMN_STRICT */
#endif /* CONFIG_JSMN_STRICT */

#ifdef CONFIG_JSMN_SHORT_TOKENS
#ifndef JSMN_SHORT_TOKENS
#define JSMN_SHORT_TOKENS
#endif /* JSMN_SHORT_TOKENS */
#endif /* CONFIG_JSMN_SHORT_TOKENS */

/**
 * JSON type identifier. Basic types are:
 * 	o Object
 * 	o Array
 * 	o String
 * 	o Other primitive: number, boolean (true/false) or null
 */
typedef enum {
	JSMN_UNDEFINED = 0,
	JSMN_OBJECT = 1,
	JSMN_ARRAY = 2,
	JSMN_STRING = 3,
	JSMN_PRIMITIVE = 4
} jsmntype_t;

enum jsmnerr {
	/* Not enough tokens were provided */
	JSMN_ERROR_NOMEM = -1,
	/* Invalid character inside JSON string */
	JSMN_ERROR_INVAL = -2,
	/* The string is not a full JSON packet, more bytes expected */
	JSMN_ERROR_PART = -3
};

#ifdef JSMN_SHORT_TOKENS
	typedef short jsmnindex_t;
	typedef unsigned char jsmnenumtype_t;
#else
	typedef int jsmnindex_t;
	typedef jsmntype_t jsmnenumtype_t;
#endif

/**
 * JSON token description.
 * @param		type	type (object, array, string etc.)
 * @param		start	start position in JSON data string
 * @param		end		end position in JSON data string
 */
typedef struct {
	jsmnenumtype_t type;
	jsmnindex_t start;
	jsmnindex_t end;
	jsmnindex_t size;
	#ifdef JSMN_PARENT_LINKS
	jsmnindex_t parent;
	#endif
} jsmntok_t;

/**
 * JSON parser. Contains an array of token blocks available. Also stores
 * the string being parsed now and current position in that string
 */
typedef struct {
	unsigned int pos; /* offset in the JSON string */
	unsigned int toknext; /* next token to allocate */
	int toksuper; /* superior token node, e.g parent object or array */
} jsmn_parser;

/**
 * Create JSON parser over an array of tokens
 */
void jsmn_init(jsmn_parser *parser);

/**
 * Run JSON parser. It parses a JSON data string into and array of tokens, each describing
 * a single JSON object.
 */
int jsmn_parse(jsmn_parser *parser, const char *js, size_t len,jsmntok_t *tokens, unsigned int num_tokens);

#ifdef __cplusplus
}
#endif

#endif /* __JSMN_H_ */
