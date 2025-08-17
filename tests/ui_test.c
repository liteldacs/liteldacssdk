#include <gtk/gtk.h>

GtkWidget *window;
GtkWidget *header_label;
GtkWidget *text_view;
GtkWidget *entry;
GtkTextBuffer *buffer;

static void on_entry_activate(GtkEntry *entry, gpointer user_data) {
    const char *text = gtk_entry_get_text(entry);

    if (g_strcmp0(text, "quit") == 0) {
        gtk_main_quit();
        return;
    }

    // 添加到文本视图
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(buffer, &iter);

    gchar *output = g_strdup_printf("> %s\nYou typed: %s\n", text, text);
    gtk_text_buffer_insert(buffer, &iter, output, -1);
    g_free(output);

    // 清空输入框
    gtk_entry_set_text(entry, "");

    // 滚动到底部
    GtkTextMark *mark = gtk_text_buffer_get_insert(buffer);
    gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(text_view), mark);
}

static void activate(GtkApplication *app, gpointer user_data) {
    // 创建主窗口
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "UI Test");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

    // 创建垂直布局容器
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // 创建固定头部区域
    header_label = gtk_label_new("hello world");
    gtk_widget_set_size_request(header_label, -1, 60);

    // 设置头部样式
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "label { background-color: #0066cc; color: white; "
        "padding: 20px; font-size: 16px; }", -1, NULL);

    GtkStyleContext *context = gtk_widget_get_style_context(header_label);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    gtk_box_pack_start(GTK_BOX(vbox), header_label, FALSE, FALSE, 0);

    // 创建分隔线
    GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 0);

    // 创建滚动窗口和文本视图
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    // 设置初始文本
    gtk_text_buffer_set_text(buffer, "Welcome! Type 'quit' to exit.\n", -1);

    gtk_container_add(GTK_CONTAINER(scrolled), text_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

    // 创建输入框
    entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter command...");
    g_signal_connect(entry, "activate", G_CALLBACK(on_entry_activate), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 5);

    gtk_widget_show_all(window);
    gtk_widget_grab_focus(entry);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.uitest", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}