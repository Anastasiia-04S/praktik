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

class ModuleViewer : public QWidget {
public:
    ModuleViewer() {
        // Создание таблицы для отображения модулей
        tableWidget = new QTableWidget();
        tableWidget->setColumnCount(5); // Модуль, Описание, Опции, Состояние, Действия
        tableWidget->setHorizontalHeaderLabels({"Модуль", "Описание", "Опции", "Состояние", "Действия"});

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

        // Запуск команды lsmod для получения списка модулей
        QProcess process;
        process.start("lsmod");
        process.waitForFinished();

        // Чтение вывода команды
        QString output = process.readAllStandardOutput();
        QStringList lines = output.split("\n");

        // Пропускаем первую строку (заголовок) и добавляем в таблицу данные
        for (int i = 1; i < lines.size(); ++i) {
            QString line = lines.at(i).trimmed();
            if (!line.isEmpty()) {
                QStringList columns = line.split(QRegExp("\\s+"));
                if (columns.size() > 1) {
                    QString moduleName = columns.at(0);
                    int row = tableWidget->rowCount();
                    tableWidget->insertRow(row);
                    tableWidget->setItem(row, 0, new QTableWidgetItem(moduleName));
                    tableWidget->setItem(row, 1, new QTableWidgetItem(getModuleDescription(moduleName))); // Описание модуля
                    tableWidget->setItem(row, 2, new QTableWidgetItem(getModuleOptions(moduleName))); // Опции модуля
                    tableWidget->setItem(row, 3, new QTableWidgetItem(isModuleLoaded(moduleName) ? "Загружен" : "Не загружен"));

                    // Кнопки для загрузки/выгрузки модулей
                    QPushButton* loadButton = new QPushButton("Загрузить");
                    connect(loadButton, &QPushButton::clicked, this, [this, moduleName] { loadModule(moduleName); });

                    QPushButton* unloadButton = new QPushButton("Выгрузить");
                    connect(unloadButton, &QPushButton::clicked, this, [this, moduleName] { unloadModule(moduleName); });

                    tableWidget->setCellWidget(row, 4, loadButton);
                    if (isModuleLoaded(moduleName)) {
                        loadButton->setEnabled(false);
                    } else {
                        unloadButton->setEnabled(false);
                    }
                }
            }
        }
    }

    QString getModuleDescription(const QString& moduleName) {
        // Получение информации о модуле с помощью команды modinfo
        QProcess process;
        process.start("modinfo", QStringList() << moduleName);
        process.waitForFinished();

        QString output = process.readAllStandardOutput();
        QStringList lines = output.split("\n");

        // Поиск строки с описанием модуля
        for (const QString& line : lines) {
            if (line.startsWith("description")) {
                return line.split(":").at(1).trimmed();
            }
        }
        return "Описание не найдено";
    }

    QString getModuleOptions(const QString& moduleName) {
        // Получение информации о модуле с помощью команды modinfo
        QProcess process;
        process.start("modinfo", QStringList() << moduleName);
        process.waitForFinished();

        QString output = process.readAllStandardOutput();
        QStringList lines = output.split("\n");

        // Поиск строк, содержащих опции
        QString options;
        for (const QString& line : lines) {
            if (line.startsWith("parm")) {
                options += line.split(":").at(1).trimmed() + "; ";
            }
        }

        return options.isEmpty() ? "Нет опций" : options;
    }

    bool isModuleLoaded(const QString& moduleName) {
        // Проверка, загружен ли модуль
        QProcess process;
        process.start("lsmod");
        process.waitForFinished();

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
        process.start("modprobe", QStringList() << moduleName);
        process.waitForFinished();

        if (process.exitCode() != 0) {
            QMessageBox::critical(this, "Ошибка", "Не удалось загрузить модуль " + moduleName);
        } else {
            QMessageBox::information(this, "Успех", "Модуль " + moduleName + " успешно загружен.");
            updateModuleList();
        }
    }

    void unloadModule(const QString& moduleName) {
        QProcess process;
        process.start("modprobe", QStringList() << "-r" << moduleName);
        process.waitForFinished();

        if (process.exitCode() != 0) {
            QMessageBox::critical(this, "Ошибка", "Не удалось выгрузить модуль " + moduleName);
        } else {
            QMessageBox::information(this, "Успех", "Модуль " + moduleName + " успешно выгружен.");
            updateModuleList();
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
