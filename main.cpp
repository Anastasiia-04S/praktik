#include <QApplication>
#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QStringList>
#include <QProcess>
#include <QMessageBox>
#include <QTextStream>
#include <QDebug>

class ModuleViewer : public QWidget {
public:
    ModuleViewer() {
        // Создание таблицы для отображения модулей
        tableWidget = new QTableWidget();
        tableWidget->setColumnCount(6); // Модуль, Описание, Опции, Состояние, Действия(2 кнопки)
        tableWidget->setHorizontalHeaderLabels({"Модуль", "Описание", "Опции", "Состояние", "Загрузить", "Выгрузить"});

        // Настройка стилей
        tableWidget->setStyleSheet("QTableWidget { font-size: 14px; }"
                                   "QHeaderView::section { background-color: #f0f0f0; font-weight: bold; }"
                                   "QPushButton { background-color: #4CAF50; color: white; padding: 5px 10px; border-radius: 5px; }"
                                   "QPushButton:hover { background-color: #45a049; }");

        // Кнопка для обновления списка модулей
        QPushButton* refreshButton = new QPushButton("Обновить модули");
        connect(refreshButton, &QPushButton::clicked, this, &ModuleViewer::updateModuleList);

        // Размещение виджетов в основном окне
        QVBoxLayout* layout = new QVBoxLayout();
        layout->addWidget(tableWidget);
        layout->addWidget(refreshButton);
        setLayout(layout);

        // Инициализация с актуальными данными
        updateModuleList();
    }

    void updateModuleList() {
        // Очистить таблицу перед обновлением
        tableWidget->setRowCount(0);

        // Получение списка всех модулей
        QStringList moduleFiles = getAllModules();

        if (moduleFiles.isEmpty()) {
            QMessageBox::information(this, "Информация", "Модули не найдены или ошибка при их поиске.");
            return;
        }

        // Обрабатываем каждый найденный модуль
        for (const QString& modulePath : moduleFiles) {
            QString moduleName = modulePath.section('/', -1).section('.', 0, 0); // Получаем имя модуля
            if (!moduleName.isEmpty()) {
                int row = tableWidget->rowCount();
                tableWidget->insertRow(row);
                tableWidget->setItem(row, 0, new QTableWidgetItem(moduleName));
                tableWidget->setItem(row, 1, new QTableWidgetItem(getModuleDescription(moduleName))); // Описание
                tableWidget->setItem(row, 2, new QTableWidgetItem(getModuleOptions(moduleName))); // Опции
                tableWidget->setItem(row, 3, new QTableWidgetItem(isModuleLoaded(moduleName) ? "Загружен" : "Не загружен"));

                // Создание кнопок для загрузки и выгрузки
                QPushButton* loadButton = new QPushButton("Загрузить");
                QPushButton* unloadButton = new QPushButton("Выгрузить");

                connect(loadButton, &QPushButton::clicked, this, [this, moduleName] { loadModule(moduleName); });
                connect(unloadButton, &QPushButton::clicked, this, [this, moduleName] { unloadModule(moduleName); });

                // Установить кнопки в таблицу в соответствующие ячейки
                tableWidget->setCellWidget(row, 4, loadButton);
                tableWidget->setCellWidget(row, 5, unloadButton);

                // Обновление состояний кнопок в зависимости от того, загружен ли модуль
                if (isModuleLoaded(moduleName)) {
                    loadButton->setEnabled(false); // Если модуль уже загружен, кнопка "Загрузить" отключена
                    unloadButton->setEnabled(true); // Кнопка "Выгрузить" активна
                } else {
                    loadButton->setEnabled(true); // Кнопка "Загрузить" активна
                    unloadButton->setEnabled(false); // Если модуль не загружен, кнопка "Выгрузить" отключена
                }
            }
        }
    }

    QStringList getAllModules() {
        // Получаем версию ядра
        QProcess unameProcess;
        unameProcess.start("uname", QStringList() << "-r");
        unameProcess.waitForFinished();
        QString kernelVersion = unameProcess.readAllStandardOutput().trimmed();

        // Формируем правильный путь к модулям
        QString modulePath = "/lib/modules/" + kernelVersion + "/kernel/";

        // Запуск find для поиска файлов модулей
        QProcess process;
        process.start("find", QStringList() << modulePath << "-name" << "*.ko");
        process.waitForFinished();

        if (process.exitCode() != 0) {
            qDebug() << "Ошибка при поиске модулей.";
            return QStringList();
        }

        QString output = process.readAllStandardOutput();
        QStringList moduleFiles = output.split("\n", QString::SkipEmptyParts);

        // Для отладки
        qDebug() << "Modules found: " << moduleFiles;

        return moduleFiles;
    }

    QString getModuleDescription(const QString& moduleName) {
        return getModuleInfo(moduleName, "description");
    }

    QString getModuleOptions(const QString& moduleName) {
        return getModuleInfo(moduleName, "parm");
    }

    QString getModuleInfo(const QString& moduleName, const QString& field) {
        QProcess process;
        process.start("modinfo", QStringList() << moduleName);
        process.waitForFinished();

        if (process.exitCode() != 0) {
            qDebug() << "Ошибка при получении информации о модуле " << moduleName;
            return "Информация не найдена";
        }

        QString output = process.readAllStandardOutput();
        QStringList lines = output.split("\n");

        for (const QString& line : lines) {
            if (line.startsWith(field)) {
                return line.split(":").at(1).trimmed();
            }
        }
        return "Информация не найдена";
    }

    bool isModuleLoaded(const QString& moduleName) {
        QProcess process;
        process.start("lsmod");
        process.waitForFinished();

        if (process.exitCode() != 0) {
            qDebug() << "Ошибка при проверке состояния модулей.";
            return false;
        }

        QString output = process.readAllStandardOutput();
        QStringList lines = output.split("\n");

        for (const QString& line : lines) {
            if (line.contains(moduleName)) {
                return true;
            }
        }
        return false;
    }

    void loadModule(const QString& moduleName) {
        QProcess process;
        process.start("beesu", QStringList() << "modprobe" << moduleName);  // Изменено с "sudo"
        process.waitForFinished();

        if (process.exitCode() != 0) {
            QMessageBox::critical(this, "Ошибка", "Не удалось загрузить модуль " + moduleName);
        } else {
            QMessageBox::information(this, "Успех", "Модуль " + moduleName + " успешно загружен.");
            QMetaObject::invokeMethod(this, "updateModuleList", Qt::QueuedConnection); // Обновление списка после загрузки
        }
    }

    void unloadModule(const QString& moduleName) {
        QProcess process;
        process.start("beesu", QStringList() << "modprobe" << "-r" << moduleName);  // Изменено с "sudo"
        process.waitForFinished();

        if (process.exitCode() != 0) {
            QMessageBox::critical(this, "Ошибка", "Не удалось выгрузить модуль " + moduleName);
        } else {
            QMessageBox::information(this, "Успех", "Модуль " + moduleName + " успешно выгружен.");
            QMetaObject::invokeMethod(this, "updateModuleList", Qt::QueuedConnection); // Обновление списка после выгрузки
        }
    }

private:
    QTableWidget* tableWidget;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    ModuleViewer viewer;
    viewer.setWindowTitle("Управление ядерными модулями");
    viewer.show();

    return app.exec();
}
