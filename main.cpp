#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QDateTime>
#include <QTcpSocket>
#include <QHostAddress>
#include <QFile>
#include <QTextStream>
#include <QDebug>

class CashManagerWindow : public QMainWindow {
    Q_OBJECT

public:
    CashManagerWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Hitachi iH-110 - نظام متابعة واعتتماد الأرقام التسلسلية");
        resize(1050, 650);
        setLayoutDirection(Qt::RightToLeft);

        // التنسيق والتصميم (Styling)
        this->setStyleSheet(
            "QMainWindow, QWidget#centralWidget { background-color: #f4f6f9; font-family: 'Segoe UI', Arial, sans-serif; font-size: 13px; }"
            "QLineEdit { background-color: #ffffff; border: 1px solid #ced4da; border-radius: 4px; padding: 6px; color: #000; }"
            "QPushButton { background-color: #0d6efd; color: white; border: none; border-radius: 4px; padding: 6px 14px; font-weight: bold; }"
            "QPushButton:hover { background-color: #0b5ed7; }"
            "QPushButton#saveBtn { background-color: #198754; }"
            "QPushButton#saveBtn:hover { background-color: #157347; }"
            "QTableWidget { background-color: #ffffff; border: 1px solid #dee2e6; gridline-color: #e9ecef; color: #000; }"
            "QHeaderView::section { background-color: #e9ecef; color: #212529; font-weight: bold; border: none; padding: 6px; }"
        );

        QWidget *centralWidget = new QWidget(this);
        centralWidget->setObjectName("centralWidget");
        centralWidget->setAutoFillBackground(true);

        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

        // 1. شريط الشبكة وإدخال بيانات المورد
        QHBoxLayout *topLayout = new QHBoxLayout();

        QLabel *ipLabel = new QLabel("عنوان IP الآلة:");
        ipInput = new QLineEdit("10.108.48.50");
        ipInput->setFixedWidth(110);

        QLabel *portLabel = new QLabel("المنفذ:");
        portInput = new QLineEdit("448");
        portInput->setFixedWidth(50);

        connectBtn = new QPushButton("اتصال بالآلة عبر LAN");

        QLabel *supplierLabel = new QLabel("اسم المورد / العميل:");
        supplierInput = new QLineEdit();
        supplierInput->setPlaceholderText("أدخل اسم المورد هنا...");

        saveBtn = new QPushButton("حفظ وأرشفة العملية", this);
        saveBtn->setObjectName("saveBtn");

        topLayout->addWidget(ipLabel);
        topLayout->addWidget(ipInput);
        topLayout->addWidget(portLabel);
        topLayout->addWidget(portInput);
        topLayout->addWidget(connectBtn);
        topLayout->addSpacing(15);
        topLayout->addWidget(supplierLabel);
        topLayout->addWidget(supplierInput);
        topLayout->addWidget(saveBtn);

        mainLayout->addLayout(topLayout);

        // 2. شريط حالة الاتصال المرئي
        statusLabel = new QLabel("🔴 غير متصل بالآلة");
        statusLabel->setAlignment(Qt::AlignCenter);
        statusLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #842029; background-color: #f8d7da; padding: 8px; border-radius: 4px;");
        mainLayout->addWidget(statusLabel);

        // 3. جدول البيانات والعد
        tableWidget = new QTableWidget(0, 5);
        tableWidget->setHorizontalHeaderLabels({"التاريخ والوقت", "الرقم التسلسلي / البيانات", "حالة الورقة", "قرار المستخدم / الأرشفة", "اسم المورد"});
        tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        mainLayout->addWidget(tableWidget);

        setCentralWidget(centralWidget);

        // 4. إعداد الاتصال الشبكي (QTcpSocket)
        socket = new QTcpSocket(this);

        connect(connectBtn, &QPushButton::clicked, this, &CashManagerWindow::toggleConnection);
        connect(socket, &QTcpSocket::readyRead, this, &CashManagerWindow::readSocketData);
        connect(saveBtn, &QPushButton::clicked, this, &CashManagerWindow::saveAndNotify);

        connect(socket, &QTcpSocket::connected, this, [this]() {
            statusLabel->setText("🟢 متصل بنجاح بآلة العد (Hitachi iH-110) - بانتظار مرور الأوراق النقدية");
            statusLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #0f5132; background-color: #d1e7dd; padding: 8px; border-radius: 4px;");
            connectBtn->setText("قطع الاتصال");
        });

        connect(socket, &QTcpSocket::disconnected, this, [this]() {
            statusLabel->setText("🔴 تم قطع الاتصال بالآلة");
            statusLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #842029; background-color: #f8d7da; padding: 8px; border-radius: 4px;");
            connectBtn->setText("اتصال بالآلة عبر LAN");
        });

        connect(socket, &QAbstractSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
            statusLabel->setText("🔴 فشل الاتصال بالآلة: " + socket->errorString());
            statusLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #842029; background-color: #f8d7da; padding: 8px; border-radius: 4px;");
            connectBtn->setText("إعادة المحاولة");
        });
    }

private slots:
    void toggleConnection() {
        if (socket->state() == QAbstractSocket::ConnectedState) {
            socket->disconnectFromHost();
        } else {
            statusLabel->setText("⏳ جاري الاتصال بالآلة...");
            statusLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #664d03; background-color: #fff3cd; padding: 8px; border-radius: 4px;");
            socket->connectToHost(ipInput->text().trimmed(), portInput->text().trimmed().toUShort());
        }
    }

    void readSocketData() {
        QByteArray data = socket->readAll();
        QString rawString = QString::fromUtf8(data).trimmed();
        if (rawString.isEmpty()) return;

        int row = tableWidget->rowCount();
        tableWidget->insertRow(row);

        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        QString supplier = supplierInput->text().trimmed();
        if (supplier.isEmpty()) supplier = "غير محدد";

        tableWidget->setItem(row, 0, new QTableWidgetItem(timestamp));
        tableWidget->setItem(row, 1, new QTableWidgetItem(rawString));
        tableWidget->setItem(row, 2, new QTableWidgetItem("سليمة / مقبول"));
        tableWidget->setItem(row, 3, new QTableWidgetItem("تم الاعتماد"));
        tableWidget->setItem(row, 4, new QTableWidgetItem(supplier));
    }

    void saveAndNotify() {
        int totalNotes = tableWidget->rowCount();
        QString supplier = supplierInput->text().trimmed();

        if (supplier.isEmpty()) {
            QMessageBox::warning(this, "تنبيه", "يرجى إدخال اسم المورد قبل حفظ العملية.");
            return;
        }

        if (totalNotes == 0) {
            QMessageBox::warning(this, "تنبيه", "لا توجد أوراق نقدية مسجلة في الجدول للحفظ.");
            return;
        }

        // حفظ البيانات في ملف CSV
        QFile file("cash_archive.csv");
        if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream stream(&file);
            for (int i = 0; i < tableWidget->rowCount(); ++i) {
                stream << tableWidget->item(i, 0)->text() << ","
                       << tableWidget->item(i, 1)->text() << ","
                       << tableWidget->item(i, 2)->text() << ","
                       << tableWidget->item(i, 3)->text() << ","
                       << tableWidget->item(i, 4)->text() << "\n";
            }
            file.close();
        }

        // إظهار إشعار النجاح
        QMessageBox::information(this, "تمت الأرشفة بنجاح",
            QString("تمت أرشفة عملية العد بنجاح!\n\nاسم المورد: %1\nإجمالي الأوراق النقدية: %2 ورقة")
            .arg(supplier)
            .arg(totalNotes)
        );
    }

private:
    QLineEdit *ipInput;
    QLineEdit *portInput;
    QLineEdit *supplierInput;
    QPushButton *connectBtn;
    QPushButton *saveBtn;
    QLabel *statusLabel;
    QTableWidget *tableWidget;
    QTcpSocket *socket;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    CashManagerWindow window;
    window.show();
    return app.exec();
}

#include "main.moc"
