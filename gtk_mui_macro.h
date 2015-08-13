/*
 * gtk_mui_macro.h
 *
 */

#ifndef GTK_MUI_MACRO_H_
#define GTK_MUI_MACRO_H_

#define UIStart do { \
	void *obj = NULL;
#define UIWindow(TYPE) obj = gtk_window_new(TYPE);
#define UIChild do { \
	void *parent = obj; void *obj = NULL;
#define UIVBox obj = gtk_vbox_new(0,0); void *box = obj; \
		gtk_container_add(GTK_CONTAINER(parent), obj);
#define UIHBox obj = gtk_hbox_new(0,0); void *box = obj; \
		gtk_container_add(GTK_CONTAINER(parent), obj);
#define UIEntry obj = gtk_entry_new();
#define UIButton(LABEL) obj = gtk_button_new_with_label (LABEL);
#define UICurve(WIDTH,HEIGHT) obj = gtk_curve_new(); gtk_curve_reset(obj); gtk_widget_set_usize(obj, WIDTH, HEIGHT);
#define UIEnd } while(0);
#define UIComboBox obj = gtk_combo_box_new_text();
#define UICBAppendText(TEXT) gtk_combo_box_append_text (GTK_COMBO_BOX (obj), TEXT);
#define UILabel(TEXT) obj = gtk_label_new (TEXT);
// TODO allow adjustment's parameters when creating the GUI
#define UISpinButton  obj = gtk_spin_button_new (GTK_ADJUSTMENT(gtk_adjustment_new (50.0, 0.0, 4096.0, 1.0, 5.0, 0.0)), 1.0, 0);
#define UIConnect(EVENT, CALLBACK, PRIV) g_signal_connect(obj, EVENT, G_CALLBACK(CALLBACK), PRIV);
#define UIEventBox obj = gtk_event_box_new();
#define UIContainerAdd gtk_container_add(GTK_CONTAINER(parent), obj);

#define UIExpand TRUE
#define UIPackExpand gtk_box_pack_start (GTK_BOX(box), GTK_WIDGET(obj), TRUE, TRUE, 0);
#define UIPack gtk_box_pack_start (GTK_BOX(box), GTK_WIDGET(obj), FALSE, FALSE, 0);


#endif /* GTK_MUI_MACRO_H_ */
