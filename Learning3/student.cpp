#include "student.h"
#include<QtDebug>
Student::Student(QObject *parent)//只有继承自QObject类的对象，才具备使用Qt信号与槽机制的资格
    : QObject{parent}
{}

void Student::treat()
{
    qDebug()<<"请老师吃饭";
}
