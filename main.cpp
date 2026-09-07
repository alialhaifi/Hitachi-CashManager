#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTcpSocket>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QSet>
#include <QHeaderView>
#include <QMessageBox>

// ============================================================================
// كلاس إدارة قاعدة البيانات والأرشيف (Core Logic)
// ============================================================================
class CashDatabaseManager {
private:
    QString filename;
    QSet<QString> registeredSerials;

public:
    CashDatabaseManager(const QString &archiveFile = "cash_database.csv") 
        : filename(archiveFile) {
        loadDatabase();
    }

    // تحميل الأرقام المسجلة سابقاً من ملف الأرشيف
    void loadDatabase() {
        registeredSerials.clear();
        QFile file(filename);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine();
                QStringList parts = line.split(",");
                if (parts.size() >= 2) {
                    QString serial = parts[1].remove("\"").trimmed();
                    if (!serial.isEmpty()) {
                        registeredSerials.insert(serial);
                    }
                }
            }
            file.close();
        }
    }

    // التحقق مما إذا كان الرقم التسلسلي مسجلاً مسبقاً
    bool isSerialRegistered(const QString &serial) const {
        return registeredSerials.contains(serial);
    }

    // حفظ العملية وتحديث الأرشيف بالقرار المتخذ
    void registerAndSaveSerial(const QString &timestamp, const QString &serial, const QString &statusAction) {
        if (statusAction == "تمت الموافقة والتسجيل") {
            registeredSerials.insert(serial);
        }

        QFile file(filename);
        if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out << "\"" << timestamp << "\",\"" << serial << "\",\"" << statusAction << "\"\n";
            file.close();
        }
    }
};

// ============================================================================
// كلاس الواجهة الرسومية والاتصال الشبكي (GUI & Networking)
// ============================================================================
class MainWindow : public QMainWindow {
    Q_OBJECT

private:
    QTcpSocket *tcpSocket;
    CashDatabaseManager dbManager;

    // عناصر الواجهة الرسومية
    QLineEdit *ipInput;
    QLineEdit *portInput;
    QPushButton *connectBtn;
    QLabel *statusLabel;
    QLabel *alertBanner;
    QTableWidget *dataTable;

public:
    MainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        tcpSocket = new QTcpSocket(this);

        // إعداد خصائص النافذة الرئيسية والاتجاه من اليمين إلى اليسار
        setWindowTitle("نظام متابعة واعتماد الأرقام التسلسلية - Hitachi iH-110");
        resize(950, 650);
        setLayoutDirection(Qt::RightToLeft);

        QWidget *centralWidget = new QWidget(this);
        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

        // 1. شريط إعدادات الاتصال بالشبكة
        QHBoxLayout *connLayout = new QHBoxLayout();
        ipInput = new QLineEdit("192.168.1.150");
        portInput = new QLineEdit("8000");
        connectBtn = new QPushButton("اتصال بالآلة عبر LAN");
        connectBtn->setStyleSheet("padding: 6px; font-weight: bold;");

        connLayout->addWidget(new QLabel("عنوان IP للآلة:"));
        connLayout->addWidget(ipInput);
        connLayout->addWidget(new QLabel("المنفذ (Port):"));
        connLayout->addWidget(portInput);
        connLayout->addWidget(connectBtn);
        mainLayout->addLayout(connLayout);

        // 2. شريط حالة الاتصال
        statusLabel = new QLabel("حالة الشبكة: غير متصل");
        statusLabel->setStyleSheet("color: #555555; font-size: 12px;");
        mainLayout->addWidget(statusLabel);

        // 3. بنر التنبيهات الملون عند العد
        alertBanner = new QLabel("جاهز للعد واستقبال البيانات...");
        alertBanner->setAlignment(Qt::AlignCenter);
        alertBanner->setStyleSheet("background-color: #e0e0e0; color: #333333; font-weight: bold; font-size: 15px; padding: 10px; border-radius: 4px;");
        mainLayout->addWidget(alertBanner);

        // 4. جدول عرض نتائج العد والأرقام التسلسلية
        dataTable = new QTableWidget(0, 4);
        QStringList headers = {"التاريخ والوقت", "الرقم التسلسلي / البيانات", "حالة الورقة", "قرار المستخدم / الأرشفة"};
        dataTable->setHorizontalHeaderLabels(headers);
        dataTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        dataTable->setAlternatingRowColors(true);
        mainLayout->addWidget(dataTable);

        setCentralWidget(centralWidget);

        // ربط الإشارات بالأحداث (Signals & Slots)
        connect(connectBtn, &QPushButton::clicked, this, &MainWindow::toggleConnection);
        connect(tcpSocket, &QTcpSocket::readyRead, this, &MainWindow::onDataReceived);
        connect(tcpSocket, &QTcpSocket::connected, this, &MainWindow::onConnected);
        connect(tcpSocket, &QTcpSocket::disconnected, this, &MainWindow::onDisconnected);
    }

private slots:
    void toggleConnection() {
        if (tcpSocket->state() == QAbstractSocket::UnconnectedState) {
            statusLabel->setText("جاري الاتصال بالآلة...");
            tcpSocket->connectToHost(ipInput->text(), portInput->text().toUShort());
        } else {
            tcpSocket->disconnectFromHost();
        }
    }

    void onConnected() {
        statusLabel->setText("حالة الشبكة: متصل بنجاح بالآلة (" + ipInput->text() + ")");
        connectBtn->setText("قطع الاتصال");
        connectBtn->setStyleSheet("padding: 6px; font-weight: bold; background-color: #ffcccc;");
    }

    void onDisconnected() {
        statusLabel->setText("حالة الشبكة: تم قطع الاتصال");
        connectBtn->setText("اتصال بالآلة عبر LAN");
        connectBtn->setStyleSheet("padding: 6px; font-weight: bold;");
    }

    // استقبال البيانات عند مرور ورقة نقدية بالآلة
    void onDataReceived() {
        while (tcpSocket->canReadLine()) {
            QByteArray rawData = tcpSocket->readLine().trimmed();
            if (!rawData.isEmpty()) {
                processCashItem(QString::fromUtf8(rawData));
            }
        }
    }

private:
    void processCashItem(const QString &serialData) {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        
        // 1. فحص هل الورقة مسجلة سابقاً أم لا
        bool isAlreadyRegistered = dbManager.isSerialRegistered(serialData);
        QString actionStatus;

        if (!isAlreadyRegistered) {
            // === حالة ورقة جديدة: إظهار تنبيه + نافذة طلب الموافقة ===
            alertBanner->setText("⚠️ تنبيه: ورقة جديدة بحاجة لموافقتك للتسجيل! (" + serialData + ")");
            alertBanner->setStyleSheet("background-color: #ff3333; color: white; font-weight: bold; font-size: 15px; padding: 10px; border-radius: 4px;");

            // إنشاء نافذة التأكيد والموافقة
            QMessageBox confirmBox(this);
            confirmBox.setLayoutDirection(Qt::RightToLeft);
            confirmBox.setWindowTitle("تأكيد تسجيل ورقة نقدية جديدة");
            confirmBox.setText("<b>تم رصد ورقة نقدية غير مسجلة في النظام!</b>");
            confirmBox.setInformativeText("الرقم التسلسلي: <b>" + serialData + "</b>\nالتاريخ: " + timestamp + "\n\nهل تريد اعتماد وتسجيل هذه الورقة في النظام؟");
            
            QPushButton *approveButton = confirmBox.addButton("اعتماد وتنسيق الورقة", QMessageBox::AcceptRole);
            QPushButton *rejectButton = confirmBox.addButton("تجاهل / عدم التسجيل", QMessageBox::RejectRole);
            confirmBox.setDefaultButton(approveButton);

            confirmBox.exec();

            if (confirmBox.clickedButton() == approveButton) {
                actionStatus = "تمت الموافقة والتسجيل";
                alertBanner->setText("✔️ تم تسجيل واعتماد الورقة الجديدة بنجاح!");
                alertBanner->setStyleSheet("background-color: #2eb82e; color: white; font-weight: bold; font-size: 15px; padding: 10px; border-radius: 4px;");
            } else {
                actionStatus = "مرفوضة من المستخدم (غير مسجلة)";
                alertBanner->setText("❌ تم رفض تسجيل الورقة بناءً على اختيارك.");
                alertBanner->setStyleSheet("background-color: #ff9900; color: white; font-weight: bold; font-size: 15px; padding: 10px; border-radius: 4px;");
            }
        } else {
            // === حالة ورقة مسجلة مسبقاً ===
            actionStatus = "مسجلة سابقاً تلقائياً";
            alertBanner->setText("✔️ ورقة معروفة: مسجلة ومحفوظة سابقاً في النظام");
            alertBanner->setStyleSheet("background-color: #2eb82e; color: white; font-weight: bold; font-size: 15px; padding: 10px; border-radius: 4px;");
        }

        // 2. تحديث وتخزين القرار في الأرشيف
        dbManager.registerAndSaveSerial(timestamp, serialData, actionStatus);

        // 3. إضافة النتيجة والقرار إلى جدول الواجهة
        int row = dataTable->rowCount();
        dataTable->insertRow(row);
        
        dataTable->setItem(row, 0, new QTableWidgetItem(timestamp));
        dataTable->setItem(row, 1, new QTableWidgetItem(serialData));
        
        QTableWidgetItem *statusItem = new QTableWidgetItem(!isAlreadyRegistered ? "غير محفوظة (جديدة)" : "مسجلة سابقاً");
        statusItem->setForeground(!isAlreadyRegistered ? Qt::red : Qt::darkGreen);
        dataTable->setItem(row, 2, statusItem);
        
        QTableWidgetItem *actionItem = new QTableWidgetItem(actionStatus);
        if (actionStatus == "تمت الموافقة والتسجيل") {
            actionItem->setForeground(Qt::blue);
        } else if (actionStatus == "مرفوضة من المستخدم (غير مسجلة)") {
            actionItem->setForeground(Qt::darkRed);
        }
        dataTable->setItem(row, 3, actionItem);

        // التمرير التلقائي لأحدث سطر
        dataTable->scrollToBottom();
    }
};

// ============================================================================
// نقطة التشغيل الرئيسية (Main Entry)
// ============================================================================
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    MainWindow window;
    window.show();
    
    return app.exec();
}

#include "main.moc"
