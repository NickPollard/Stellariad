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

	#define MAX_SPRITES 64
	win->buffer = spritebuffer_create(MAX_SPRITES);

	renderData* rData = renderData_create(win->buffer, win->canv);
	g_signal_connect(drawArea, "expose_event", G_CALLBACK(area_on_draw), (gpointer)rData);

	gtk_container_add((GtkContainer*)win->GTKwindow, drawArea);

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

void area_on_draw(GtkWidget* w, gpointer data) {
	GdkGC* gc = gdk_gc_new(w->window);
	GdkColor* black = malloc(sizeof(GdkColor));
	gdk_color_black(gdk_colormap_get_system(), black);
	gdk_gc_set_foreground(gc, black);
	gdk_draw_rectangle(w->window, gc, true, 0, 0, 640, 480);

	spritebuffer* buffer = ((renderData*)data)->buffer;
	canvas* canv = ((renderData*)data)->canv;
	printf("area_on_draw buffer = %d\n", (int)buffer);
	printf("area_on_draw canvas = %d\n", (int)canv);
	spritebuffer_render_to_canvas(buffer, canv);
};

renderData* renderData_create(spritebuffer* s, canvas* c) {
	renderData* r = malloc(sizeof(renderData));
	r->buffer = s;
	r->canv = c;
	printf("RenderData buffer = %d\n", (int)r->buffer);
	printf("RenderData canvas = %d\n", (int)r->canv);
	return r;
}
