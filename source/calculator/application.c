#include <calculator/application.h>
#include <math.h>

struct _CalculatorApplication {
	GtkApplication parent;
};

G_DEFINE_TYPE(
	CalculatorApplication,
	calculator_application,
	GTK_TYPE_APPLICATION
)

static void calculator_application_init(CalculatorApplication *application) {
	(void)application;
}

static void remove_provider(gpointer data) {
	GtkStyleProvider *provider = GTK_STYLE_PROVIDER(data);
	gtk_style_context_remove_provider_for_display(
		gdk_display_get_default(),
		provider
	);
}

gboolean should_clear = FALSE;
static GtkLabel *expression_label;
static GtkLabel *value_label;

static void append_to_expression(GtkWidget *widget, gpointer data) {
	(void)widget;

	if (should_clear) {
		gtk_label_set_text(expression_label, "");
		gtk_label_set_text(value_label, "");
		should_clear = FALSE;
	}

	gchar character = (gchar)GPOINTER_TO_INT(data);

	GString *text = g_string_new(gtk_label_get_text(expression_label));
	g_string_append_c(text, character);

	gtk_label_set_text(expression_label, text->str);
}

static gdouble evaluate_expression_atom(const gchar **expression) {
	gchar *end;
	gdouble value = g_strtod(*expression, &end);
	if (end == *expression) {
		return NAN;
	}
	*expression = end;

	return value;
}

static gdouble evaluate_expression_primary(const gchar **expression) {
	while (g_ascii_isspace(**expression)) {
		++*expression;
	}
	switch (**expression) {
		case '+': {
			++*expression;
			return evaluate_expression_atom(expression);
		} break;
		case '-': {
			++*expression;
			return -evaluate_expression_atom(expression);
		} break;
		default: return evaluate_expression_atom(expression);
	}
}

static gdouble evaluate_expression_factor(const gchar **expression) {
	gdouble value = evaluate_expression_primary(expression);

	while (1) {
		while (g_ascii_isspace(**expression)) {
			++*expression;
		}
		switch (**expression) {
			case '*': {
				++*expression;
				value *= evaluate_expression_primary(expression);
			} break;
			case '/': {
				++*expression;
				value /= evaluate_expression_primary(expression);
			} break;
			default: return value;
		}
	}
}

static gdouble evaluate_expression_term(const gchar **expression) {
	gdouble value = evaluate_expression_factor(expression);

	while (1) {
		while (g_ascii_isspace(**expression)) {
			++*expression;
		}
		switch (**expression) {
			case '+': {
				++*expression;
				value += evaluate_expression_factor(expression);
			} break;
			case '-': {
				++*expression;
				value -= evaluate_expression_factor(expression);
			} break;
			default: return value;
		}
	}
}

static void evaluate_expression(GtkWidget *widget, gpointer data) {
	(void)widget, (void)data;

	const gchar *expression = gtk_label_get_text(expression_label);

	gdouble value = evaluate_expression_term(&expression);

	while (g_ascii_isspace(*expression)) {
		++expression;
	}
	if (!isfinite(value) || *expression != '\0') {
		gtk_label_set_text(value_label, "Math Error");
	} else {
		GString *text = g_string_new(NULL);
		g_string_printf(text, "%lg", value);

		gtk_label_set_text(value_label, text->str);
	}

	should_clear = TRUE;
}

static void calculator_application_activate(GApplication *application) {
	GtkBuilder *builder = gtk_builder_new_from_resource(
		"/com/github/TarekSaeed0/Calculator/window.ui"
	);

	GObject *window = gtk_builder_get_object(builder, "window");
	gtk_window_set_application(
		GTK_WINDOW(window),
		GTK_APPLICATION(application)
	);

	expression_label =
		GTK_LABEL(gtk_builder_get_object(builder, "expression-label"));
	value_label = GTK_LABEL(gtk_builder_get_object(builder, "value-label"));

	GString *name = g_string_new(NULL);
	for (gint digit = 0; digit <= 9; digit++) {
		g_string_printf(name, "button-%d", digit);

		GObject *button_digit = gtk_builder_get_object(builder, name->str);
		g_signal_connect(
			GTK_BUTTON(button_digit),
			"clicked",
			G_CALLBACK(append_to_expression),
			GINT_TO_POINTER('0' + digit)
		);
	}

	GObject *button_dot = gtk_builder_get_object(builder, "button-dot");
	g_signal_connect(
		GTK_BUTTON(button_dot),
		"clicked",
		G_CALLBACK(append_to_expression),
		GINT_TO_POINTER('.')
	);

	GObject *button_add = gtk_builder_get_object(builder, "button-add");
	g_signal_connect(
		GTK_BUTTON(button_add),
		"clicked",
		G_CALLBACK(append_to_expression),
		GINT_TO_POINTER('+')
	);

	GObject *button_subtract =
		gtk_builder_get_object(builder, "button-subtract");
	g_signal_connect(
		GTK_BUTTON(button_subtract),
		"clicked",
		G_CALLBACK(append_to_expression),
		GINT_TO_POINTER('-')
	);

	GObject *button_multiply =
		gtk_builder_get_object(builder, "button-multiply");
	g_signal_connect(
		GTK_BUTTON(button_multiply),
		"clicked",
		G_CALLBACK(append_to_expression),
		GINT_TO_POINTER('*')
	);

	GObject *button_divide = gtk_builder_get_object(builder, "button-divide");
	g_signal_connect(
		GTK_BUTTON(button_divide),
		"clicked",
		G_CALLBACK(append_to_expression),
		GINT_TO_POINTER('/')
	);

	GObject *button_equal = gtk_builder_get_object(builder, "button-equal");
	g_signal_connect(
		GTK_BUTTON(button_equal),
		"clicked",
		G_CALLBACK(evaluate_expression),
		NULL
	);

	GtkCssProvider *provider = gtk_css_provider_new();
	gtk_css_provider_load_from_resource(
		provider,
		"/com/github/TarekSaeed0/Calculator/window.css"
	);
	gtk_style_context_add_provider_for_display(
		gdk_display_get_default(),
		GTK_STYLE_PROVIDER(provider),
		G_MAXUINT
	);
	g_object_set_data_full(
		window,
		"provider",
		GTK_STYLE_PROVIDER(provider),
		remove_provider
	);

	gtk_window_present(GTK_WINDOW(window));

	g_object_unref(builder);
}

static void calculator_application_class_init(CalculatorApplicationClass
												  *class) {
	G_APPLICATION_CLASS(class)->activate = calculator_application_activate;
}

CalculatorApplication *calculator_application_new(void) {
	return g_object_new(
		CALCULATOR_APPLICATION_TYPE,
		"application-id",
		"com.github.TarekSaeed0.Calculator",
		"flags",
		G_APPLICATION_DEFAULT_FLAGS,
		NULL
	);
}
