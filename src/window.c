// Window management library

#include "common.h"
#include "window.h"

// window_create - creates a new window, and allocates memory for it
// TAKES width and height of the window, as ints
window* window_create(int w, int h) {
	// Allocate memory
	window* win = malloc(sizeof(window));

	// Set params
	win->width = w;
	win->height = h;

	// Create GTK window
	win->GTKwindow = (GtkWindow*)gtk_window_new(GTK_WINDOW_TOPLEVEL);
	
	// Set GTK window parameters
	gtk_window_set_default_size(win->GTKwindow, w, h);

	// Set up event handlers
	g_signal_connect(win->GTKwindow, "destroy", G_CALLBACK(window_on_exit), NULL);

	win->canv = canvas_create();
	GtkWidget* drawArea = gtk_drawing_area_new();
	win->canv->area = (GtkDrawingArea*)drawArea;
	gtk_widget_set_size_request(drawArea, w, h);

	gtk_container_add((GtkContainer*)win->GTKwindow, drawArea);

	#define MAX_SPRITES 64
	win->buffer = spritebuffer_create(MAX_SPRITES);

	render_data* r = renderData_create(win->buffer, win->canv);
	g_signal_connect(G_OBJECT(drawArea), "expose_event", G_CALLBACK(area_on_draw), (gpointer)r);

	return win;
}

// window_show - makes a window visible
// TAKES a pointer to the window
void window_show(window* w) {
	gtk_widget_show_all((GtkWidget*)w->GTKwindow);
}

// window_on_exit - event handler for when a window is destroyed
// TAKES a pointer to the window, custom data
void window_on_exit(GtkWindow* w, gpointer data) {
	gtk_main_quit();
}

void area_on_draw(GtkWidget* w, GdkEventExpose* e, gpointer renderData) {
	GdkGC* gc = gdk_gc_new(w->window);
	
	// Black
	GdkColor* black = malloc(sizeof(GdkColor));
	gdk_color_black(gdk_colormap_get_system(), black);

	render_data* r = (render_data*)renderData;

	// Draw black rectangle
	gdk_gc_set_foreground(gc, black);
	gdk_draw_rectangle(canvas_get_gdkwindow(r->canv), gc, true, 0, 0, 640, 480);

	// Render all sprites
	spritebuffer_render_to_canvas(r->buffer, r->canv);
};

render_data* renderData_create(spritebuffer* s, canvas* c) {
	render_data* r = malloc(sizeof(render_data));
	r->buffer = s;
	r->canv = c;
	return r;
}

void tick_window_tick(void* widget, float dt) {
	GtkWidget* w = (GtkWidget*)widget;
	gtk_widget_queue_draw(w);
}
