#include <calculator/application.h>

#include <calculator/evaluate.h>
#include <calculator/ratio.h>

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

static GtkTextView *expression_view = NULL;
static GtkTextBuffer *expression_buffer = NULL;

static GtkTextView *value_view = NULL;
static GtkTextBuffer *value_buffer = NULL;

static GtkButton *button_toggle_ratio = NULL;
static gboolean value_as_ratio = false;

G_MODULE_EXPORT void button_insert_clicked(GtkButton *self, gpointer user_data) {
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

G_MODULE_EXPORT void button_evaluate_clicked(GtkButton *self, gpointer user_data) {
	(void)self, (void)user_data;

	GtkTextIter start, end;
	gtk_text_buffer_get_start_iter(expression_buffer, &start);
	gtk_text_buffer_get_end_iter(expression_buffer, &end);

	gchar *text = gtk_text_buffer_get_text(expression_buffer, &start, &end, FALSE);

	gdouble value = calculator_evaluate(text);

	struct calculator_ratio ratio = calculator_ratio_limit_denominator(
		calculator_ratio_from_value(value),
		G_GUINT64_CONSTANT(1000000000)
	);

	if (!isfinite(value)) {
		gtk_text_buffer_set_text(value_buffer, "Math Error", -1);
	} else {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
		if (ratio.denominator != 1 && calculator_ratio_to_value(ratio) == value) {
#pragma GCC diagnostic pop
			gtk_widget_set_visible(GTK_WIDGET(button_toggle_ratio), TRUE);
		} else {
			value_as_ratio = FALSE;
		}

		GString *value_text;
		if (value_as_ratio) {
			value_text = calculator_ratio_to_string(ratio);
		} else {
			value_text = g_string_new(NULL);
			g_string_printf(value_text, "%lg", value);
		}

		gtk_text_buffer_set_text(value_buffer, value_text->str, (gint)value_text->len);

		g_string_free(value_text, TRUE);
	}

	g_free(text);
}

G_MODULE_EXPORT void button_toggle_ratio_clicked(GtkButton *self, gpointer user_data) {
	(void)self, (void)user_data;

	value_as_ratio = !value_as_ratio;
	button_evaluate_clicked(NULL, NULL);
}

G_MODULE_EXPORT void text_buffer_changed(GtkTextBuffer *self, gpointer user_data) {
	(void)self, (void)user_data;

	gtk_text_buffer_set_text(value_buffer, "", -1);
	gtk_widget_set_visible(GTK_WIDGET(button_toggle_ratio), FALSE);
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

	switch (keyval) {
		case GDK_KEY_Return: button_evaluate_clicked(NULL, NULL); break;
		case GDK_KEY_Tab: button_toggle_ratio_clicked(NULL, NULL); break;
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

	button_toggle_ratio = GTK_BUTTON(gtk_builder_get_object(builder, "button-toggle-ratio"));

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
