#pragma once

#include <QObject>

class TestTranslationCache : public QObject
{
    Q_OBJECT

private slots:
    void roundtrip();
    void missReturnsEmpty();
    void partialHit();
    void clearRemovesAll();
    void docKeyVariesWithFile();
};
