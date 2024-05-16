#include <gtk/gtk.h>

#include <calculator/app.h>

int main(int argc, char *argv[]) {
	return g_application_run(G_APPLICATION(calculator_app_new()), argc, argv);
}
