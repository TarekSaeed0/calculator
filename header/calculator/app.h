#ifndef CALCULATOR_APP_H
#define CALCULATOR_APP_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CALCULATOR_APP_TYPE calculator_app_get_type()
G_DECLARE_FINAL_TYPE(
	CalculatorApp,
	calculator_app,
	CALCULATOR,
	APP,
	GtkApplication
)

CalculatorApp *calculator_app_new(void);

G_END_DECLS

#endif
