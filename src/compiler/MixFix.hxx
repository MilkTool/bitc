#ifndef MIXFIX_HXX
#define MIXFIX_HXX

/**************************************************************************
 *
 * Copyright (C) 2010, Jonathan S. Shapiro
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

/// @bug This file may not survive, but the mixfix system was getting
/// complicated enough to pull it out into a separate file. The issue
/// I anticipate is that mixfix is statefule, so we may need to attach
/// the mixfix state to the lexer. That's where all of the parser
/// state lives, because Bison doesn't provide a straightforward way
/// to have a parser object.

#include <iostream>

#include "AST.hxx"
#include "shared_ptr.hxx"

enum Associativity {
  assoc_left = -1,
  assoc_right = 1,
  assoc_none = 0
};

enum Fixity {
  prefix,
  postfix,
  infix,
  closed                      // sometimes called "outfix"
};

enum SyntacticCategory {
  msc_keyword,                  // ??
  msc_expr,
  msc_type,
  msc_kind,                     // not yet implemented
};

/// @brief Given a mixfix expression AST, convert it into a
/// expression tree according to the prevailing mixfix rules.
boost::shared_ptr<AST>
ProcessMixFix(std::ostream& errStream, boost::shared_ptr<AST> mixAST);

/// @brief Initialize the mixfix rule table (temporary expedient for
/// testing).
void mixfix_init();

#endif /* MIXFIX_HXX */
