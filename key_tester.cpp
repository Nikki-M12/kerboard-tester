#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

class KeyboardTester {
private:
    HWND m_hwnd;
    HWND m_statusLabel;
    HWND m_keyLabel;
    HWND m_versionLabel;
    HWND m_osLabel;
    HBRUSH m_bgBrush;
    HBRUSH m_grayBrush;
    HBRUSH m_greenBrush;
    std::atomic<bool> m_beepRunning;
    std::thread m_beepThread;
    bool m_keyPressed;
    std::string m_currentKey;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        KeyboardTester* tester = nullptr;
        if (msg == WM_NCCREATE) {
            CREATESTRUCT* create = reinterpret_cast<CREATESTRUCT*>(lParam);
            tester = reinterpret_cast<KeyboardTester*>(create->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(tester));
        } else {
            tester = reinterpret_cast<KeyboardTester*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }
        
        if (tester) {
            switch (msg) {
                case WM_CREATE:
                    tester->createControls(hwnd);
                    break;
                case WM_PAINT:
                    tester->drawIndicator();
                    break;
                case WM_KEYDOWN:
                    tester->onKeyPress(static_cast<int>(wParam));
                    break;
                case WM_KEYUP:
                    tester->onKeyRelease();
                    break;
                case WM_DESTROY:
                    tester->stopBeep();
                    PostQuitMessage(0);
                    break;
                case WM_CTLCOLORSTATIC: {
                    HDC hdc = (HDC)wParam;
                    SetBkColor(hdc, RGB(240, 240, 240));
                    return (LRESULT)tester->m_bgBrush;
                }
            }
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    void createControls(HWND hwnd) {
        CreateWindow("STATIC", "Keyboard Tester", 
                    WS_VISIBLE | WS_CHILD | SS_CENTER,
                    0, 20, 400, 30, hwnd, NULL, NULL, NULL);
        
        m_statusLabel = CreateWindow("STATIC", "Press a key", 
                                   WS_VISIBLE | WS_CHILD | SS_CENTER,
                                   0, 150, 400, 20, hwnd, NULL, NULL, NULL);
        
        CreateWindow("STATIC", "Key detected:", 
                    WS_VISIBLE | WS_CHILD | SS_CENTER,
                    0, 180, 400, 20, hwnd, NULL, NULL, NULL);
        
        m_keyLabel = CreateWindow("STATIC", "", 
                                WS_VISIBLE | WS_CHILD | SS_CENTER | SS_CENTERIMAGE,
                                0, 200, 400, 30, hwnd, NULL, NULL, NULL);
        
        m_versionLabel = CreateWindow("STATIC", "Version: 1.0", 
                                    WS_VISIBLE | WS_CHILD,
                                    10, 270, 100, 20, hwnd, NULL, NULL, NULL);
        
        m_osLabel = CreateWindow("STATIC", "OS: Windows", 
                               WS_VISIBLE | WS_CHILD | SS_RIGHT,
                               200, 270, 190, 20, hwnd, NULL, NULL, NULL);
        
        HFONT font = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, 
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                               DEFAULT_QUALITY, DEFAULT_PITCH, "Arial");
        SendMessage(m_keyLabel, WM_SETFONT, (WPARAM)font, TRUE);
    }

    void drawIndicator() {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);
        
        HBRUSH brush = m_keyPressed ? m_greenBrush : m_grayBrush;
        SelectObject(hdc, brush);
        SelectObject(hdc, GetStockObject(NULL_PEN));
        Ellipse(hdc, 170, 80, 230, 140);
        
        EndPaint(m_hwnd, &ps);
    }

    void onKeyPress(int vk) {
        if (!m_keyPressed) {
            m_keyPressed = true;
            m_currentKey = virtualKeyToString(vk);
            SetWindowText(m_statusLabel, "Key pressed");
            SetWindowText(m_keyLabel, m_currentKey.c_str());
            InvalidateRect(m_hwnd, NULL, TRUE);
            startBeep();
        }
    }

    void onKeyRelease() {
        m_keyPressed = false;
        SetWindowText(m_statusLabel, "Press a key");
        SetWindowText(m_keyLabel, "");
        InvalidateRect(m_hwnd, NULL, TRUE);
        stopBeep();
    }

    std::string virtualKeyToString(int vk) {
        switch (vk) {
            case VK_SPACE: return "Space";
            case VK_RETURN: return "Enter";
            case VK_BACK: return "Backspace";
            case VK_TAB: return "Tab";
            case VK_ESCAPE: return "Escape";
            case VK_LEFT: return "Left Arrow";
            case VK_RIGHT: return "Right Arrow";
            case VK_UP: return "Up Arrow";
            case VK_DOWN: return "Down Arrow";
            case VK_HOME: return "Home";
            case VK_END: return "End";
            case VK_PRIOR: return "Page Up";
            case VK_NEXT: return "Page Down";
            case VK_INSERT: return "Insert";
            case VK_DELETE: return "Delete";
            case VK_F1: return "F1";
            case VK_F2: return "F2";
            case VK_F3: return "F3";
            case VK_F4: return "F4";
            case VK_F5: return "F5";
            case VK_F6: return "F6";
            case VK_F7: return "F7";
            case VK_F8: return "F8";
            case VK_F9: return "F9";
            case VK_F10: return "F10";
            case VK_F11: return "F11";
            case VK_F12: return "F12";
            case VK_SHIFT: return "Shift";
            case VK_CONTROL: return "Control";
            case VK_MENU: return "Alt";
            default:
                if (vk >= 'A' && vk <= 'Z') return std::string(1, (char)vk);
                if (vk >= '0' && vk <= '9') return std::string(1, (char)vk);
                return "Key " + std::to_string(vk);
        }
    }

    void continuousBeep() {
        while (m_beepRunning) {
            Beep(1000, 100);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    void startBeep() {
        if (!m_beepRunning) {
            m_beepRunning = true;
            m_beepThread = std::thread(&KeyboardTester::continuousBeep, this);
        }
    }

    void stopBeep() {
        if (m_beepRunning) {
            m_beepRunning = false;
            if (m_beepThread.joinable()) {
                m_beepThread.join();
            }
        }
    }

public:
    KeyboardTester() : m_beepRunning(false), m_keyPressed(false) {
        m_bgBrush = CreateSolidBrush(RGB(240, 240, 240));
        m_grayBrush = CreateSolidBrush(RGB(204, 204, 204));
        m_greenBrush = CreateSolidBrush(RGB(46, 139, 87));
        
        WNDCLASS wc = {};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = "KeyboardTester";
        wc.hbrBackground = m_bgBrush;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        
        RegisterClass(&wc);
        
        m_hwnd = CreateWindow("KeyboardTester", "Keyboard Tester",
                             WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                             CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
                             NULL, NULL, GetModuleHandle(NULL), this);
    }

    ~KeyboardTester() {
        stopBeep();
        DeleteObject(m_bgBrush);
        DeleteObject(m_grayBrush);
        DeleteObject(m_greenBrush);
    }

    void run() {
        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
        
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
};

#else

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

class KeyboardTester {
private:
    GtkWidget* m_window;
    GtkWidget* m_drawing_area;
    GtkWidget* m_status_label;
    GtkWidget* m_key_label;
    std::atomic<bool> m_beepRunning;
    std::thread m_beepThread;
    bool m_keyPressed;

    static gboolean on_draw(GtkWidget* widget, cairo_t* cr, gpointer user_data) {
        KeyboardTester* tester = static_cast<KeyboardTester*>(user_data);
        tester->drawIndicator(cr);
        return FALSE;
    }

    static gboolean on_key_press(GtkWidget* widget, GdkEventKey* event, gpointer user_data) {
        KeyboardTester* tester = static_cast<KeyboardTester*>(user_data);
        tester->onKeyPress(event);
        return TRUE;
    }

    static gboolean on_key_release(GtkWidget* widget, GdkEventKey* event, gpointer user_data) {
        KeyboardTester* tester = static_cast<KeyboardTester*>(user_data);
        tester->onKeyRelease();
        return TRUE;
    }

    static void on_window_destroy(GtkWidget* widget, gpointer user_data) {
        gtk_main_quit();
    }

    void drawIndicator(cairo_t* cr) {
        if (m_keyPressed) {
            cairo_set_source_rgb(cr, 0.18, 0.55, 0.34);
        } else {
            cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
        }
        cairo_arc(cr, 30, 30, 25, 0, 2 * G_PI);
        cairo_fill(cr);
    }

    void onKeyPress(GdkEventKey* event) {
        if (!m_keyPressed) {
            m_keyPressed = true;
            std::string keyStr = keyvalToString(event->keyval);
            gtk_label_set_text(GTK_LABEL(m_status_label), "Key pressed");
            gtk_label_set_text(GTK_LABEL(m_key_label), keyStr.c_str());
            gtk_widget_queue_draw(m_drawing_area);
            startBeep();
        }
    }

    void onKeyRelease() {
        m_keyPressed = false;
        gtk_label_set_text(GTK_LABEL(m_status_label), "Press a key");
        gtk_label_set_text(GTK_LABEL(m_key_label), "");
        gtk_widget_queue_draw(m_drawing_area);
        stopBeep();
    }

    std::string keyvalToString(guint keyval) {
        switch (keyval) {
            case GDK_KEY_space: return "Space";
            case GDK_KEY_Return: return "Enter";
            case GDK_KEY_BackSpace: return "Backspace";
            case GDK_KEY_Tab: return "Tab";
            case GDK_KEY_Escape: return "Escape";
            case GDK_KEY_Left: return "Left Arrow";
            case GDK_KEY_Right: return "Right Arrow";
            case GDK_KEY_Up: return "Up Arrow";
            case GDK_KEY_Down: return "Down Arrow";
            case GDK_KEY_Home: return "Home";
            case GDK_KEY_End: return "End";
            case GDK_KEY_Page_Up: return "Page Up";
            case GDK_KEY_Page_Down: return "Page Down";
            case GDK_KEY_Insert: return "Insert";
            case GDK_KEY_Delete: return "Delete";
            case GDK_KEY_F1: return "F1";
            case GDK_KEY_F2: return "F2";
            case GDK_KEY_F3: return "F3";
            case GDK_KEY_F4: return "F4";
            case GDK_KEY_F5: return "F5";
            case GDK_KEY_F6: return "F6";
            case GDK_KEY_F7: return "F7";
            case GDK_KEY_F8: return "F8";
            case GDK_KEY_F9: return "F9";
            case GDK_KEY_F10: return "F10";
            case GDK_KEY_F11: return "F11";
            case GDK_KEY_F12: return "F12";
            case GDK_KEY_Shift_L: case GDK_KEY_Shift_R: return "Shift";
            case GDK_KEY_Control_L: case GDK_KEY_Control_R: return "Control";
            case GDK_KEY_Alt_L: case GDK_KEY_Alt_R: return "Alt";
            case GDK_KEY_Super_L: case GDK_KEY_Super_R: return "Super";
            default:
                const char* name = gdk_keyval_name(keyval);
                if (name) {
                    return std::string(name);
                }
                return "Key " + std::to_string(keyval);
        }
    }

    void continuousBeep() {
        while (m_beepRunning) {
            std::cout << "\a" << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    void startBeep() {
        if (!m_beepRunning) {
            m_beepRunning = true;
            m_beepThread = std::thread(&KeyboardTester::continuousBeep, this);
        }
    }

    void stopBeep() {
        if (m_beepRunning) {
            m_beepRunning = false;
            if (m_beepThread.joinable()) {
                m_beepThread.join();
            }
        }
    }

public:
    KeyboardTester() : m_beepRunning(false), m_keyPressed(false) {
        gtk_init(0, NULL);
        
        m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(m_window), "Keyboard Tester");
        gtk_window_set_default_size(GTK_WINDOW(m_window), 400, 300);
        gtk_window_set_resizable(GTK_WINDOW(m_window), FALSE);
        
        GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_container_set_border_width(GTK_CONTAINER(m_window), 20);
        gtk_container_add(GTK_CONTAINER(m_window), main_box);
        
        GtkWidget* title_label = gtk_label_new("Keyboard Tester");
        PangoAttrList* attrs = pango_attr_list_new();
        pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
        pango_attr_list_insert(attrs, pango_attr_foreground_new(0, 0, 65535));
        gtk_label_set_attributes(GTK_LABEL(title_label), attrs);
        pango_attr_list_unref(attrs);
        gtk_widget_set_halign(title_label, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(main_box), title_label, FALSE, FALSE, 0);
        
        m_drawing_area = gtk_drawing_area_new();
        gtk_widget_set_size_request(m_drawing_area, 60, 60);
        gtk_widget_set_halign(m_drawing_area, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(main_box), m_drawing_area, FALSE, FALSE, 10);
        
        m_status_label = gtk_label_new("Press a key");
        gtk_widget_set_halign(m_status_label, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(main_box), m_status_label, FALSE, FALSE, 5);
        
        GtkWidget* detected_label = gtk_label_new("Key detected:");
        gtk_widget_set_halign(detected_label, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(main_box), detected_label, FALSE, FALSE, 10);
        
        m_key_label = gtk_label_new("");
        gtk_widget_set_halign(m_key_label, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(main_box), m_key_label, FALSE, FALSE, 5);
        
        GtkWidget* spacer = gtk_label_new("");
        gtk_box_pack_start(GTK_BOX(main_box), spacer, TRUE, TRUE, 0);
        
        GtkWidget* info_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_box_pack_start(GTK_BOX(main_box), info_box, FALSE, FALSE, 0);
        
        GtkWidget* version_label = gtk_label_new("Version: 1.0");
        gtk_box_pack_start(GTK_BOX(info_box), version_label, FALSE, FALSE, 0);
        
        GtkWidget* os_label = gtk_label_new("OS: Linux");
        gtk_box_pack_end(GTK_BOX(info_box), os_label, FALSE, FALSE, 0);
        
        g_signal_connect(m_drawing_area, "draw", G_CALLBACK(on_draw), this);
        g_signal_connect(m_window, "key-press-event", G_CALLBACK(on_key_press), this);
        g_signal_connect(m_window, "key-release-event", G_CALLBACK(on_key_release), this);
        g_signal_connect(m_window, "destroy", G_CALLBACK(on_window_destroy), this);
        
        gtk_widget_set_can_focus(m_window, TRUE);
        gtk_widget_grab_focus(m_window);
    }

    ~KeyboardTester() {
        stopBeep();
    }

    void run() {
        gtk_widget_show_all(m_window);
        gtk_main();
    }
};

#endif

int main() {
    KeyboardTester tester;
    tester.run();
    return 0;
}
