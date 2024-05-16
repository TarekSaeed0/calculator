#ifndef CALCULATOR_APPLICATION_H
#define CALCULATOR_APPLICATION_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CALCULATOR_APPLICATION_TYPE calculator_application_get_type()
G_DECLARE_FINAL_TYPE(
	CalculatorApp,
	calculator_application,
	CALCULATOR,
	APPLICATION,
	GtkApplication
)

CalculatorApp *calculator_application_new(void);

G_END_DECLS

#endif
