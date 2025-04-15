#include "qt/main_window.h"
#include "utils/cv_qt_convert.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QStatusBar>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QScrollArea>
#include <QDockWidget>

namespace mib
{
    namespace ui
    {

        MainWindow::MainWindow(QWidget *parent)
            : QMainWindow(parent), imageLabel(nullptr)
        {
            setWindowTitle("MIB-Studio");
            resize(1024, 768);

            setupUi();

            // Connect updateTimer to updateMetrics slot
            connect(&updateTimer, &QTimer::timeout, this, &MainWindow::updateMetrics);
            updateTimer.start(1000); // Update every second
        }

        MainWindow::~MainWindow()
        {
        }

        void MainWindow::displayImage(const cv::Mat &image)
        {
            if (image.empty() || !imageLabel)
            {
                return;
            }

            // Convert OpenCV Mat to QImage
            QImage qImage = mib::utils::matToQImage(image);
            if (!qImage.isNull())
            {
                // Set the image to the label
                imageLabel->setPixmap(QPixmap::fromImage(qImage));

                // Adjust scroll area if needed
                imageLabel->adjustSize();
            }
        }

        void MainWindow::updateMetrics()
        {
            // This method will be expanded to update metrics displays
            // For now, it just updates the status bar
            statusBar()->showMessage(QString("FPS: - | Processing Time: - ms"));
        }

        void MainWindow::setupUi()
        {
            createMenus();
            createCentralWidget();
            createImageDisplay();
            // These will be implemented later
            // createMetricsDisplay();
            // createControlPanel();

            statusBar()->showMessage("Ready");
        }

        void MainWindow::createMenus()
        {
            QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

            QAction *exitAction = new QAction(tr("E&xit"), this);
            exitAction->setShortcuts(QKeySequence::Quit);
            connect(exitAction, &QAction::triggered, this, &QWidget::close);
            fileMenu->addAction(exitAction);

            QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
            // Additional menu items will be added later
        }

        void MainWindow::createCentralWidget()
        {
            QWidget *centralWidget = new QWidget(this);
            setCentralWidget(centralWidget);

            QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
            centralWidget->setLayout(mainLayout);
        }

        void MainWindow::createImageDisplay()
        {
            // Create a scroll area for the image display
            QScrollArea *scrollArea = new QScrollArea(centralWidget());
            scrollArea->setWidgetResizable(true);

            // Create image label within scroll area
            imageLabel = new QLabel(scrollArea);
            imageLabel->setAlignment(Qt::AlignCenter);
            imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            imageLabel->setScaledContents(false);
            scrollArea->setWidget(imageLabel);

            // Add scroll area to central widget layout
            QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(centralWidget()->layout());
            if (layout)
            {
                layout->addWidget(scrollArea);
            }
        }

        void MainWindow::createMetricsDisplay()
        {
            // Will be implemented in the next phases
            // This will use QtCharts for metrics visualization
        }

        void MainWindow::createControlPanel()
        {
            // Will be implemented in the next phases
            // This will include parameter controls, etc.
        }

    } // namespace ui
} // namespace mib