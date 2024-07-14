#include <calculator/ratio.h>

#include <inttypes.h>

// https://stackoverflow.com/questions/50962041/how-can-i-get-numerator-and-denominator-from-a-fractional-number/50976626#50976626

struct calculator_ratio calculator_ratio_from_value(const gdouble value) {
	guint64 bits;
	G_STATIC_ASSERT(sizeof(bits) == sizeof(gdouble));
	memcpy(&bits, &value, sizeof(value));

	const guint64 sign_bit = bits >> (sizeof(bits) * CHAR_BIT - 1);
	const guint64 exponent_bits =
		(bits >> (DBL_MANT_DIG - 1)) & ((DBL_MAX_EXP + 1) - (DBL_MIN_EXP - 1));
	const guint64 mantissa_bits = bits & ((G_GUINT64_CONSTANT(1) << (DBL_MANT_DIG - 1)) - 1);

	const gint sign = sign_bit ? -1 : 1;

	if (exponent_bits == 0 && mantissa_bits == 0) {
		return (struct calculator_ratio){ sign, 0, 1 };
	} else if (exponent_bits == (DBL_MAX_EXP + 1) - (DBL_MIN_EXP - 1)) {
		if (mantissa_bits == 0) {
			return (struct calculator_ratio){ sign, 1, 0 };
		} else {
			return (struct calculator_ratio){ sign, 0, 0 };
		}
	}

	guint64 significand = (G_GUINT64_CONSTANT(1) << (DBL_MANT_DIG - 1)) | mantissa_bits;

	guint64 leading_zeros = 0;
	while (leading_zeros < sizeof(bits) * CHAR_BIT && (significand & 1) == 0) {
		significand >>= 1;
		++leading_zeros;
	}

	gint64 exponent =
		(gint64)(exponent_bits - (DBL_MAX_EXP - 1) - (DBL_MANT_DIG - 1) + leading_zeros);

	if (exponent < 0) {
		if ((guint64)-exponent >= sizeof(bits) * CHAR_BIT) {
			significand >>= (guint64)-exponent - sizeof(bits) * CHAR_BIT + 1;
			exponent = -(gint64)(sizeof(bits) * CHAR_BIT - 1);
		}
		return (struct calculator_ratio
		){ sign, significand, G_GUINT64_CONSTANT(1) << (guint64)-exponent };
	} else {
		return (struct calculator_ratio){ sign, significand << (guint64)exponent, 1 };
	}
}
gdouble calculator_ratio_to_value(const struct calculator_ratio ratio) {
	return ratio.sign * (gdouble)ratio.numerator / (gdouble)ratio.denominator;
}

GString *calculator_ratio_to_string(struct calculator_ratio ratio) {
	GString *string = g_string_new(NULL);
	if (ratio.denominator == 0) {
		if (ratio.numerator == 0) {
			g_string_append(string, "undefined");
		} else {
			if (ratio.sign == -1) {
				g_string_append_c(string, '-');
			}
			g_string_append(string, "infinity");
		}
	} else {
		if (ratio.sign == -1) {
			g_string_append_c(string, '-');
		}
		g_string_append_printf(string, "%" PRIi64, ratio.numerator);
		if (ratio.denominator != 1) {
			g_string_append_printf(string, "/%" PRIi64, ratio.denominator);
		}
	}
	return string;
}

// https://docs.python.org/3/library/fractions.html#fractions.Fraction.limit_denominator

struct calculator_ratio calculator_ratio_limit_denominator(
	const struct calculator_ratio ratio,
	const guint64 maximum_denominator
) {
	// Algorithm notes: For any real number x, define a *best upper
	// approximation* to x to be a rational number p/q such that:
	//
	//   (1) p/q >= x, and
	//   (2) if p/q > r/s >= x then s > q, for any rational r/s.
	//
	// Define *best lower approximation* similarly.  Then it can be
	// proved that a rational number is a best upper or lower
	// approximation to x if, and only if, it is a convergent or
	// semiconvergent of the (unique shortest) continued fraction
	// associated to x.
	//
	// To find a best rational approximation with denominator <= M,
	// we find the best upper and lower approximations with
	// denominator <= M and take whichever of these is closer to x.
	// In the event of a tie, the bound with smaller denominator is
	// chosen.  If both denominators are equal (which can happen
	// only when maximum_denominator == 1 and self is midway between
	// two integers) the lower bound---i.e., the floor of self, is
	// taken.

	g_assert(maximum_denominator >= 1);

	if (ratio.denominator <= maximum_denominator) {
		return ratio;
	}

	guint64 p0 = 0, q0 = 1, p1 = 1, q1 = 0;
	guint64 n = ratio.numerator, d = ratio.denominator, k;
	while (TRUE) {
		guint64 a = n / d;

		guint64 q2 = q0 + a * q1;
		if (q2 > maximum_denominator) {
			break;
		}

		guint64 p0_ = p0, p1_ = p1;
		p0 = p1, q0 = q1, p1 = p0_ + a * p1_, q1 = q2;

		guint64 n_ = n;
		n = d, d = n_ - a * d;

		k = (maximum_denominator - q0) / q1;
	}

	// Determine which of the candidates (p0+k*p1)/(q0+k*q1) and p1/q1 is
	// closer to self. The distance between them is 1/(q1*(q0+k*q1)), while
	// the distance from p1/q1 to self is d/(q1*self._denominator). So we
	// need to compare 2*(q0+k*q1) with self._denominator/d.
	if (2 * d * (q0 + k * q1) <= ratio.denominator) {
		return (struct calculator_ratio){ ratio.sign, p1, q1 };
	} else {
		return (struct calculator_ratio){ ratio.sign, p0 + k * p1, q0 + k * q1 };
	}
}
