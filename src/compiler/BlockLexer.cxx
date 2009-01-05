/**************************************************************************
 *
 * Copyright (C) 2008, Johns Hopkins University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 *   - Redistributions of source code must contain the above
 *     copyright notice, this list of conditions, and the following
 *     disclaimer.
 *
 *   - Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions, and the following
 *     disclaimer in the documentation and/or other materials
 *     provided with the distribution.
 *
 *   - Neither the names of the copyright holders nor the names of any
 *     of any contributors may be used to endorse or promote products
 *     derived from this software without specific prior written
 *     permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **************************************************************************/

#include <assert.h>
#include <string.h>
#include <string>

#include <unicode/uchar.h>

#include <libsherpa/utf8.hxx>
#include <libsherpa/LexLoc.hxx>
#include <libsherpa/utf8.hxx>

#include "BUILD/BlockParser.hxx"

using namespace sherpa;

#include "BlockLexer.hxx"

static bool
valid_char_printable(uint32_t ucs4)
{
  if (strchr("!#$%&`()*+-.,/:;<>=?@_|~^[]'", ucs4))
    return true;
  return false;
}

static bool
valid_ident_punct(uint32_t ucs4)
{
  if (strchr("!$%&*+-/<>=?@_~", ucs4))
    return true;
  return false;
}

static bool
valid_ident_start(uint32_t ucs4)
{
  return (u_hasBinaryProperty(ucs4,UCHAR_XID_START) ||
	  valid_ident_punct(ucs4));
}

static bool
valid_ident_continue(uint32_t ucs4)
{
  return (u_hasBinaryProperty(ucs4,UCHAR_XID_CONTINUE) ||
	  valid_ident_punct(ucs4));
}

static bool
valid_ifident_start(uint32_t ucs4)
{
  return (isalpha(ucs4) || ucs4 == '_');
  //  return (u_hasBinaryProperty(ucs4,UCHAR_XID_START));
}

static bool
valid_ifident_continue(uint32_t ucs4)
{
  return (isalpha(ucs4) || isdigit(ucs4) || ucs4 == '_' || ucs4 == '-');
  //  return (u_hasBinaryProperty(ucs4,UCHAR_XID_CONTINUE) ||
  //valid_ifident_punct(ucs4));
}

static bool
valid_tv_ident_start(uint32_t ucs4)
{
  return (u_hasBinaryProperty(ucs4,UCHAR_XID_START) ||
	  ucs4 == '_');
}

static bool
valid_tv_ident_continue(uint32_t ucs4)
{
  return (u_hasBinaryProperty(ucs4,UCHAR_XID_CONTINUE) ||
	  ucs4 == '_');
}
static bool
valid_charpoint(uint32_t ucs4)
{
  if (valid_char_printable(ucs4))
    return true;

  return u_isgraph(ucs4);
}

static bool
valid_charpunct(uint32_t ucs4)
{
  if (strchr("!\"#$%&'()*+,-./:;{}<=>?@[\\]^_`|~", ucs4))
    return true;
  return false;
}

static unsigned
validate_string(const char *s)
{
  const char *spos = s;
  uint32_t c;

  while (*spos) {
    const char *snext;
    c = sherpa::utf8_decode(spos, &snext); //&OK

    if (c == ' ') {		/* spaces are explicitly legal */
      spos = snext;
    }
    else if (c == '\\') {	/* escaped characters are legal */
      const char *escStart = spos;
      spos++;
      switch (*spos) {
      case 'n':
      case 't':
      case 'r':
      case 'b':
      case 's':
      case 'f':
      case '"':
      case '\\':
	{
	  spos++;
	  break;
	}
      case '{':
	{
	  if (*++spos != 'U')
	    return (spos - s);
	  if (*++spos != '+')
	    return (spos - s);
	  while (*++spos != '}')
	    if (!isxdigit(*spos))
	      return (spos - s);
	}
	spos++;
	break;
      default:
	// Bad escape sequence
	return (escStart - s);
      }
    }
    else if (u_isgraph(c)) {
      spos = snext;
    }
    else return (spos - s);
  }

  return 0;
}

struct BlockLexer::KeyWord BlockLexer::keywords[] = {
  { "array",            tk_ARRAY },
  { "as",               tk_AS },
  { "bitc-version",     tk_BITC_VERSION },
  { "bitfield",         tk_BITFIELD },
  { "bool",             tk_BOOL },
  { "break",            tk_Reserved },
  { "byref",            tk_BY_REF },
  { "ref",              tk_REF },
  { "catch",            tk_CATCH },
  { "char",             tk_CHAR },
  { "const",            tk_CONST },
  { "continue",         tk_CONTINUE },
  { "declare",          tk_DECLARE },
  { "deref",            tk_DEREF },
  { "do",               tk_DO },
  { "double",           tk_DOUBLE },
  { "dup",              tk_DUP },
  { "exception",        tk_EXCEPTION },
  { "exception",        tk_EXCEPTION },
  { "external",         tk_EXTERNAL },
  { "false",            tk_FALSE },
  { "fill",             tk_FILL },
  { "float",            tk_FLOAT },
  { "forall",           tk_FORALL },
  { "if",               tk_IF },
  { "import",           tk_IMPORT },
  { "impure",           tk_IMPURE },
  { "instance",         tk_INSTANCE },
  { "int16",            tk_INT16 },
  { "int32",            tk_INT32 },
  { "int64",            tk_INT64 },
  { "int8",             tk_INT8 },
  { "interface",        tk_INTERFACE },
  { "lambda",           tk_LAMBDA },
  { "let",              tk_LET },
  { "letrec",           tk_LETREC },
  { "module",           tk_MODULE },
  { "mutable",          tk_MUTABLE },
  { "not",              tk_NOT },
  { "opaque",           tk_OPAQUE },
  { "or",               tk_OR },
  { "otherwise",        tk_OTHERWISE },
  { "pair",             tk_PAIR },
  { "provide",          tk_PROVIDE },
  { "pure",             tk_PURE },
  { "ref",              tk_REF },
  { "repr",             tk_REPR },
  { "return",           tk_RETURN },
  { "return-from",      tk_RETURN_FROM },
  { "set!",             tk_SET },
  { "string",           tk_STRING },
  { "struct",           tk_STRUCT },
  { "suspend",          tk_SUSPEND },
  { "switch",           tk_SWITCH },
  { "tag",              tk_TAG },
  { "the",              tk_THE },
  { "throw",            tk_THROW },
  { "true",             tk_TRUE },
  { "try",              tk_TRY },
  { "tyfn",             tk_TYFN },
  { "typeclass",        tk_TYPECLASS },
  { "uint16",           tk_UINT16 },
  { "uint32",           tk_UINT32 },
  { "uint64",           tk_UINT64 },
  { "uint8",            tk_UINT8 },
  { "union",            tk_UNION },
  { "unit",             tk_UNIT },
  { "val",              tk_VAL },
  { "vector",           tk_VECTOR },
  { "when",             tk_WHEN },
  { "where",            tk_WHERE },
  { "word",             tk_WORD }
};

static int
kwstrcmp(const void *vKey, const void *vCandidate)
{
  const char *key = ((const BlockLexer::KeyWord *) vKey)->nm;
  const char *candidate = ((const BlockLexer::KeyWord *) vCandidate)->nm;

  return strcmp(key, candidate);
}

int
BlockLexer::kwCheck(const char *s)
{
  if (ifIdentMode) {
    if (!valid_ifident_start(*s))
      return tk_Reserved;

    for (++s; *s; s++)
      if (!valid_ifident_continue(*s))
	return tk_Reserved;

    return tk_Ident;
  }

  KeyWord key = { s, 0 };
  KeyWord *entry =
    (KeyWord *)bsearch(&key, keywords, // &OK
		       sizeof(keywords)/sizeof(keywords[0]),
		       sizeof(keywords[0]), kwstrcmp);

  // If it is in the token table, return the indicated token type:
  if (entry)
    return entry->tokValue;

  // Otherwise, check for various reserved words:

  // Things starting with "__":
  if (s[0] == '_' && s[1] == '_') {
    if (!isRuntimeUoc)
      return tk_Reserved;
  }

  // Things starting with "def":
  if (s[0] == 'd' && s[1] == 'e' && s[2] == 'f')
    return tk_Reserved;

  // Things starting with "#":
  if (s[0] == '#')
    return tk_Reserved;

  return tk_Ident;
}

void
BlockLexer::ReportParseError()
{
  errStream << here
	    << ": syntax error (via yyerror)" << '\n';
  num_errors++;
}

void
BlockLexer::ReportParseError(const LexLoc& where, std::string msg)
{
  errStream << where
	    << ": "
	    << msg << std::endl;

  num_errors++;
}

void
BlockLexer::ReportParseWarning(const LexLoc& where, std::string msg)
{
  errStream << where
	    << ": "
	    << msg << std::endl;
}

BlockLexer::BlockLexer(std::ostream& _err, std::istream& _in,
		       const std::string& origin,
		       bool commandLineInput)
  :here(origin, 1, 0), inStream(_in), errStream(_err)
{
  inStream.unsetf(std::ios_base::skipws);

  num_errors = 0;
  isRuntimeUoc = false;
  ifIdentMode = false;
  isCommandLineInput = commandLineInput;
  debug = false;
  putbackChar = -1;
  nModules = 0;
}

long
BlockLexer::digitValue(ucs4_t ucs4, unsigned radix)
{
  long l = -1;

  if (ucs4 >= '0' && ucs4 <= '9')
    l = ucs4 - '0';
  if (ucs4 >= 'a' && ucs4 <= 'f')
    l = ucs4 - 'a' + 10;
  if (ucs4 >= 'A' && ucs4 <= 'F')
    l = ucs4 - 'A' + 10;

  if (l > radix)
    l = -1;
  return l;
}

ucs4_t
BlockLexer::getChar()
{
  char utf[8];
  unsigned char c;

  long ucs4 = putbackChar;

  if (putbackChar != -1) {
    putbackChar = -1;
    utf8_encode(ucs4, utf);
    goto checkDigit;
  }

  memset(utf, 0, 8);

  utf[0] = inStream.get();
  c = utf[0];
  if (utf[0] == EOF)
    return EOF;

  if (c <= 127)
    goto done;

  utf[1] = inStream.get();
  if (utf[1] == EOF)
    return EOF;

  if (c <= 223)
    goto done;

  utf[2] = inStream.get();
  if (utf[2] == EOF)
    return EOF;

  if (c <= 239)
    goto done;

  utf[3] = inStream.get();
  if (utf[3] == EOF)
    return EOF;

  if (c <= 247)
    goto done;

  utf[4] = inStream.get();
  if (utf[4] == EOF)
    return EOF;

  if (c <= 251)
    goto done;

  utf[5] = inStream.get();
  if (utf[5] == EOF)
    return EOF;

 done:
  ucs4 = utf8_decode(utf, 0);
 checkDigit:
  thisToken += utf;

  return ucs4;
}

void
BlockLexer::ungetChar(ucs4_t c)
{
  char utf[8];
  assert(putbackChar == -1);
  putbackChar = c;

  unsigned len = utf8_encode(c, utf);
  thisToken.erase( thisToken.length() - len);
}

static bool
isCharDelimiter(ucs4_t c)
{
  switch (c) {
  case ' ':
  case '\t':
  case '\n':
  case '\r':
  case ')':
    return true;
  default:
    return false;
  }
}

int
BlockLexer::lex(ParseType *lvalp)
{
  ucs4_t c;

 startOver:
  thisToken.erase();

  c = getChar();

  switch (c) {
  case ';':			// Comments
    do {
      c = getChar();
    } while (c != '\n' && c != '\r');
    ungetChar(c);
    // FALL THROUGH

  case ' ':			// White space
  case '\t':
  case '\n':
  case '\r':
    here.updateWith(thisToken);
    goto startOver;


  case '.':			// Single character tokens
  case ',':
  case '[':
  case ']':
  case '(':
  case ')':
  case ':':
  case '^':
    lvalp->tok = LToken(here, thisToken);
    here.updateWith(thisToken);
    return c;


  case '"':			// String literal
    {
      do {
	c = getChar();

	if (c == '\\') {
	  (void) getChar();	// just ignore it -- will validate later
	}
      }	while (c != '"');

      unsigned badpos = validate_string(thisToken.c_str());

      if (badpos) {
	LexLoc badHere = here;
	badHere.offset += badpos;
	errStream << badHere.asString()
		  << ": Illegal (non-printing) character in string '"
		  << thisToken << "'\n";
	num_errors++;
      }

      LexLoc tokStart = here;
      here.updateWith(thisToken);
      lvalp->tok = LToken(here, thisToken.substr(1, thisToken.size()-2));
      return tk_String;
    }


  case '#':			// character and boolean literals
    {
      c = getChar();
      switch(c) {
      case 't':
	lvalp->tok = LToken(here, thisToken);
	here.updateWith(thisToken);
	return tk_TRUE;
      case 'f':
	lvalp->tok = LToken(here, thisToken);
	here.updateWith(thisToken);
	return tk_FALSE;
      case '\\':
	{
	  c = getChar();

	  if (valid_charpunct(c)) {
	    lvalp->tok = LToken(here, thisToken);
	    here.updateWith(thisToken);
	    return tk_Char;
	  }
	  else if (c == 'U') {
	    c = getChar();
	    if (c == '+') {
	      do {
		c = getChar();
	      } while (digitValue(c, 16) >= 0);
	
	      if (!isCharDelimiter(c)) {
		ungetChar(c);
		return EOF;
	      }
	    }
	    ungetChar(c);

	    lvalp->tok = LToken(here, thisToken);
	    here.updateWith(thisToken);
	    return tk_Char;
	  }
	  else if (valid_charpoint(c)) {
	    // Collect more characters in case this is a named character
	    // literal.
	    do {
	      c = getChar();
	    } while (isalpha(c));
	    ungetChar(c);

	    if (thisToken.size() == 3 || // '#' '\' CHAR
		thisToken == "#\\space" ||
		thisToken == "#\\linefeed" ||
		thisToken == "#\\return" ||
		thisToken == "#\\tab" ||
		thisToken == "#\\backspace" ||
		thisToken == "#\\lbrace" ||
		thisToken == "#\\rbrace") {
	      lvalp->tok = LToken(here, thisToken);
	      here.updateWith(thisToken);
	      return tk_Char;
	    }
	  }

	  // FIX: this is bad input
	  return EOF;
	}
      default:
	// FIX: this is bad input
	return EOF;
      }
    }

  case '\'':			// Type variables
    {
      int tokType = tk_TypeVar;

      c = getChar();

      if (c == '%') {
	tokType = tk_EffectVar;
	c = getChar();
      }

      if (!valid_tv_ident_start(c)) {
	// FIX: this is bad input
	ungetChar(c);
	return EOF;
      }

      do {
	c = getChar();
      } while (valid_tv_ident_continue(c));
      ungetChar(c);

      here.updateWith(thisToken);
      lvalp->tok = LToken(here, thisToken);
      return tokType;
    }

    // Hyphen requires special handling. If it is followed by a digit
    // then it is the beginning of a numeric literal, else it is part
    // of an identifier.
  case '-':
    c = getChar();
    if (digitValue(c, 10) < 0) {
      ungetChar(c);
      goto identifier;
    }

    /* ELSE fall through to digit processing */
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    {
      int radix = 10;

      do {
	c = getChar();
      } while (digitValue(c, 10) >= 0);

      /* At this point we could either discover a radix marker 'r', or
	 a decimal point (indicating a floating poing literal). If it
	 is a radix marker, change the radix value here so that we
	 match the succeeding digits in the correct radix. */
      if (c == 'r') {
	radix = strtol(thisToken.c_str(), 0, 10);
	if (radix < 0) radix = -radix; // leading sign not part of radix

	long count = 0;
	do {
	  c = getChar();
	  count++;
	} while (digitValue(c, radix) >= 0);
	count--;
	/* FIX: if count is 0, number is malformed */
      }

      /* We are either done with the literal, in which case it is an
	 integer literal, or we are about to see a decimal point, in
	 which case it is a floating point literal */
      if (c != '.') {
	ungetChar(c);
	lvalp->tok = LToken(here, thisToken);
	here.updateWith(thisToken);
	return tk_Int;
      }

      /* We have seen a decimal point, so from here on it must be a
	 floating point literal. */
      {
	long count = 0;
	do {
	  c = getChar();
	  count++;
	} while (digitValue(c, radix) >= 0);
	count--;
	/* FIX: if count is 0, number is malformed */
      }

      /* We are either done with this token or we are looking at a '^'
	 indicating start of an exponent. */
      if (c != '^') {
	ungetChar(c);
	lvalp->tok = LToken(here, thisToken);
	here.updateWith(thisToken);
	return tk_Float;
      }

      /* Need to collect the exponent. Revert to radix 10 until
	 otherwise proven. */
      c = getChar();
      radix = 10;

      if (c != '-' && digitValue(c, radix) < 0) {
	// FIX: Malformed token
      }

      do {
	c = getChar();
      } while (digitValue(c, 10) >= 0);

      /* Check for radix marker on exponent */
      if (c == 'r') {
	radix = strtol(thisToken.c_str(), 0, 10);
	if (radix < 0) radix = -radix; // leading sign not part of radix

	long count = 0;
	do {
	  c = getChar();
	  count++;
	} while (digitValue(c, radix) >= 0);
	count--;
	/* FIX: if count is 0, number is malformed */
      }

      ungetChar(c);
      lvalp->tok = LToken(here, thisToken);
      here.updateWith(thisToken);
      return tk_Float;
    }

  case EOF:
    return EOF;

  default:
    if (valid_ident_start(c))
      goto identifier;

    // FIX: Malformed token
    return EOF;
  }

 identifier:
  do {
    c = getChar();
  } while (valid_ident_continue(c));
  ungetChar(c);
  lvalp->tok = LToken(here, thisToken);
  here.updateWith(thisToken);
  return kwCheck(thisToken.c_str());
}