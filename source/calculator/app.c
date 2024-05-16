#include <calculator/app.h>

struct _CalculatorApp {
	GtkApplication parent;
};

G_DEFINE_TYPE(CalculatorApp, calculator_app, GTK_TYPE_APPLICATION)

static void calculator_app_init(CalculatorApp *app) {
	(void)app;
}

static void calculator_app_activate(GApplication *app) {
	GtkBuilder *builder = gtk_builder_new_from_resource(
		"/com/github/TarekSaeed0/Calculator/calculator.ui"
	);

	GObject *window = gtk_builder_get_object(builder, "window");
	gtk_window_set_application(GTK_WINDOW(window), GTK_APPLICATION(app));

	gtk_widget_set_visible(GTK_WIDGET(window), TRUE);

	g_object_unref(builder);
}

static void calculator_app_class_init(CalculatorAppClass *class) {
	G_APPLICATION_CLASS(class)->activate = calculator_app_activate;
}

CalculatorApp *calculator_app_new(void) {
	return g_object_new(
		CALCULATOR_APP_TYPE,
		"application-id",
		"com.github.TarekSaeed0.Calculator",
		"flags",
		G_APPLICATION_DEFAULT_FLAGS,
		NULL
	);
}
