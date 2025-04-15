#ifndef MIB_STUDIO_MAIN_WINDOW_H
#define MIB_STUDIO_MAIN_WINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <opencv2/core/mat.hpp>

namespace mib
{
    namespace ui
    {

        /**
         * @brief Main application window for MIB-Studio
         *
         * This class provides the main Qt window for the application,
         * integrating image display, controls, and metrics visualization.
         */
        class MainWindow : public QMainWindow
        {
            Q_OBJECT

        public:
            /**
             * @brief Constructor
             * @param parent Parent widget
             */
            explicit MainWindow(QWidget *parent = nullptr);

            /**
             * @brief Destructor
             */
            ~MainWindow();

            /**
             * @brief Display OpenCV image in the window
             * @param image OpenCV image to display
             */
            void displayImage(const cv::Mat &image);

        private slots:
            /**
             * @brief Update UI elements with current metrics
             */
            void updateMetrics();

        private:
            /**
             * @brief Setup the UI components
             */
            void setupUi();

            /**
             * @brief Create menu bar and actions
             */
            void createMenus();

            /**
             * @brief Create central widget with layout
             */
            void createCentralWidget();

            /**
             * @brief Create image display area
             */
            void createImageDisplay();

            /**
             * @brief Create metrics display area
             */
            void createMetricsDisplay();

            /**
             * @brief Create control panel
             */
            void createControlPanel();

            // UI Elements
            QLabel *imageLabel;
            QTimer updateTimer;
        };

    } // namespace ui
} // namespace mib

#endif // MIB_STUDIO_MAIN_WINDOW_H