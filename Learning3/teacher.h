#ifndef TEACHER_H
#define TEACHER_H

#include <QObject>

class Teacher : public QObject
{
    Q_OBJECT
public:
    explicit Teacher(QObject *parent = nullptr);

signals:
    void hungry(); //hungry()是信号，只需要声明，不需要在Teacher.cpp中实现！容易犯的错！
};

#endif // TEACHER_H
