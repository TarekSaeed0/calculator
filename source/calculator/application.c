#include <calculator/application.h>
#include <gtk/gtkcssprovider.h>

struct _CalculatorApp {
	GtkApplication parent;
};

G_DEFINE_TYPE(CalculatorApp, calculator_application, GTK_TYPE_APPLICATION)

static void calculator_application_init(CalculatorApp *application) {
	(void)application;
}

static void remove_provider(gpointer data) {
	GtkStyleProvider *provider = data;
	gtk_style_context_remove_provider_for_display(
		gdk_display_get_default(),
		provider
	);
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

	GtkCssProvider *css_provider = gtk_css_provider_new();
	gtk_css_provider_load_from_resource(
		css_provider,
		"/com/github/TarekSaeed0/Calculator/window.css"
	);
	gtk_style_context_add_provider_for_display(
		gdk_display_get_default(),
		GTK_STYLE_PROVIDER(css_provider),
		G_MAXUINT
	);
	g_object_set_data_full(
		window,
		"provider",
		GTK_STYLE_PROVIDER(css_provider),
		remove_provider
	);

	gtk_window_present(GTK_WINDOW(window));

	g_object_unref(builder);
}

static void calculator_application_class_init(CalculatorAppClass *class) {
	G_APPLICATION_CLASS(class)->activate = calculator_application_activate;
}

CalculatorApp *calculator_application_new(void) {
	return g_object_new(
		CALCULATOR_APPLICATION_TYPE,
		"application-id",
		"com.github.TarekSaeed0.Calculator",
		"flags",
		G_APPLICATION_DEFAULT_FLAGS,
		NULL
	);
}
