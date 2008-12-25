#include "MainView.h"
#include "MainScene.h"
#include "Car.h"

MainView::MainView(QGraphicsScene *scene,QWidget *parent):QGraphicsView(scene,parent)
{
	// do nothing
}

MainView::MainView(QWidget *parent):QGraphicsView(parent)
{
	scene = new MainScene(this);
	scene->setSceneRect(-300,-300,600,600); // 左上角座標是(-300,-300)
	scene->setItemIndexMethod(QGraphicsScene::NoIndex);
	setScene(scene);
	setRenderHint(QPainter::Antialiasing);
	setBackgroundBrush(QPixmap("images/cheese.jpg"));
	setCacheMode(QGraphicsView::CacheBackground);
	//setDragMode(QGraphicsView::ScrollHandDrag);
	resize(400,300);
}

void MainView::addCar(QGraphicsItem *item)
{
	scene->addItem(item);
}

void MainView::addCar()
{
	Car *car = new Car;
	scene->addItem(car);
}
