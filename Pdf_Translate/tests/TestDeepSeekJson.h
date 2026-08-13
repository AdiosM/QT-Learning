#pragma once

#include <QObject>

class TestDeepSeekJson : public QObject
{
    Q_OBJECT

private slots:
    void parseContent_success();
    void parseContent_emptyChoices();
    void parseTranslations_cleanArray();
    void parseTranslations_withMarkdownFences();
    void parseTranslations_withStrayText();
    void parseTranslations_countMismatch();
    void parseTranslations_notFound();
    void parseErrorMessageExtraction();
};
