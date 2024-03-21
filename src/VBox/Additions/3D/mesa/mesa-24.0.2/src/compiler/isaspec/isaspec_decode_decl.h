/*
 * Copyright © 2020 Google, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* This file should not be included directly. Instead, it is included as part
 * of the header file generated by isaspec/decode.py
 */

#include <stdbool.h>
#include <stdint.h>

/*
 * Defines the tables which are generated from xml for disassembly
 */

struct decode_scope;
struct isa_bitset;

/**
 * Table of enum values
 */
struct isa_enum {
	unsigned num_values;
	struct {
		unsigned val;
		const char *display;
	} values[];
};

/**
 * An expression used to for conditional overrides, derived fields, etc
 */
typedef uint64_t (*isa_expr_t)(struct decode_scope *scope);

/**
 * Used by generated expr functions
 */
uint64_t isa_decode_field(struct decode_scope *scope, const char *field_name);

void isa_decode_bitset(void *out, const struct isa_bitset **bitsets, struct decode_scope *scope, bitmask_t val);

/**
 * Used by generated decode functions
 */
uint32_t isa_get_gpu_id(struct decode_scope *scope);

/**
 * Allows to use gpu_id in expr functions
 */
#define ISA_GPU_ID() isa_get_gpu_id(scope)

/**
 * For bitset fields, there are some cases where we want to "remap" field
 * names, essentially allowing one to parameterize a nested bitset when
 * it resolves fields in an enclosing bitset.
 */
struct isa_field_params {
	unsigned num_params;
	struct {
		const char *name;
		const char *as;
	} params[];
};

struct decode_scope;

/**
 * Description of a single field within a bitset case.
 */
struct isa_field {
	const char *name;
	isa_expr_t expr;       /* for virtual "derived" fields */
	unsigned low;
	unsigned high;
	enum {
		/* Basic types: */
		TYPE_BRANCH,   /* relative branch target, like INT but optional labeling*/
		TYPE_ABSBRANCH,   /* absolute branch target */
		TYPE_INT,
		TYPE_UINT,
		TYPE_HEX,
		TYPE_OFFSET,   /* Like INT but formated with +/- or omitted if ==0 */
		TYPE_UOFFSET,  /* Like UINT but formated with + or omitted if ==0 */
		TYPE_FLOAT,
		TYPE_BOOL,
		TYPE_BOOL_INV, /* Like BOOL but inverted */
		TYPE_ENUM,

		/* For fields that must be printed via a user-provided callback */
		TYPE_CUSTOM,

		/* To assert a certain value in a given range of bits.. not
		 * used for pattern matching, but allows an override to specify
		 * that a certain bitpattern in some "unused" bits is expected
		 */
		TYPE_ASSERT,

		/* For fields that are decoded with another bitset hierarchy: */
		TYPE_BITSET,
	} type;
	union {
		const struct isa_bitset **bitsets;  /* if type==BITSET */
		bitmask_t val;                      /* if type==ASSERT */
		const struct isa_enum *enums;       /* if type==ENUM */
		const char *display;                /* if type==BOOL */
		bool call;                          /* if type==(BRANCH|ABSBRANCH) */
	};

	/**
	 * type==BITSET fields can also optionally provide remapping for
	 * field names
	 */
	const struct isa_field_params *params;
};

/**
 * A bitset consists of N "cases", with the last one (with case->expr==NULL)
 * being the default.
 *
 * When resolving a field, display template string, etc, all the cases with
 * an expression that evaluates to non-zero are consider, falling back to
 * the last (default) case.
 */
struct isa_case {
	isa_expr_t expr;
	const char *display;
	unsigned num_fields;
	struct isa_field fields[];
};

struct isa_field_decode {
	const char *name;
	void (*decode)(void *out, struct decode_scope *scope, uint64_t val);
};

/**
 * An individual bitset, the leaves of a bitset inheritance hiearchy will
 * have the match and mask to match a single instruction (or arbitrary
 * bit-pattern) against.
 */
struct isa_bitset {
	const struct isa_bitset *parent;
	const char *name;
	struct {
		unsigned min;
		unsigned max;
	} gen;
	bitmask_t match;
	bitmask_t dontcare;
	bitmask_t mask;
	void (*decode)(void *out, struct decode_scope *scope);
	unsigned num_decode_fields;
	const struct isa_field_decode *decode_fields;
	unsigned num_cases;
	const struct isa_case *cases[];
};
