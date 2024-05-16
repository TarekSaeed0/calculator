#include <gtk/gtk.h>

#include <calculator/application.h>

int main(int argc, char *argv[]) {
	return g_application_run(
		G_APPLICATION(calculator_application_new()),
		argc,
		argv
	);
}
