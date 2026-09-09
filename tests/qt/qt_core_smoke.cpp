#include <QSysInfo>
#include <QTest>
#include <QtGlobal>

class QtCoreSmoke final : public QObject
{
    Q_OBJECT

private slots:
    void usesExactQtSdk()
    {
        QCOMPARE(QString::fromLatin1(qVersion()), QStringLiteral("6.11.2"));
    }

    void runsAsX64()
    {
        QCOMPARE(QSysInfo::currentCpuArchitecture(), QStringLiteral("x86_64"));
        QCOMPARE(sizeof(void*), std::size_t{8});
    }
};

QTEST_APPLESS_MAIN(QtCoreSmoke)

#include "qt_core_smoke.moc"
