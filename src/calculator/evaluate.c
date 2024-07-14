#include <calculator/evaluate.h>
#include <math.h>

// temporary fix until
// https://gitlab.gnome.org/GNOME/glib/-/commit/c583162cc6d7078ff549c72615617092b0bc150a

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"

/*

expression ::= term | expression ( "+" | "-" ) term
term ::= factor | term ( "*" | "/" ) factor
factor ::= atom | ( "+" | "-" ) primary
primary ::= ( number | "(" expression ")" ) , { "(" expression ")" }
number ::= digit , { digit } , [ "." , { digit } ] | "." , digit , { digit }
digit ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"

*/

static inline void skip_whitespace(const gchar **string) {
	while (g_unichar_isspace(g_utf8_get_char(*string))) {
		*string = g_utf8_next_char(*string);
	}
}

static gdouble expression(const gchar **string);
static gdouble primary(const gchar **string) {
	gdouble value;

	skip_whitespace(string);
	if (g_utf8_get_char(*string) == '(') {
		*string = g_utf8_next_char(*string);

		value = expression(string);

		skip_whitespace(string);
		if (g_utf8_get_char(*string) == ')') {
			*string = g_utf8_next_char(*string);
		}
	} else {
		gchar *end;
		value = g_strtod(*string, &end);
		if (end == *string) {
			return NAN;
		}
		*string = end;
	}

	while (TRUE) {
		skip_whitespace(string);
		if (g_utf8_get_char(*string) != '(') {
			break;
		}
		*string = g_utf8_next_char(*string);

		value *= expression(string);

		skip_whitespace(string);
		if (g_utf8_get_char(*string) == ')') {
			*string = g_utf8_next_char(*string);
		}
	}

	return value;
}

static gdouble factor(const gchar **string) {
	skip_whitespace(string);
	switch (g_utf8_get_char(*string)) {
		case '+': {
			*string = g_utf8_next_char(*string);
			return factor(string);
		} break;
		case '-': {
			*string = g_utf8_next_char(*string);
			return -factor(string);
		} break;
		default: return primary(string);
	}
}

static gdouble term(const gchar **string) {
	gdouble value = factor(string);

	while (TRUE) {
		skip_whitespace(string);
		switch (g_utf8_get_char(*string)) {
			case L'×': {
				*string = g_utf8_next_char(*string);
				value *= factor(string);
			} break;
			case L'÷': {
				*string = g_utf8_next_char(*string);
				value /= factor(string);
			} break;
			default: return value;
		}
	}
}

static gdouble expression(const gchar **string) {
	gdouble value = term(string);

	while (TRUE) {
		skip_whitespace(string);
		switch (g_utf8_get_char(*string)) {
			case '+': {
				*string = g_utf8_next_char(*string);
				value += term(string);
			} break;
			case '-': {
				*string = g_utf8_next_char(*string);
				value -= term(string);
			} break;
			default: return value;
		}
	}
}

gdouble calculator_evaluate(const gchar *string) {
	gdouble value = expression(&string);

	skip_whitespace(&string);
	if (*string != '\0') {
		return NAN;
	}

	return value;
}

#pragma GCC diagnostic pop
