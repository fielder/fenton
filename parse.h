#ifndef __PARSE_H__
#define __PARSE_H__

extern int
Parse_Begin (const char *text);

extern void
Parse_End (void);

extern int
Parse_Token (void);

extern int
Parse_Remaining (void);

enum
{
	PARSE_ERR_NONE,
	PARSE_ERR_INVAL,	/* invalid argument */
	PARSE_ERR_LONGTOKEN,	/* token is too long */
	PARSE_ERR_UNTERM,	/* unterminated token */
	PARSE_ERR_ESCAPECHAR	/* invalid escape sequence */
};

extern int parse_err;
extern char *parse_token;

#endif /* __PARSE_H__ */
