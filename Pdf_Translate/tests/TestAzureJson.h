#pragma once

#include <QObject>

class TestAzureJson : public QObject
{
    Q_OBJECT

private slots:
    void parse_success();
    void parse_mismatchedCount();
    void parse_malformed();
    void parse_errorMessageExtraction();
};
