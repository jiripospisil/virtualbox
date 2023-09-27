/* DLexer.h (formerly DLexer.cpp)
 *
 * This was renamed because the use of the .cpp extension caused problems
 * with IDEs.
 *
 * SOFTWARE RIGHTS
 *
 * We reserve no LEGAL rights to the Purdue Compiler Construction Tool
 * Set (PCCTS) -- PCCTS is in the public domain.  An individual or
 * company may do whatever they wish with source code distributed with
 * PCCTS or the code generated by PCCTS, including the incorporation of
 * PCCTS, or its output, into commerical software.
 *
 * We encourage users to develop software with PCCTS.  However, we do ask
 * that credit is given to us for developing PCCTS.  By "credit",
 * we mean that if you incorporate our source code into one of your
 * programs (commercial product, research project, or otherwise) that you
 * acknowledge this fact somewhere in the documentation, research report,
 * etc...  If you like PCCTS and have developed a nice tool with the
 * output, please mention that you developed it using PCCTS.  In
 * addition, we ask that this header remain intact in our source code.
 * As long as these guidelines are kept, we expect to continue enhancing
 * this system and expect to make other tools available as they are
 * completed.
 *
 * ANTLR 1.33
 * Terence Parr
 * Parr Research Corporation
 * with Purdue University and AHPCRC, University of Minnesota
 * 1989-2000
 */

#include <assert.h>

#if defined(VBOX) && defined(__clang__)
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif

#define ZZINC {if ( track_columns ) (++_endcol);}

#define ZZGETC {ch = input->nextChar(); cl = ZZSHIFT(ch);}

#define ZZNEWSTATE	(newstate = dfa[state][cl])

#ifndef ZZCOPY
#define ZZCOPY	\
	/* Truncate matching buffer to size (not an error) */	\
	if (nextpos < lastpos){				\
		*(nextpos++) = ch;			\
	}else{							\
		bufovf = 1;					\
	}
#endif

void DLGLexer::
mode( int m )
{
	/* points to base of dfa table */
	if (m<MAX_MODE){
		automaton = m;
		/* have to redo class since using different compression */
		cl = ZZSHIFT(ch);
	}else{
		sprintf((char *)ebuf,"Invalid automaton mode = %d ",m);
		errstd(ebuf);
	}
}

ANTLRTokenType DLGLexer::
nextTokenType(void)
{
	register int state, newstate;
	/* last space reserved for the null char */
	register DLGChar *lastpos;
	ANTLRTokenType tk;

skip:
	bufovf = 0;
	lastpos = &_lextext[_bufsize-1];
	nextpos = _lextext;
	_begcol = _endcol+1;
more:
	_begexpr = nextpos;
	if ( interactive ) {
		/* interactive version of automaton */
		/* if there is something in ch, process it */
		state = newstate = dfa_base[automaton];
		if (charfull){
			ZZINC;
			ZZCOPY;
			ZZNEWSTATE;
		}
		while (alternatives[newstate]){
			state = newstate;
			ZZGETC;
			ZZINC;
			ZZCOPY;
			ZZNEWSTATE;
		}
		/* figure out if last character really part of token */
		if ((state != dfa_base[automaton]) && (newstate == DfaStates)){
			charfull = 1;
			--nextpos;
		}else{
			charfull = 0;
			state = newstate;
		}
		*(nextpos) = '\0';
		/* Able to transition out of start state to some non err state?*/
		if ( state == dfa_base[automaton] ){
			/* make sure doesn't get stuck */
			advance();
		}
	}
	else { /* non-interactive version of automaton */
		if (!charfull)
			advance();
		else
			ZZINC;
		state = dfa_base[automaton];
		while (ZZNEWSTATE != DfaStates) {
			state = newstate;
            assert(state <= sizeof(dfa)/sizeof(dfa[0]));
			ZZCOPY;
			ZZGETC;
			ZZINC;
		}
		charfull = 1;
		if ( state == dfa_base[automaton] ){
			if (nextpos < lastpos){
				*(nextpos++) = ch;
			}else{
				bufovf = 1;
			}
			*nextpos = '\0';
			/* make sure doesn't get stuck */
			advance();
		}else{
			*nextpos = '\0';
		}
	}
	if ( track_columns ) _endcol -= charfull;
	_endexpr = nextpos -1;
	add_erase = 0;
#ifdef OLD
	tk = (ANTLRTokenType)
		 (*actions[accepts[state]])(this);	// must pass this manually
											// actions is not a [] of pointers
											// to member functions.
#endif
	tk = (this->*actions[accepts[state]])();

// MR1
// MR1 11-Apr-97  Help for tracking DLG results
// MR1

#ifdef DEBUG_LEXER

/* MR1 */        if (debugLexerFlag) {
/* MR1 */	   if (parser != NULL) {
/* MR1 */	     /* MR23 */ printMessage(stdout, "\ntoken name=%s",parser->parserTokenName(tk));
/* MR1 */	   } else {
/* MR1 */	     /* MR23 */ printMessage(stdout, "\ntoken nnumber=%d",tk);
/* MR1 */	   };
/* MR1 */	   /* MR23 */ printMessage(stdout, " lextext=(%s) mode=%d",
/* MR1 */		 (_lextext[0]=='\n' && _lextext[1]==0) ?
/* MR1 */			"newline" : _lextext,
/* MR1 */				automaton);
/* MR1 */          if (interactive && !charfull) {
/* MR1 */	     /* MR23 */ printMessage(stdout, " char=empty");
/* MR1 */          } else {
/* MR1 */	     if (ch=='\n') {
/* MR1 */	       /* MR23 */ printMessage(stdout, " char=newline");
/* MR1 */	     } else {
/* MR1 */	       /* MR23 */ printMessage(stdout, " char=(%c)",ch);
/* MR1 */	     };
/* MR1 */	   };
/* MR1 */	   /* MR23 */ printMessage(stdout, " %s\n",
/* MR1 */		 (add_erase==1 ? "skip()" :
/* MR1 */		  add_erase==2 ? "more()" :
/* MR1 */		  ""));
/* MR1 */        };

#endif

	switch (add_erase) {
		case 1: goto skip;
		case 2: goto more;
	}
	return tk;
}

void DLGLexer::
advance()
{
	if ( input==NULL ) err_in();
	ZZGETC; charfull = 1; ZZINC;
}
