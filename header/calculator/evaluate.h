#ifndef CALCULATOR_EVALUATE_H
#define CALCULATOR_EVALUATE_H

#include <glib.h>

gdouble calculator_evaluate(
	const gchar *expression,
	const gchar **expression_end
);

#endif
