#ifndef STUDENT_H
#define STUDENT_H

#include <QObject>

class Student : public QObject//继承QObject
{
    Q_OBJECT
public:
    explicit Student(QObject *parent = nullptr);

signals:

public slots:
//旧Qt写法（Qt4/Qt5早期，槽函数必须写在public slots:/private slots:/protected slots:下）
//且类头加Q_OBJECT宏，moc工具识别槽
    void treat(); //只有在QObject子类里才能定义slots

};

#endif // STUDENT_H
