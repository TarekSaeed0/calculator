#ifndef CALCULATOR_RATIO_H
#define CALCULATOR_RATIO_H

#include <glib.h>

struct calculator_ratio {
	gint sign;
	guint64 numerator;
	guint64 denominator;
};

struct calculator_ratio calculator_ratio_from_value(const gdouble value);
gdouble calculator_ratio_to_value(const struct calculator_ratio ratio);

GString *calculator_ratio_to_string(struct calculator_ratio ratio);

struct calculator_ratio calculator_ratio_limit_denominator(
	const struct calculator_ratio ratio,
	const guint64 maximum_denominator
);

#endif
