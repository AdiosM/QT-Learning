#pragma once

#include <QObject>

class TestParagraphSplitter : public QObject
{
    Q_OBJECT

private slots:
    void normalize_joinsLines();
    void normalize_mergesHyphenation();
    void normalize_handlesFormFeedAndCr();
    void split_byBlankLines();
    void split_keepsShortParagraphWhole();
    void split_longParagraphAtSentences();
    void split_hardCutWhenSentenceTooLong();
    void split_skipsDecimalPoints();
    void split_sentencesForLongParagraphUnderLimit();
};
