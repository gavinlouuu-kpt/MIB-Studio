#pragma once

#include <QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <memory>
#include "qt_ui/ImageView.h"

namespace Ui {
class MainWindow;
}

/**
 * @brief Main application window
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

private slots:
    /**
     * @brief Open file action handler
     */
    void on_actionOpen_triggered();
    
    /**
     * @brief Save file action handler
     */
    void on_actionSave_triggered();
    
    /**
     * @brief Exit application action handler
     */
    void on_actionExit_triggered();
    
    /**
     * @brief Run mock sample action handler
     */
    void on_actionRunMockSample_triggered();
    
    /**
     * @brief Run live sample action handler
     */
    void on_actionRunLiveSample_triggered();
    
    /**
     * @brief Convert saved images action handler
     */
    void on_actionConvertSavedImages_triggered();
    
    /**
     * @brief Open file button handler
     */
    void on_openFileButton_clicked();
    
    /**
     * @brief Open folder button handler
     */
    void on_openFolderButton_clicked();
    
    /**
     * @brief Run mock sample button handler
     */
    void on_runMockSampleButton_clicked();
    
    /**
     * @brief Run live sample button handler
     */
    void on_runLiveSampleButton_clicked();
    
    /**
     * @brief Convert saved images button handler
     */
    void on_convertSavedImagesButton_clicked();
    
    /**
     * @brief About action handler
     */
    void on_actionAbout_triggered();
    
    /**
     * @brief Preferences action handler
     */
    void on_actionPreferences_triggered();

private:
    /**
     * @brief Initialize the UI components
     */
    void initializeUi();
    
    /**
     * @brief Connect signals and slots
     */
    void connectSignals();
    
    /**
     * @brief Load an image file
     * @param filePath Path to the image file
     */
    void loadImageFile(const QString& filePath);
    
    /**
     * @brief Load a folder of images
     * @param folderPath Path to the folder
     */
    void loadImageFolder(const QString& folderPath);
    
    /**
     * @brief Run the mock sample
     * @param folderPath Path to sample folder
     */
    void runMockSample(const QString& folderPath);
    
    /**
     * @brief Run the live sample
     */
    void runLiveSample();
    
    /**
     * @brief Convert saved images
     * @param filePath Path to saved image file
     * @param outputDir Path to output directory
     */
    void convertSavedImages(const QString& filePath, const QString& outputDir);

    std::unique_ptr<Ui::MainWindow> ui;
    ImageView* imageView;
}; 