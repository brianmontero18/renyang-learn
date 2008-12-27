
#include <QGraphicsEllipseItem>
#include <QTimeLine>
#include <QGraphicsItemAnimation>

#include "myscene.h"

MyScene::MyScene(QObject *parent):QGraphicsScene(parent) {
	// do nothing
}

MyScene::MyScene(qreal x,qreal y,qreal width,qreal height,QObject *parent):QGraphicsScene(x,y,width,height,parent) {
	// 預計加一個圓形,然後,執徑逐漸變大
	QGraphicsEllipseItem *ball = new QGraphicsEllipseItem(0,0,30,30);
	addItem(ball);

	timeLine = new QTimeLine;

	QGraphicsItemAnimation *headAnimation = new QGraphicsItemAnimation;
	headAnimation->setItem(ball);
	headAnimation->setTimeLine(timeLine);
	headAnimation->setScaleAt(1,10,10);

	timeLine->setUpdateInterval(1000 / 25);
	timeLine->setCurveShape(QTimeLine::SineCurve);
	timeLine->setLoopCount(0);
	timeLine->setDuration(2000);
	timeLine->start();
}

MyScene::~MyScene()
{
	delete timeLine;
}

