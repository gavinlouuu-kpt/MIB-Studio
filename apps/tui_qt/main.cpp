#include <QApplication>
#include <thread>
#include <atomic>

#include "mib/api/MibFactory.h"
#include "src/qt/QtMibController.h"
#include "src/qt/ImageWindow.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

using namespace ftxui;

static void run_tui(std::atomic<bool>& running, mib::IMibController* controller) {
    auto status_text = std::make_shared<std::string>("stopped");

    auto renderer = Renderer([&] {
        return vbox({
            text("MIB TUI") | bold,
            separator(),
            text("Status: ") + text(*status_text),
            text("Keys: ESC exit, Space pause, A/D nav, B,P,Q,R,S,F,T,Z,X,M"),
        });
    });

    auto app = CatchEvent(renderer, [&](Event e) {
        if (!controller) return false;
        if (e.is_character()) {
            controller->onKey(e.character()[0]);
            return true;
        }
        if (e == Event::Escape) {
            controller->onKey(27);
            running = false;
            return true;
        }
        if (e == Event::Character(' ')) {
            controller->onKey(32);
            return true;
        }
        return false;
    });

    auto screen = ScreenInteractive::TerminalOutput();
    screen.Loop(app);
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    auto controller = mib::createMibController();
    mib::QtMibController qt(controller.get());
    mib::ImageWindow window(&qt);
    window.resize(800, 600);
    window.show();

    const char* imageDir = std::getenv("MIB_IMAGE_DIR");
    if (imageDir) {
        qt.setParam("image_dir", imageDir);
        qt.start();
    }

    std::atomic<bool> running{true};
    std::thread tui_thread(run_tui, std::ref(running), controller.get());

    int ret = app.exec();
    running = false;
    if (tui_thread.joinable()) tui_thread.join();
    return ret;
}


