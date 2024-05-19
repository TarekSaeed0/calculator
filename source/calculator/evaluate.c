#include <calculator/evaluate.h>
#include <math.h>

/*

expression ::= term | expression ( "+" | "-" ) term
term ::= factor | term ( "*" | "/" ) factor
factor ::= atom | ( "+" | "-" ) primary
primary ::= number | "(" expression ")"
number ::= digit, { digit }, [ "." , { digit } ] | "." , digit, { digit }
digit ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"

*/

static gdouble calculator_evaluate_expression(const gchar **expression);
static gdouble calculator_evaluate_primary(const gchar **expression) {
	gdouble value;

	while (g_ascii_isspace(**expression)) {
		++*expression;
	}
	if (**expression == '(') {
		++*expression;

		value = calculator_evaluate_expression(expression);

		while (g_ascii_isspace(**expression)) {
			++*expression;
		}
		if (**expression == ')') {
			++*expression;
		}
	} else {
		gchar *end;
		value = g_strtod(*expression, &end);
		if (end == *expression) {
			return NAN;
		}
		*expression = end;
	}

	return value;
}

static gdouble calculator_evaluate_factor(const gchar **expression) {
	while (g_ascii_isspace(**expression)) {
		++*expression;
	}
	switch (**expression) {
		case '+': {
			++*expression;
			return calculator_evaluate_factor(expression);
		} break;
		case '-': {
			++*expression;
			return -calculator_evaluate_factor(expression);
		} break;
		default: return calculator_evaluate_primary(expression);
	}
}

static gdouble calculator_evaluate_term(const gchar **expression) {
	gdouble value = calculator_evaluate_factor(expression);

	while (1) {
		while (g_ascii_isspace(**expression)) {
			++*expression;
		}
		switch (**expression) {
			case '*': {
				++*expression;
				value *= calculator_evaluate_factor(expression);
			} break;
			case '/': {
				++*expression;
				value /= calculator_evaluate_factor(expression);
			} break;
			default: return value;
		}
	}
}

static gdouble calculator_evaluate_expression(const gchar **expression) {
	gdouble value = calculator_evaluate_term(expression);

	while (1) {
		while (g_ascii_isspace(**expression)) {
			++*expression;
		}
		switch (**expression) {
			case '+': {
				++*expression;
				value += calculator_evaluate_term(expression);
			} break;
			case '-': {
				++*expression;
				value -= calculator_evaluate_term(expression);
			} break;
			default: return value;
		}
	}
}

gdouble calculator_evaluate(const gchar *expression, const gchar **expression_end) {
	gdouble value = calculator_evaluate_expression(&expression);

	if (expression_end) {
		*expression_end = expression;
	}

	return value;
}
