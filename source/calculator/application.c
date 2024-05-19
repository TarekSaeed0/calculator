#include <calculator/application.h>
#include <calculator/evaluate.h>
#include <math.h>

struct _CalculatorApplication {
	GtkApplication parent;
};

G_DEFINE_TYPE(CalculatorApplication, calculator_application, GTK_TYPE_APPLICATION)

static void calculator_application_init(CalculatorApplication *application) {
	(void)application;
}

static void calculator_application_remove_provider(gpointer data) {
	GtkStyleProvider *provider = GTK_STYLE_PROVIDER(data);
	gtk_style_context_remove_provider_for_display(gdk_display_get_default(), provider);
}

static GtkLabel *expression_label;
static GtkLabel *value_label;

static void calculator_application_button_append(GtkWidget *widget, gpointer data) {
	(void)widget;

	gtk_label_set_text(value_label, "");

	gchar character = (gchar)GPOINTER_TO_INT(data);

	GString *text = g_string_new(gtk_label_get_text(expression_label));
	g_string_append_c(text, character);

	gtk_label_set_text(expression_label, text->str);

	g_string_free(text, TRUE);
}
static void calculator_application_button_evaluate(GtkWidget *widget, gpointer data) {
	(void)widget, (void)data;

	const gchar *expression = gtk_label_get_text(expression_label);
	const gchar *expression_end;

	gdouble value = calculator_evaluate(expression, &expression_end);

	while (g_ascii_isspace(*expression_end)) {
		++expression_end;
	}
	if (!isfinite(value) || *expression_end != '\0') {
		gtk_label_set_text(value_label, "Math Error");
	} else {
		GString *text = g_string_new(NULL);
		g_string_printf(text, "%lg", value);

		gtk_label_set_text(value_label, text->str);

		g_string_free(text, TRUE);
	}
}

static void calculator_application_button_delete(GtkWidget *widget, gpointer data) {
	(void)widget, (void)data;

	GString *text = g_string_new(gtk_label_get_text(expression_label));
	if (text->len > 0) {
		g_string_truncate(text, text->len - 1);

		gtk_label_set_text(expression_label, text->str);
	}

	gtk_label_set_text(value_label, "");

	g_string_free(text, TRUE);
}

static void calculator_application_activate(GApplication *application) {
	GtkBuilder *builder =
		gtk_builder_new_from_resource("/com/github/TarekSaeed0/calculator/window.ui");

	GObject *window = gtk_builder_get_object(builder, "window");
	gtk_window_set_application(GTK_WINDOW(window), GTK_APPLICATION(application));

	GtkCssProvider *provider = gtk_css_provider_new();
	gtk_css_provider_load_from_resource(provider, "/com/github/TarekSaeed0/calculator/style.css");
	gtk_style_context_add_provider_for_display(
		gdk_display_get_default(),
		GTK_STYLE_PROVIDER(provider),
		G_MAXUINT
	);
	g_object_set_data_full(
		window,
		"provider",
		GTK_STYLE_PROVIDER(provider),
		calculator_application_remove_provider
	);

	GtkIconTheme *icon_theme =
		gtk_icon_theme_get_for_display(gtk_widget_get_display(GTK_WIDGET(window)));
	gtk_icon_theme_add_resource_path(icon_theme, "/com/github/TarekSaeed0/calculator/icons");

	expression_label = GTK_LABEL(gtk_builder_get_object(builder, "expression-label"));
	value_label = GTK_LABEL(gtk_builder_get_object(builder, "value-label"));

	GString *name = g_string_new(NULL);
	for (gint digit = 0; digit <= 9; digit++) {
		g_string_printf(name, "button-%d", digit);

		GObject *button_digit = gtk_builder_get_object(builder, name->str);
		g_signal_connect(
			GTK_BUTTON(button_digit),
			"clicked",
			G_CALLBACK(calculator_application_button_append),
			GINT_TO_POINTER('0' + digit)
		);
	}

	GObject *button_dot = gtk_builder_get_object(builder, "button-dot");
	g_signal_connect(
		GTK_BUTTON(button_dot),
		"clicked",
		G_CALLBACK(calculator_application_button_append),
		GINT_TO_POINTER('.')
	);

	GObject *button_add = gtk_builder_get_object(builder, "button-add");
	g_signal_connect(
		GTK_BUTTON(button_add),
		"clicked",
		G_CALLBACK(calculator_application_button_append),
		GINT_TO_POINTER('+')
	);

	GObject *button_subtract = gtk_builder_get_object(builder, "button-subtract");
	g_signal_connect(
		GTK_BUTTON(button_subtract),
		"clicked",
		G_CALLBACK(calculator_application_button_append),
		GINT_TO_POINTER('-')
	);

	GObject *button_multiply = gtk_builder_get_object(builder, "button-multiply");
	g_signal_connect(
		GTK_BUTTON(button_multiply),
		"clicked",
		G_CALLBACK(calculator_application_button_append),
		GINT_TO_POINTER('*')
	);

	GObject *button_divide = gtk_builder_get_object(builder, "button-divide");
	g_signal_connect(
		GTK_BUTTON(button_divide),
		"clicked",
		G_CALLBACK(calculator_application_button_append),
		GINT_TO_POINTER('/')
	);

	GObject *button_open_parenthesis = gtk_builder_get_object(builder, "button-open-parenthesis");
	g_signal_connect(
		GTK_BUTTON(button_open_parenthesis),
		"clicked",
		G_CALLBACK(calculator_application_button_append),
		GINT_TO_POINTER('(')
	);

	GObject *button_close_parenthesis = gtk_builder_get_object(builder, "button-close-parenthesis");
	g_signal_connect(
		GTK_BUTTON(button_close_parenthesis),
		"clicked",
		G_CALLBACK(calculator_application_button_append),
		GINT_TO_POINTER(')')
	);

	GObject *button_equal = gtk_builder_get_object(builder, "button-equal");
	g_signal_connect(
		GTK_BUTTON(button_equal),
		"clicked",
		G_CALLBACK(calculator_application_button_evaluate),
		NULL
	);

	GObject *button_delete = gtk_builder_get_object(builder, "button-delete");
	g_signal_connect(
		GTK_BUTTON(button_delete),
		"clicked",
		G_CALLBACK(calculator_application_button_delete),
		NULL
	);

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
