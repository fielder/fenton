#include <string.h>

#include "parse.h"

static char _token[1024];
static char *tok_nullchar = NULL;
static const char *p_ptr = NULL;

int parse_err = PARSE_ERR_NONE;
char *parse_token = _token;


static int
AppendSegment (const char *seg, int seglen)
{
	if (tok_nullchar + seglen >= _token + sizeof(_token))
	{
		parse_err = PARSE_ERR_LONGTOKEN;
		return 0;
	}

	memcpy (tok_nullchar, seg, seglen);
	tok_nullchar[seglen] = '\0';
	while (*tok_nullchar != '\0')
		tok_nullchar++;

	return 1;
}


static int
ParseQuotedToken (void)
{
#if 0
	const char *start;
	char escaped[1];

	/* skip beginning quote */
	p_ptr++;

	for (;;)
	{
		start = p_ptr;
		while (*p_ptr != '\0' && *p_ptr != '\\' && *p_ptr != '\"')
			p_ptr++;

		if (*p_ptr == '\0')
		{
			parse_err = PARSE_ERR_UNTERM;
			return 0;
		}

		if (!AppendSegment (start, p_ptr - start))
			return 0;

		if (*p_ptr == '\"')
			break;

		/* escaped char */
		p_ptr++;
		switch (*p_ptr)
		{
			case '\0':
				parse_err = PARSE_ERR_ESCAPECHAR;
				return 0;
			case 'r':	*escaped = '\r';	break;
			case 'n':	*escaped = '\n';	break;
			case 't':	*escaped = '\t';	break;
			default:	*escaped = *p_ptr;	break;
//TODO: python \x23 style binary characters
		}
		if (!AppendSegment (escaped, 1))
			return 0;
		p_ptr++;
	}

	/* skip ending quote */
	p_ptr++;

	return 1;
#endif

	return 0;
}


static int
ParseRegularToken (void)
{
	const char *start = p_ptr;

	while (*p_ptr != '\0' && *p_ptr > ' ')
		p_ptr++;

	return AppendSegment (start, p_ptr - start);
}


static int
ParseRemaining (void)
{
	const char *start = p_ptr;

	while (*p_ptr != '\0')
		p_ptr++;

	return AppendSegment (start, p_ptr - start);
}


static int
ParseTokenPrep (void)
{
	parse_err = PARSE_ERR_NONE;
	tok_nullchar = _token;
	*tok_nullchar = '\0';

	if (p_ptr == NULL)
	{
		parse_err = PARSE_ERR_INVAL;
		return 0;
	}

	while (*p_ptr != '\0' && *p_ptr <= ' ')
		p_ptr++;

	return 1;
}


int
Parse_Begin (const char *text)
{
	if (text == NULL)
	{
		parse_err = PARSE_ERR_INVAL;
		return 0;
	}

	parse_err = PARSE_ERR_NONE;
	tok_nullchar = _token;
	*tok_nullchar = '\0';
	p_ptr = text;

	return 1;
}


void
Parse_End (void)
{
	parse_err = PARSE_ERR_NONE;
	tok_nullchar = _token;
	*tok_nullchar = '\0';
	p_ptr = NULL;
}


int
Parse_Token (void)
{
	if (!ParseTokenPrep ())
		return 0;

	if (*p_ptr != '\0')
	{
		if (*p_ptr == '\"')
			return ParseQuotedToken ();
		else
			return ParseRegularToken ();
	}

	/* EOF */

	return 0;
}


int
Parse_Remaining (void)
{
	if (!ParseTokenPrep ())
		return 0;

	if (*p_ptr != '\0')
		return ParseRemaining ();

	/* EOF */

	return 0;
}
