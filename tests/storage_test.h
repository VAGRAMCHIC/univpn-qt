#pragma once

#include <QObject>

class StorageTest : public QObject {
    Q_OBJECT

private slots:
    void settingsRoundTrip();
    void settingsOmitsOptionalFields();
    void settingsMissingFileReturnsDefaults();
    void settingsCorruptJsonReturnsDefaults();
    void subscriptionRoundTrip();
    void subscriptionReadsFlutterFormat();
    void subscriptionIdStable();
};

