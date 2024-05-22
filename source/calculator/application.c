#include <calculator/application.h>
#include <calculator/evaluate.h>
#include <float.h>
#include <math.h>

struct _CalculatorApplication {
	GtkApplication parent;
};

G_DEFINE_TYPE(CalculatorApplication, calculator_application, GTK_TYPE_APPLICATION)

static void calculator_application_init(CalculatorApplication *application) {
	(void)application;
}

static void remove_style_provider(gpointer data) {
	GtkStyleProvider *provider = GTK_STYLE_PROVIDER(data);
	gtk_style_context_remove_provider_for_display(gdk_display_get_default(), provider);
}

static GtkTextView *expression_view;
static GtkTextBuffer *expression_buffer;

static GtkTextView *value_view;
static GtkTextBuffer *value_buffer;

G_MODULE_EXPORT void button_clicked(GtkButton *self, gpointer user_data) {
	(void)self, (void)user_data;

	gtk_text_buffer_insert_at_cursor(expression_buffer, gtk_button_get_label(self), -1);
}

G_MODULE_EXPORT void button_delete_clicked(GtkButton *self, gpointer user_data) {
	(void)self, (void)user_data;

	GtkTextMark *mark = gtk_text_buffer_get_mark(expression_buffer, "insert");
	GtkTextIter iterator;
	gtk_text_buffer_get_iter_at_mark(expression_buffer, &iterator, mark);
	gtk_text_buffer_backspace(expression_buffer, &iterator, TRUE, TRUE);
}

struct ratio {
	gint sign;
	guint64 numerator;
	guint64 denominator;
};

// https://stackoverflow.com/questions/50962041/how-can-i-get-numerator-and-denominator-from-a-fractional-number/50976626#50976626

static struct ratio ratio_from_value(const gdouble value) {
	guint64 bits;
	G_STATIC_ASSERT(sizeof(bits) == sizeof(gdouble));
	memcpy(&bits, &value, sizeof(value));

	const guint64 sign_bit = bits >> (sizeof(bits) * CHAR_BIT - 1);
	const guint64 exponent_bits =
		(bits >> (DBL_MANT_DIG - 1)) & ((DBL_MAX_EXP + 1) - (DBL_MIN_EXP - 1));
	const guint64 mantissa_bits = bits & ((G_GUINT64_CONSTANT(1) << (DBL_MANT_DIG - 1)) - 1);

	const gint sign = sign_bit ? -1 : 1;

	if (exponent_bits == 0 && mantissa_bits == 0) {
		return (struct ratio){ sign, 0, 1 };
	} else if (exponent_bits == (DBL_MAX_EXP + 1) - (DBL_MIN_EXP - 1)) {
		if (mantissa_bits == 0) {
			return (struct ratio){ sign, 1, 0 };
		} else {
			return (struct ratio){ sign, 0, 0 };
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
		return (struct ratio){ sign, significand, G_GUINT64_CONSTANT(1) << (guint64)-exponent };
	} else {
		return (struct ratio){ sign, significand << (guint64)exponent, 1 };
	}
}

static gdouble ratio_to_value(const struct ratio ratio) {
	return ratio.sign * (gdouble)ratio.numerator / (gdouble)ratio.denominator;
}

// https://docs.python.org/3/library/fractions.html#fractions.Fraction.limit_denominator

static struct ratio ratio_limit_denominator(
	const struct ratio ratio,
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
		return (struct ratio){ ratio.sign, p1, q1 };
	} else {
		return (struct ratio){ ratio.sign, p0 + k * p1, q0 + k * q1 };
	}
}

static void ratio_print(struct ratio ratio) {

	if (ratio.denominator == 0) {
		if (ratio.numerator == 0) {
			g_print("undefined");
		} else {
			if (ratio.sign == -1) {
				g_print("-");
			}
			g_print("infinity");
		}
	} else {
		if (ratio.sign == -1) {
			g_print("-");
		}
		g_print("%" PRIi64, ratio.numerator);
		if (ratio.denominator != 1) {
			g_print("/%" PRIu64, ratio.denominator);
		}
	}
}

G_MODULE_EXPORT void button_equal_clicked(GtkButton *self, gpointer user_data) {
	(void)self, (void)user_data;

	GtkTextIter start, end;
	gtk_text_buffer_get_start_iter(expression_buffer, &start);
	gtk_text_buffer_get_end_iter(expression_buffer, &end);

	gchar *text = gtk_text_buffer_get_text(expression_buffer, &start, &end, FALSE);
	g_print("%s --> ", text);

	gdouble value = calculator_evaluate(text);
	g_print("%.*lg --> ", DBL_DIG, value);

	struct ratio ratio = ratio_from_value(value);
	ratio_print(ratio);
	g_print(" --> ");

	ratio = ratio_limit_denominator(ratio, G_GUINT64_CONSTANT(1000000000));
	ratio_print(ratio);
	g_print(" --> ");

	g_print("%.*lg\n", DBL_DIG, ratio_to_value(ratio));

	if (!isfinite(value)) {
		gtk_text_buffer_set_text(value_buffer, "Math Error", -1);
	} else {
		GString *value_text = g_string_new(NULL);
		g_string_printf(value_text, "%lg", value);

		gtk_text_buffer_set_text(value_buffer, value_text->str, (gint)value_text->len);

		g_string_free(value_text, TRUE);
	}

	g_free(text);
}

G_MODULE_EXPORT void text_buffer_changed(GtkTextBuffer *self, gpointer user_data) {
	(void)self, (void)user_data;

	gtk_text_buffer_set_text(value_buffer, "", -1);
}

G_MODULE_EXPORT void text_buffer_insert_text(
	GtkTextBuffer *self,
	const GtkTextIter *location,
	gchar *text,
	gint len,
	gpointer user_data
) {
	(void)len, (void)user_data;

	GString *filtered_text = g_string_new(NULL);

	while (*text != '\0') {
		gunichar character = g_utf8_get_char(text);
		switch (character) {
			case '+':
			case '-':
			case L'×':
			case L'÷':
			case '(':
			case ')':
			case '.': break;
			case '*': character = L'×'; break;
			case '/': character = L'÷'; break;
			default: {
				if ((character & 0x80) || !g_unichar_isdigit(character)) {
					text = g_utf8_next_char(text);
					continue;
				}
			}
		}
		g_string_append_unichar(filtered_text, character);
		text = g_utf8_next_char(text);
	}

	GtkTextIter iterator = *location;

	g_signal_handlers_block_by_func(self, (gpointer)text_buffer_insert_text, NULL);
	gtk_text_buffer_insert(self, &iterator, filtered_text->str, (gint)filtered_text->len);
	g_signal_handlers_unblock_by_func(self, (gpointer)text_buffer_insert_text, NULL);

	g_string_free(filtered_text, TRUE);

	g_signal_stop_emission_by_name(self, "insert-text");
}

static gboolean window_key_press_callback(
	GtkEventControllerKey *self,
	guint keyval,
	guint keycode,
	GdkModifierType state,
	gpointer user_data
) {
	(void)self, (void)keycode, (void)state, (void)user_data;

	if (keyval == GDK_KEY_Return) {
		button_equal_clicked(NULL, NULL);
	}

	return FALSE;
}

static void calculator_application_activate(GApplication *application) {
	GtkBuilder *builder =
		gtk_builder_new_from_resource("/com/github/TarekSaeed0/calculator/window.ui");

	GtkWindow *window = GTK_WINDOW(gtk_builder_get_object(builder, "window"));
	gtk_window_set_application(GTK_WINDOW(window), GTK_APPLICATION(application));

	GtkEventController *controller = gtk_event_controller_key_new();
	g_signal_connect(controller, "key-pressed", G_CALLBACK(window_key_press_callback), NULL);
	gtk_widget_add_controller(GTK_WIDGET(window), controller);
	gtk_event_controller_set_propagation_phase(controller, GTK_PHASE_CAPTURE);

	GtkCssProvider *provider = gtk_css_provider_new();
	gtk_css_provider_load_from_resource(provider, "/com/github/TarekSaeed0/calculator/style.css");
	gtk_style_context_add_provider_for_display(
		gdk_display_get_default(),
		GTK_STYLE_PROVIDER(provider),
		G_MAXUINT
	);
	g_object_set_data_full(
		G_OBJECT(window),
		"provider",
		GTK_STYLE_PROVIDER(provider),
		remove_style_provider
	);

	GtkIconTheme *icon_theme =
		gtk_icon_theme_get_for_display(gtk_widget_get_display(GTK_WIDGET(window)));
	gtk_icon_theme_add_resource_path(icon_theme, "/com/github/TarekSaeed0/calculator/icons");

	expression_view = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "expression-view"));
	expression_buffer = gtk_text_view_get_buffer(expression_view);

	value_view = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "value-view"));
	value_buffer = gtk_text_view_get_buffer(value_view);

	gtk_window_present(GTK_WINDOW(window));

	g_object_unref(builder);
}

static void calculator_application_class_init(CalculatorApplicationClass *class) {
	G_APPLICATION_CLASS(class)->activate = calculator_application_activate;
}

CalculatorApplication *calculator_application_new(void) {
	return g_object_new(
		CALCULATOR_APPLICATION_TYPE,
		"application-id",
		"com.github.TarekSaeed0.calculator",
		"flags",
		G_APPLICATION_DEFAULT_FLAGS,
		NULL
	);
}
